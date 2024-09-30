#ifndef HV_PROTO_RPC_ROUTER_H_
#define HV_PROTO_RPC_ROUTER_H_

#include "generated/base.pb.h"

typedef void (*protorpc_handler)(const protorpc::ClientMessage& req, protorpc::ServerMessage* res);

typedef struct {
    const char*      method;
    protorpc_handler handler;
} protorpc_router;

void error_server_response(protorpc::ServerMessage* res, int code, const std::string& message);
void server_not_found(const protorpc::ClientMessage& req, protorpc::ServerMessage* res);
void server_bad_request(const protorpc::ClientMessage& req, protorpc::ServerMessage* res); //server -> client

void rpc_server_login(const protorpc::ClientMessage& req, protorpc::ServerMessage* res);

#endif // HV_PROTO_RPC_ROUTER_H_
