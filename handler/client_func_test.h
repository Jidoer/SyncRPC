#ifndef HV_PROTO_RPC_HANDLER_LOGIN_H_
#define HV_PROTO_RPC_HANDLER_LOGIN_H_

#include <base.pb.h>
#include "../generated/login.pb.h"
#include "../router_cilent.h"

void rpc_client_test(const protorpc::ServerMessage& req, protorpc::ClientMessage* res) {
    // params
    if (req.params_size() == 0) {
        // return bad_request(req, res);
        return client_bad_request(req, res);
    }
    protorpc::LoginParam param;
    if (!param.ParseFromString(req.params(0))) {
        return client_bad_request(req, res);
    }

    // result
    protorpc::LoginResult result;
    result.set_user_id(123456);
    result.set_token(param.username() + ":" + param.password());
    res->set_result(result.SerializeAsString());
}

#endif // HV_PROTO_RPC_HANDLER_LOGIN_H_
