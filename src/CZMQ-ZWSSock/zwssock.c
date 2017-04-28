#include "zwssock.h"
#include "zwshandshake.h"
#include "zwsdecoder.h"

#include <czmq.h>
#include <string.h>

struct _zwssock_t
{
	zctx_t *ctx;                //  Our parent context
	void *control;              //  Control to/from agent
	void *data;                 //  Data to/from agent
};

//  This background thread does all the real work
static void s_agent_task(void *args, zctx_t *ctx, void *control);

//  --------------------------------------------------------------------------
//  Constructor

zwssock_t* zwssock_new_router(zctx_t *ctx)
{
	zwssock_t *self = (zwssock_t *)zmalloc(sizeof(zwssock_t));

	assert(self);

	self->ctx = ctx;
	self->control = zthread_fork(self->ctx, s_agent_task, NULL);

	//  Create separate data socket, send address on control socket
	self->data = zsocket_new(self->ctx, ZMQ_PAIR);
	assert(self->data);
	int rc = zsocket_bind(self->data, "inproc://data-%p", self->data);
	assert(rc != -1);
	zstr_sendf(self->control, "inproc://data-%p", self->data);

	return self;
}

//  --------------------------------------------------------------------------
//  Destructor

void zwssock_destroy(zwssock_t **self_p)
{
	assert(self_p);
	if (*self_p) {
		zwssock_t *self = *self_p;
		zstr_send(self->control, "TERMINATE");

		free(zstr_recv(self->control));
		free(self);
		*self_p = NULL;
	}
}

int zwssock_bind(zwssock_t *self, char *endpoint)
{
	assert(self);
	return zstr_sendx(self->control, "BIND", endpoint, NULL);
}

int zwssock_send(zwssock_t *self, zmsg_t **msg_p)
{
	assert(self);
	assert(zmsg_size(*msg_p) > 0);
	zmsg_send(msg_p, self->data);
	return 0;
}

zmsg_t * zwssock_recv(zwssock_t *self)
{
	assert(self);
	zmsg_t *msg = zmsg_recv(self->data);
	return msg;
}

void* zwssock_handle(zwssock_t *self)
{
	assert(self);
	return self->data;
}


//  *************************    BACK END AGENT    *************************

typedef struct {
	zctx_t *ctx;                //  CZMQ context
	void *control;              //  Control socket back to application
	void *data;                 //  Data socket to application
	void *stream;               //  stream socket to server
	zhash_t *clients;           //  Clients known so far
	bool terminated;            //  Agent terminated by API
} agent_t;

static agent_t *
s_agent_new(zctx_t *ctx, void *control)
{
	agent_t *self = (agent_t *)zmalloc(sizeof(agent_t));
	self->ctx = ctx;
	self->control = control;
	self->stream = zsocket_new(ctx, ZMQ_STREAM);

	//  Connect our data socket to caller's endpoint
	self->data = zsocket_new(ctx, ZMQ_PAIR);
	char *endpoint = zstr_recv(self->control);
	int rc = zsocket_connect(self->data, "%s", endpoint);
	assert(rc != -1);
	free(endpoint);

	self->clients = zhash_new();
	return self;
}

static void
s_agent_destroy(agent_t **self_p)
{
	assert(self_p);
	if (*self_p) {
		agent_t *self = *self_p;
		zhash_destroy(&self->clients);
		free(self);
		*self_p = NULL;
	}
}

//  This section covers a single client connection
typedef enum {
	closed = 0,
	connected = 1,              //  Ready for messages
	exception = 2               //  Failed due to some error
} state_t;

typedef struct {
	agent_t *agent;             //  Agent for this client
	state_t state;              //  Current state
	zframe_t *address;          //  Client address identity
	char *hashkey;              //  Key into clients hash
	zwsdecoder_t* decoder;

	zmsg_t *outgoing_msg;		// Currently outgoing message, if not NULL final frame was not yet arrived
} client_t;

static client_t *
client_new(agent_t *agent, zframe_t *address)
{
	client_t *self = (client_t *)zmalloc(sizeof(client_t));
	assert(self);
	self->agent = agent;
	self->address = zframe_dup(address);
	self->hashkey = zframe_strhex(address);
	self->state = closed;
	self->decoder = NULL;
	self->outgoing_msg = NULL;
	return self;
}

