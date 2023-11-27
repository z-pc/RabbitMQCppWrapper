#include "../src/Channel.h"
#include "../src/Connection.h"
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>

using namespace std;

int main(void)
{
    try
    {
        AMQP::TCPConnection::ptr cnn = AMQP::TCPConnection::createConnection("localhost", 5672);
        cnn->login("/", "guest", "guest");
        auto channel = cnn->createChannel();
        auto exchange_bind = channel->declareExchange("gfd", "direct");
        channel->bindExchange(exchange_bind, "test_existing_exchange", "test_bind_exchange");
        channel->deleteExchange(exchange_bind);
    }
    catch (const exception& e)
    {
        cout << e.what() << endl;
    }
    return 0;
}
