ZWSSock
=======

ZWSSock is C implementation of [ZWS](http://rfc.zeromq.org/spec:39) for ZeroMQ.

ZWS and ZWSSock are both in early stage, the protocol is not yet finalized and so this library.

The API of ZWSSock is very similar to the API of zsock of CZMQ v3.0, so using it should be very simple.

ZWSSock currently only implement router pattern.

[JSMQ](https://github.com/zeromq/JSMQ) is Javascript port of zeromq which implement the ZWS protocol and can connect to ZWSSock.

To use the ZWSSock take a look at [main.c](https://github.com/somdoron/zwssock/blob/master/src/CZMQ-ZWSSock/main.c) file. The project also include [an example of JSMQ](https://github.com/somdoron/zwssock/blob/master/src/CZMQ-ZWSSock/Example.html).

