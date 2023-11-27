#include "../src/Channel.h"
#include "../src/Connection.h"
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock.h>

using namespace std;

int main(void)
{
    try
    {
        AMQP::TCPConnection::ptr cnn = AMQP::TCPConnection::createConnection("localhost", 5672);
        cnn->login("/", "guest", "guest");
        auto channel = cnn->createChannel();
        std::string cmsTag = channel->basicConsume("test_existing_queue");

        while (true)
        {
            auto envelope = channel->getMessage({30, 0});
            channel->basicAck(envelope, true);

            if (envelope.msg.bytes)
            {
                string text((char*)envelope.msg.bytes, envelope.msg.len);
                cout << text << endl;
            }
        }

        channel->basicCancel(cmsTag);
    }
    catch (const exception& e)
    {
        cout << e.what() << endl;
    }

    return 0;
}
