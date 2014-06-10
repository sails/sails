#ifndef _HANDLE_RPC_H_
#define _HANDLE_RPC_H_

#include <common/base/handle.h>
#include <common/net/packets.h>

namespace sails {


class HandleRPC : public common::Handle<common::net::PacketCommon*, common::net::PacketCommon*>
{
public:
    void do_handle(common::net::PacketCommon* request, 
		   common::net::PacketCommon* response, 
		   common::HandleChain<common::net::PacketCommon*, common::net::PacketCommon*> *chain);
    void decode_protobuf(common::net::PacketRPC *brequest, common::net::PacketRPC *response, common::HandleChain<common::net::PacketCommon *, common::net::PacketCommon *> *chain);
};


} //namespace sails


#endif /* _HANDLE_RPC_H_ */
