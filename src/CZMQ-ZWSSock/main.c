#include <czmq.h>

#include "zwssock.h"


int main(int argc, char **argv)
{
	zctx_t *ctx;
	zwssock_t *sock;

	ctx = zctx_new();
	sock = zwssock_new_router(ctx);

	zwssock_bind(sock , "tcp://127.0.0.1:80");	

	zmsg_t* msg;
	zframe_t *id;

	while (true)
	{		
		msg = zwssock_recv(sock);
		
		// first message is the routing id
		id = zmsg_pop(msg);

		while (zmsg_size(msg) != 0)
		{
			printf("%s\n", zmsg_popstr(msg));			
		}

		zmsg_destroy(&msg);

		msg = zmsg_new();

		zmsg_push(msg, id);
		zmsg_addstr(msg, "You just sent ZWS message");

		zwssock_send(sock, &msg);
	}
	
	

	
}
