ZWSSock - ZeroMQ over WebSocket library
=======

ZWSSock is implement [ZWS (ZeroMQ WebSocket)](http://rfc.zeromq.org/spec:39) for use in ZeroMQ applications.

ZWS and ZWSSock are both in early stage and protocol is not yet finalized and so this library.

The API of ZWSSock is very similar to the API of zsock of CZMQ v3.0, so using it should be very simple.

While ZWSSock implement the server side of ZWS [JSMQ](https://github.com/zeromq/JSMQ) library implement ZWS on the browser side. 
[JSMQ](https://github.com/zeromq/JSMQ) is javascript library which implement the ZWS protocol and expose ZeroMQ like API. You can learn more in the [JSMQ](https://github.com/zeromq/JSMQ) page.

So with JSMQ and ZWSSock javascript applications can talk directly to zeromq applications without a webserver in the middle, however if you want to use SSL (e.g wss://someaddress) you a need a webserver or loadbalaner in the middle to terminate SSL because ZWSSock doesn't support SSL.

Please note that when using WebSocket it is recommended to do it over SSL because not all firewalls likes non-text protocols over port 80.

ZWSSock currently implements the router pattern. Publisher pattern is next to come (JSMQ implements subscriber and dealer).

To use the ZWSSock take a look at [main.c](https://github.com/somdoron/zwssock/blob/master/src/CZMQ-ZWSSock/main.c) file. 
The project also includes [broswer side example](https://github.com/somdoron/zwssock/blob/master/src/CZMQ-ZWSSock/Example.html).

To build on Linux:

   Review src/CZMQ-ZWSSock/Makefile - CFLAGS and LIBS assume an install and pkg-config. Tested with ØMQ=4.0.4 czmq=2.2.0.

   cd src/CZMQ-ZWSSock
   make

This  creates the 'zwstest' program in the same directory. By default it will listen on 127.0.0.1:8000.
Pass a different listen socket as single argument:

     ./zwstest tcp://0.0.0.0:1234