static void
client_destroy(client_t **self_p)
{
	assert(self_p);
	if (*self_p) {
		client_t *self = *self_p;
		zframe_destroy(&self->address);

		if (self->decoder != NULL)
		{
			zwsdecoder_destroy(&self->decoder);
		}

		if (self->outgoing_msg != NULL)
		{
			zmsg_destroy(&self->outgoing_msg);
		}

		free(self->hashkey);
		free(self);
		*self_p = NULL;
	}
}

void router_message_received(void *tag, byte* payload, int length, bool more)
{
	client_t *self = (client_t *)tag;

	assert(!more);

	if (self->outgoing_msg == NULL)
	{
		self->outgoing_msg = zmsg_new();
		zmsg_addstr(self->outgoing_msg, self->hashkey);
	}

	zmsg_addmem(self->outgoing_msg, payload, length);

	if (!more)
	{
		zmsg_send(&self->outgoing_msg, self->agent->data);
	}
}

void close_received(void *tag, byte* payload, int length)
{
	// TODO: close received
}

void ping_received(void *tag, byte* payload, int length)
{
	// TODO: implement ping
}

void pong_received(void *tag, byte* payload, int length)
{
	// TOOD: implement pong
}

static void client_data_ready(client_t * self)
{
	zframe_t* data;
	zwshandshake_t * handshake;

	data = zframe_recv(self->agent->stream);

	switch (self->state)
	{
	case closed:
		// TODO: we might didn't receive the entire request, make the zwshandshake able to handle multiple inputs

		handshake = zwshandshake_new();
		if (zwshandshake_parse_request(handshake, data))
		{
			// request is valid, getting the response
			zframe_t* response = zwshandshake_get_response(handshake);

			zframe_t *address = zframe_dup(self->address);

			zframe_send(&address, self->agent->stream, ZFRAME_MORE);
			zframe_send(&response, self->agent->stream, 0);

			free(response);

			self->decoder = zwsdecoder_new(self, &router_message_received, &close_received, &ping_received, &pong_received);
			self->state = connected;
		}
		else
		{
			// request is invalid
			// TODO: return http error and send null message to close the socket
			self->state = exception;
		}
		zwshandshake_destroy(&handshake);

		break;
	case connected:
		zwsdecoder_process_buffer(self->decoder, data);

		if (zwsdecoder_is_errored(self->decoder))
		{
			self->state = exception;
		}

		break;

	case exception:
		// ignoring the message
		break;
	}

	zframe_destroy(&data);
}

//  Callback when we remove client from 'clients' hash table
static void
client_free(void *argument)
{
	client_t *client = (client_t *)argument;
	client_destroy(&client);
}

static int
s_agent_handle_control(agent_t *self)
{
	//  Get the whole message off the control socket in one go
	zmsg_t *request = zmsg_recv(self->control);
	char *command = zmsg_popstr(request);
	if (!command)
		return -1;                  //  Interrupted

	if (streq(command, "BIND")) {
		char *endpoint = zmsg_popstr(request);
		puts(endpoint);
		int rc = zsocket_bind(self->stream, "%s", endpoint);
		assert(rc != -1);
		free(endpoint);
	}
	else if (streq(command, "UNBIND")) {
		char *endpoint = zmsg_popstr(request);
		int rc = zsocket_unbind(self->stream, "%s", endpoint);
		assert(rc != -1);
		free(endpoint);
	}
	else if (streq(command, "TERMINATE")) {
		self->terminated = true;
		zstr_send(self->control, "OK");
	}
	free(command);
	zmsg_destroy(&request);
	return 0;
}

//  Handle a message from the server

static int
s_agent_handle_router(agent_t *self)
{
	zframe_t *address = zframe_recv(self->stream);
	char *hashkey = zframe_strhex(address);
	client_t *client = zhash_lookup(self->clients, hashkey);
	if (client == NULL) {
		client = client_new(self, address);

		zhash_insert(self->clients, hashkey, client);
		zhash_freefn(self->clients, hashkey, client_free);
	}
	free(hashkey);
	zframe_destroy(&address);

	client_data_ready(client);

	//  If client is misbehaving, remove it
	if (client->state == exception)
		zhash_delete(self->clients, client->hashkey);

	return 0;
}

