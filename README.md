ZWSSock - ZeroMQ over WebSocket library
=======================================

ZWSSock implements [ZWS (ZeroMQ WebSocket)](http://rfc.zeromq.org/spec:39) for use in ZeroMQ applications. Additionally it supports [Compression Extensions for WebSocket](https://tools.ietf.org/html/draft-ietf-hybi-permessage-compression-28) for per message deflate.

ZWS and ZWSSock are both in early stage and the protocol is not yet finalized nor is this library.
The API of ZWSSock is very similar to the API of zsock of CZMQ v3.0, so using it should be very simple.
While ZWSSock implements the server side of ZWS [JSMQ](https://github.com/zeromq/JSMQ) library implement ZWS on the browser side. 
[JSMQ](https://github.com/zeromq/JSMQ) is javascript library which implements the ZWS protocol and exposes ZeroMQ like API. More information is available via the [JSMQ](https://github.com/zeromq/JSMQ) repository.

With JSMQ and ZWSSock, javascript applications can talk directly to ZeroMQ applications without a webserver in the middle, however if you want to use SSL (e.g wss://someaddress) you will need a webserver or loadbalaner in the middle to terminate SSL because ZWSSock does not support SSL.

Please note that when using WebSocket it is recommended to do it over SSL because not all firewalls likes non-text protocols over port 80.

ZWSSock currently implements the router pattern. Publisher pattern is next to come (JSMQ implements subscriber and dealer).

To use the ZWSSock take a look at [main.c](https://github.com/somdoron/zwssock/blob/master/src/CZMQ-ZWSSock/main.c) file. 
The project also includes [browser side example](https://github.com/somdoron/zwssock/blob/master/src/CZMQ-ZWSSock/Example.html).

On Linux please review src/CZMQ-ZWSSock/Makefile - CFLAGS and LIBS assume an installation of pkg-config. The code is tested with ØMQ=4.0.4 czmq=2.2.0. To build:

    cd src/CZMQ-ZWSSock
    make

This  creates the 'zwstest' program in the same directory. By default `zwstest` will listen on 127.0.0.1:8000.
Pass a different listen socket as single argument:

     ./zwstest tcp://0.0.0.0:1234
