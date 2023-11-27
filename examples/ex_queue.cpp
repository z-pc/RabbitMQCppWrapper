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
        AMQP::Table queuePrps;
        queuePrps.addEntry(AMQP_FIELD_EXPIRES, 300000, AMQP_FIELD_SINGLE_ACTIVE_CONSUMER, true);
        auto queueName = channel->declareQueue("", false, false, false, true, &queuePrps);
        channel->bindQueue("test_existing_exchange", queueName, queueName);
    }
    catch (const exception& e)
    {
        cout << e.what() << endl;
    }
    return 0;
}
