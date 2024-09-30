/*
 * proto rpc client
 *
 * @build   make protorpc
 * @server  bin/protorpc_server 1234
 * @client  bin/protorpc_client 127.0.0.1 1234 add 1 2
 *
 */

#include <hv/TcpClient.h>

#include <mutex>
#include <condition_variable>

using namespace hv;

#include "protorpc.h"
#include "../../../usr/include/google/protobuf/stubs/common.h"
#include "generated/base.pb.h"
#include "generated/calc.pb.h"
#include "generated/login.pb.h"
#include "router_cilent.h"
// #include "handler/client_func_test.h"

// valgrind --leak-check=full --show-leak-kinds=all
class ProtobufRAII
{
public:
    ProtobufRAII()
    {
    }
    ~ProtobufRAII()
    {
        google::protobuf::ShutdownProtobufLibrary();
    }
};
static ProtobufRAII s_protobuf;
protorpc_router router[] = {
    {"rpc_client_test", rpc_client_test},
};
#define PROTORPC_ROUTER_NUM (sizeof(router) / sizeof(router[0]))

namespace protorpc
{
    typedef std::shared_ptr<protorpc::ServerMessage> RequestPtr;  // server -> client(me)
    typedef std::shared_ptr<protorpc::ClientMessage> ResponsePtr; // client(me) -> server

    enum ProtoRpcResult
    {
        kRpcSuccess = 0,
        kRpcTimeout = -1,
        kRpcError = -2,
        kRpcNoResult = -3,
        kRpcParseError = -4,
    };

    class ProtoRpcContext
    {
    public:
        protorpc::RequestPtr req;
        protorpc::ResponsePtr res;

    private:
        std::mutex _mutex;
        std::condition_variable _cond;

    public:
        void wait(int timeout_ms)
        {
            std::unique_lock<std::mutex> locker(_mutex);
            _cond.wait_for(locker, std::chrono::milliseconds(timeout_ms));
        }

        void notify()
        {
            _cond.notify_one();
        }
    };
    typedef std::shared_ptr<ProtoRpcContext> ContextPtr;

    class ProtoRpcClient : public TcpClient
    {
    public:
        ProtoRpcClient() : TcpClient()
        {
            connect_state = kInitialized;

            setConnectTimeout(5000);

            reconn_setting_t reconn;
            reconn_setting_init(&reconn);
            reconn.min_delay = 1000;
            reconn.max_delay = 10000;
            reconn.delay_policy = 2;
            setReconnect(&reconn);

            // init protorpc_unpack_setting
            unpack_setting_t protorpc_unpack_setting;
            memset(&protorpc_unpack_setting, 0, sizeof(unpack_setting_t));
            protorpc_unpack_setting.mode = UNPACK_BY_LENGTH_FIELD;
            protorpc_unpack_setting.package_max_length = DEFAULT_PACKAGE_MAX_LENGTH;
            protorpc_unpack_setting.body_offset = PROTORPC_HEAD_LENGTH;
            protorpc_unpack_setting.length_field_offset = PROTORPC_HEAD_LENGTH_FIELD_OFFSET;
            protorpc_unpack_setting.length_field_bytes = PROTORPC_HEAD_LENGTH_FIELD_BYTES;
            protorpc_unpack_setting.length_field_coding = ENCODE_BY_BIG_ENDIAN;
            setUnpack(&protorpc_unpack_setting);

            onConnection = [this](const SocketChannelPtr &channel)
            {
                std::string peeraddr = channel->peeraddr();
                if (channel->isConnected())
                {
                    connect_state = kConnected;
                    printf("connected to %s! connfd=%d\n", peeraddr.c_str(), channel->fd());
                }
                else
                {
                    connect_state = kDisconnectd;
                    printf("disconnected to %s! connfd=%d\n", peeraddr.c_str(), channel->fd());
                }
            };

            onMessage = [this](const SocketChannelPtr &channel, Buffer *buf)
            {
                // client server
                // unpack -> Request::ParseFromArray -> router -> Response::SerializeToArray -> pack -> Channel::write

                // protorpc_unpack
                protorpc_message msg;
                memset(&msg, 0, sizeof(msg));
                int packlen = protorpc_unpack(&msg, buf->data(), buf->size());
                if (packlen < 0)
                {
                    printf("protorpc_unpack failed!\n");
                    return;
                }
                assert(packlen == buf->size());
                if (protorpc_head_check(&msg.head) != 0)
                {
                    printf("protorpc_head_check failed!\n");
                    return;
                }
                // Response::ParseFromArray
                // add client func
                protorpc::ServerMessage req; // serever --> client(me)
                protorpc::ClientMessage res; //(me) -> server
                // end
                // client 's router
                if (req.is_req())
                {
                    if (req.ParseFromArray(msg.body, msg.head.length))
                    {
                        printf("> %s\n", req.DebugString().c_str());
                        res.set_id(req.id());
                        // router
                        const char *method = req.method().c_str();
                        bool found = false;
                        for (int i = 0; i < PROTORPC_ROUTER_NUM; ++i)
                        {
                            if (strcmp(method, router[i].method) == 0)
                            {
                                found = true;
                                router[i].handler(req, &res);
                                break;
                            }
                        }
                        if (!found)
                        {
                            client_not_found(req, &res);
                        }
                    }
                    else
                    {
                        client_bad_request(req, &res);
                    }

                    // Response::SerializeToArray + protorpc_pack
                    protorpc_message_init(&msg);
                    msg.head.length = res.ByteSize();
                    packlen = protorpc_package_length(&msg.head);
                    unsigned char *writebuf = NULL;
                    HV_STACK_ALLOC(writebuf, packlen);
                    packlen = protorpc_pack(&msg, writebuf, packlen);
                    if (packlen > 0)
                    {
                        printf("< %s\n", res.DebugString().c_str());
                        res.SerializeToArray(writebuf + PROTORPC_HEAD_LENGTH, msg.head.length);
                        channel->write(writebuf, packlen);
                    }
                    HV_STACK_FREE(writebuf);
                    return;
                }

                auto res0 = std::make_shared<protorpc::ClientMessage>(); // client(me)---ing--->server
                if (!res0->ParseFromArray(msg.body, msg.head.length))
                {
                    return;
                }
                // id => res0
                calls_mutex.lock();
                auto iter = calls.find(res0->id());
                if (iter == calls.end())
                {
                    calls_mutex.unlock();
                    return;
                }
                auto ctx = iter->second;
                calls_mutex.unlock();
                ctx->res = res0;
                ctx->notify();
            };
        }