static int
s_agent_handle_data(agent_t *self)
{
	//  First frame is client address (hashkey)
	//  If caller sends unknown client address, we discard the message
	//  The assert disappears when we start to timeout clients...
	zmsg_t *request = zmsg_recv(self->data);
	char *hashkey = zmsg_popstr(request);
	client_t *client = zhash_lookup(self->clients, hashkey);

	zframe_t* address;

	if (client) {
		//  Each frame is a full ZMQ message with identity frame
		while (zmsg_size(request)) {
			zframe_t *receivedFrame = zmsg_pop(request);
			bool more = false;

			if (zmsg_size(request))
				more = true;

			int frameSize = 2 + 1 + zframe_size(receivedFrame);
			int payloadStartIndex = 2;
			int payloadLength = zframe_size(receivedFrame) + 1;

			if (payloadLength > 125)
			{
				frameSize += 2;
				payloadStartIndex += 2;

				if (payloadLength > 0xFFFF) // 2 bytes max value
				{
					frameSize += 6;
					payloadStartIndex += 6;
				}
			}

			byte* outgoingData = (byte*)zmalloc(frameSize);

			outgoingData[0] = (byte)0x82; // Binary and Final

			// No mask
			outgoingData[1] = 0x00;

			if (payloadLength <= 125)
			{
				outgoingData[1] |= (byte)(payloadLength & 127);
			}
			else if (payloadLength <= 0xFFFF) // maximum size of short
			{
				outgoingData[1] |= 126;
				outgoingData[2] = (payloadLength >> 8) & 0xFF;
				outgoingData[3] = payloadLength & 0xFF;
			}
			else
			{
				outgoingData[1] |= 127;
				outgoingData[2] = 0;
				outgoingData[3] = 0;
				outgoingData[4] = 0;
				outgoingData[5] = 0;
				outgoingData[6] = (payloadLength >> 24) & 0xFF;
				outgoingData[7] = (payloadLength >> 16) & 0xFF;
				outgoingData[8] = (payloadLength >> 8) & 0xFF;
				outgoingData[9] = payloadLength & 0xFF;
			}

			// more byte
			outgoingData[payloadStartIndex] = (byte)(more ? 1 : 0);
			payloadStartIndex++;

			// payload
			memcpy(outgoingData + payloadStartIndex, zframe_data(receivedFrame), zframe_size(receivedFrame));

			address = zframe_dup(client->address);

			zframe_send(&address, self->stream, ZFRAME_MORE);
			zsocket_sendmem(self->stream, outgoingData, frameSize, 0);

			free(outgoingData);
			zframe_destroy(&receivedFrame);

			// TODO: check return code, on return code different than 0 or again set exception
		}
	}

	free(hashkey);
	zmsg_destroy(&request);
	return 0;
}

void s_agent_task(void *args, zctx_t *ctx, void *control)
{
	//  Create agent instance as we start this task
	agent_t *self = s_agent_new(ctx, control);
	if (!self)                  //  Interrupted
		return;

	//  We always poll all three sockets
	zmq_pollitem_t pollitems[] = {
			{ self->control, 0, ZMQ_POLLIN, 0 },
			{ self->stream, 0, ZMQ_POLLIN, 0 },
			{ self->data, 0, ZMQ_POLLIN, 0 }
	};
	while (!zctx_interrupted) {
		if (zmq_poll(pollitems, 3, -1) == -1)
			break;              //  Interrupted

		if (pollitems[0].revents & ZMQ_POLLIN)
			s_agent_handle_control(self);
		if (pollitems[1].revents & ZMQ_POLLIN)
			s_agent_handle_router(self);
		if (pollitems[2].revents & ZMQ_POLLIN)
			s_agent_handle_data(self);

		if (self->terminated)
			break;
	}
	//  Done, free all agent resources
	s_agent_destroy(&self);
}
