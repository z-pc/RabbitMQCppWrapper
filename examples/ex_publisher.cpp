#include "../src/Channel.h"
#include "../src/Connection.h"
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

using namespace std;

int main(void)
{
    try
    {
        AMQP::TCPConnection::ptr cnn = AMQP::TCPConnection::createConnection("localhost", 5672);
        cnn->login("/", "guest", "guest");
        auto channel = cnn->createChannel();
        channel->basicPublish("test_existing_exchange", "test_existing_queue", "congratulation");
    }
    catch (const exception& e)
    {
        cout << e.what() << endl;
    }

    return 0;
}
