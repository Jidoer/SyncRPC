#ifndef HV_PROTO_RPC_HANDLER_LOGIN_H_
#define HV_PROTO_RPC_HANDLER_LOGIN_H_

#include "router_server.h"

#include "../common/generated/login.pb.h"

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

#endif // HV_PROTO_RPC_HANDLER_LOGIN_H_
