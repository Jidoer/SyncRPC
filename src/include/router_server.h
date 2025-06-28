#ifndef HV_PROTO_RPC_ROUTER_H_
#define HV_PROTO_RPC_ROUTER_H_
#ifdef SERVER
#include "../common/generated/base.pb.h"
#include "../common/generated/login.pb.h"

typedef void (*protorpc_handler)(const protorpc::ClientMessage &req, protorpc::ServerMessage *res);

typedef struct
{
    const char *method;
    protorpc_handler handler;
} protorpc_router;

void error_response(protorpc::ServerMessage *res, int code, const std::string &message)
{
    res->mutable_error()->set_code(code);
    res->mutable_error()->set_message(message);
}

// void not_found(const protorpc::ServerMessage& req, protorpc::ServerMessage* res) {
//     error_response(res, 404, "Not Found");
// }

// void bad_request(const protorpc::ServerMessage& req, protorpc::ServerMessage* res) {
//     error_response(res, 400, "Bad Request");
// }
void error_server_response(protorpc::ServerMessage *res, int code, const std::string &message);
void server_not_found(const protorpc::ClientMessage &req, protorpc::ServerMessage *res)
{
    error_response(res, 404, "Not Found");
}
void server_bad_request(const protorpc::ClientMessage &req, protorpc::ServerMessage *res) // server -> client
{
    error_response(res, 400, "Bad Request");
}

void rpc_server_login(const protorpc::ClientMessage &req, protorpc::ServerMessage *res);

void rpc_server_login(const protorpc::ClientMessage& req, protorpc::ServerMessage* res) {
    // params
    if (req.params_size() == 0) {
        return server_bad_request(req, res);
    }
    protorpc::LoginParam param;
    if (!param.ParseFromString(req.params(0))) {
        return server_bad_request(req, res);
    }

    // result
    protorpc::LoginResult result;
    result.set_user_id(123456);
    result.set_token(param.username() + ":" + param.password());
    res->set_result(result.SerializeAsString());
}


#endif
#endif // HV_PROTO_RPC_ROUTER_H_