        int connect(int port, const char *host = "127.0.0.1")
        {
            createsocket(port, host);
            connect_state = kConnecting;
            start();
            return 0;
        }

        protorpc::ResponsePtr call(protorpc::RequestPtr &req, int timeout_ms = 10000)
        {
            if (connect_state != kConnected)
            {
                return NULL;
            }
            static std::atomic<uint64_t> s_id = ATOMIC_VAR_INIT(0);
            req->set_id(++s_id);
            req->id();
            auto ctx = std::make_shared<protorpc::ProtoRpcContext>();
            ctx->req = req;
            calls_mutex.lock();
            calls[req->id()] = ctx;
            calls_mutex.unlock();
            // Request::SerializeToArray + protorpc_pack
            protorpc_message msg;
            protorpc_message_init(&msg);
            msg.head.length = req->ByteSize();
            int packlen = protorpc_package_length(&msg.head);
            unsigned char *writebuf = NULL;
            HV_STACK_ALLOC(writebuf, packlen);
            packlen = protorpc_pack(&msg, writebuf, packlen);
            if (packlen > 0)
            {
                printf("%s\n", req->DebugString().c_str());
                req->SerializeToArray(writebuf + PROTORPC_HEAD_LENGTH, msg.head.length);
                channel->write(writebuf, packlen);
            }
            HV_STACK_FREE(writebuf);
            // wait until response come or timeout
            ctx->wait(timeout_ms);
            auto res = ctx->res;
            calls_mutex.lock();
            calls.erase(req->id());
            calls_mutex.unlock();
            if (res == NULL)
            {
                printf("RPC timeout!\n");
            }
            else if (res->has_error())
            {
                printf("RPC error:\n%s\n", res->error().DebugString().c_str());
            }
            return res;
        }

        // int login(const protorpc::LoginParam& param, protorpc::LoginResult* result) {
        //     auto req = std::make_shared<protorpc::Request>();
        //     // method
        //     req->set_method("login");
        //     // params
        //     req->add_params()->assign(param.SerializeAsString());

        //     auto res = call(req);

        //     if (res == NULL) return kRpcTimeout;
        //     if (res->has_error()) return kRpcError;
        //     if (res->result().empty()) return kRpcNoResult;
        //     if (!result->ParseFromString(res->result())) return kRpcParseError;
        //     return kRpcSuccess;
        // }

        int client_test(const protorpc::LoginParam &param, protorpc::LoginResult *result)
        {
            auto req = std::make_shared<protorpc::ServerMessage>();
            // method
            req->set_method("rpc_client_test");
            // params
            req->add_params()->assign(param.SerializeAsString());

            auto res = call(req);

            if (res == NULL)
                return kRpcTimeout;
            if (res->has_error())
                return kRpcError;
            if (res->result().empty())
                return kRpcNoResult;
            if (!result->ParseFromString(res->result()))
                return kRpcParseError;
            return kRpcSuccess;
        }

        enum
        {
            kInitialized,
            kConnecting,
            kConnected,
            kDisconnectd,
        } connect_state;
        std::map<uint64_t, protorpc::ContextPtr> calls;
        std::mutex calls_mutex;
    };
}

int main(int argc, char **argv)
{
    if (argc < 6)
    {
        printf("Usage: %s host port method param1 param2\n", argv[0]);
        printf("method = [add, sub, mul, div]\n");
        printf("Examples:\n");
        printf("  %s 127.0.0.1 1234 add 1 2\n", argv[0]);
        printf("  %s 127.0.0.1 1234 div 1 0\n", argv[0]);
        return -10;
    }
    const char *host = argv[1];
    int port = atoi(argv[2]);
    const char *method = argv[3];
    const char *param1 = argv[4];
    const char *param2 = argv[5];

    protorpc::ProtoRpcClient cli;
    cli.connect(port, host);
    while (cli.connect_state == protorpc::ProtoRpcClient::kConnecting)
        hv_msleep(1);
    if (cli.connect_state == protorpc::ProtoRpcClient::kDisconnectd)
    {
        return -20;
    }

    // // test login
    // {
    //     protorpc::LoginParam param;
    //     param.set_username("admin");
    //     param.set_password("123456");
    //     protorpc::LoginResult result;
    //     if (cli.login(param, &result) == protorpc::kRpcSuccess) {
    //         printf("login success!\n");
    //         printf("%s\n", result.DebugString().c_str());
    //     } else {
    //         printf("login failed!\n");
    //     }
    // }

    // test calc
    // {
    //     int num1 = atoi(param1);
    //     int num2 = atoi(param2);
    //     int result = 0;
    //     if (cli.calc(method, num1, num2, result) == protorpc::kRpcSuccess) {
    //         printf("calc success!\n");
    //         printf("%d %s %d = %d\n", num1, method, num2, result);
    //     } else {
    //         printf("calc failed!\n");
    //     }
    // }
    return 0;
}
