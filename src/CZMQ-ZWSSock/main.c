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

	while (!zctx_interrupted)
	{		
		msg = zwssock_recv(sock);
		
		if (!msg)
			break;

		// first message is the routing id
		id = zmsg_pop(msg);

		while (zmsg_size(msg) != 0)
		{
			char * str = zmsg_popstr(msg);

			printf("%s\n", str);
			
			free(str);
		}

		zmsg_destroy(&msg);

		msg = zmsg_new();

		zmsg_push(msg, id);
		zmsg_addstr(msg, "hello back");

		zwssock_send(sock, &msg);
	}
	
	zwssock_destroy(&sock);
	zctx_destroy(&ctx);	
}
