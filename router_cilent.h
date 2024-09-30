#ifndef HV_PROTO_RPC_ROUTER_H_
#define HV_PROTO_RPC_ROUTER_H_

#include "generated/base.pb.h"

typedef void (*protorpc_handler)(const protorpc::ServerMessage &req, protorpc::ClientMessage *res);

typedef struct
{
    const char *method;
    protorpc_handler handler;
} protorpc_router;

void error_client_response(protorpc::ClientMessage *res, int code, const std::string &message);
void client_not_found(const protorpc::ServerMessage &req, protorpc::ClientMessage *res);
void client_bad_request(const protorpc::ServerMessage &req, protorpc::ClientMessage *res); // client -> server

void rpc_client_test(const protorpc::ServerMessage &req, protorpc::ClientMessage *res);

void error_response(protorpc::ClientMessage *res, int code, const std::string &message)
{
    res->mutable_error()->set_code(code);
    res->mutable_error()->set_message(message);
}

void client_not_found(const protorpc::ServerMessage &req, protorpc::ClientMessage *res)
{
    error_response(res, 404, "Not Found");
}
void client_bad_request(const protorpc::ServerMessage &req, protorpc::ClientMessage *res)
{
    error_response(res, 400, "Bad Request");
}
void rpc_client_test(const protorpc::ServerMessage &req, protorpc::ClientMessage *res)
{
    // params
    if (req.params_size() == 0)
    {
        // return bad_request(req, res);
        return client_bad_request(req, res);
    }
    protorpc::LoginParam param;
    if (!param.ParseFromString(req.params(0)))
    {
        return client_bad_request(req, res);
    }

    // result
    protorpc::LoginResult result;
    result.set_user_id(123456);
    result.set_token(param.username() + ":" + param.password());
    res->set_result(result.SerializeAsString());
}

#endif // HV_PROTO_RPC_ROUTER_H_
