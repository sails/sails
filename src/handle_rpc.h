#ifndef _HANDLE_RPC_H_
#define _HANDLE_RPC_H_

#include <common/base/handle.h>
#include <common/net/packets.h>

namespace sails {

#define MAX_CONTENT_LEN  1024

class HandleRPC : public common::Handle<common::net::PacketCommon*, common::net::ResponseContent*>
{
public:
    void do_handle(common::net::PacketCommon* request, 
		   common::net::ResponseContent* response, 
		   common::HandleChain<common::net::PacketCommon*, common::net::ResponseContent*> *chain);
    void decode_protobuf(common::net::PacketRPC *brequest, common::net::ResponseContent *response, common::HandleChain<common::net::PacketCommon *, common::net::ResponseContent *> *chain);
    ~HandleRPC();
};


} //namespace sails


#endif /* _HANDLE_RPC_H_ */
