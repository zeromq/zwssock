#include <czmq.h>

#include "zwssock.h"

#define ZMQ_STATIC

static char *listen_on = "tcp://127.0.0.1:8000";

int main(int argc, char **argv)
{
	zwssock_t *sock;

	char *l =  argc > 1 ? argv[1] : listen_on;

	int major, minor, patch;
	zsys_version (&major, &minor, &patch);
	printf("built with: ØMQ=%d.%d.%d czmq=%d.%d.%d\n",
	       major, minor, patch,
	       CZMQ_VERSION_MAJOR, CZMQ_VERSION_MINOR,CZMQ_VERSION_PATCH);


	sock = zwssock_new_router();

	zwssock_bind(sock, l);

	zmsg_t* msg;
	zframe_t *id;

	while (!zsys_interrupted)
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

		int rc = zwssock_send(sock, &msg);
		if (rc != 0)
			zmsg_destroy(&msg);
	}

	zwssock_destroy(&sock);
}
