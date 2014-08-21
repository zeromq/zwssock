#ifndef __ZWSHANDSHAKE_H_INCLUDED__
#define __ZWSHANDSHAKE_H_INCLUDED__

#include <czmq.h>

typedef struct _zwshandshake_t zwshandshake_t;

zwshandshake_t* zwshandshake_new();

void zwshandshake_destroy(zwshandshake_t **self_p);

bool zwshandshake_parse_request(zwshandshake_t *self, zframe_t* data);

zframe_t* zwshandshake_get_response(zwshandshake_t *self);

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif

#endif