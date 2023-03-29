#include "src/Channel.h"
#include "src/Connection.h"
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>

#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#include <stdlib.h>

using namespace std;

int main(void)
{
    try
    {
        AMQP::TCPConnection::ptr cnn = AMQP::TCPConnection::createConnection("localhost", 5672);

        // create connection with properties
        {
            AMQP::Table table;
            table.addEntry("connection_name", "rbmq_test_cxx_wrapper");
            cnn->login("/", "guest", "guest", &table);

            timeval timeOut{30, 0};
            cnn->setRPCTimeOut(timeOut);
        }

        auto channel = cnn->createChannel();
        auto exchange = channel->declareExchange("test_declare_exchange", "direct");

        // exchange bind
        {
            auto exchange_bind = channel->declareExchange("test_declare_exchange_bind", "direct");
            channel->bindExhange(exchange_bind, exchange, "test_bind_exchange");
            channel->unbindExhange(exchange_bind, exchange, "test_bind_exchange");
            channel->deleteExchange(exchange_bind);
        }

        // queue
        string queueName;
        {
            AMQP::Table queuePrps;
            queuePrps.addEntry(AMQP_FIELD_EXPIRES, 300000, AMQP_FIELD_SINGLE_ACTIVE_CONSUMER, true);

            queueName = channel->declareQueue("", false, false, false, true, &queuePrps);
            channel->bindQueue(exchange, queueName, queueName);
        }

        AMQP::MessageProps msgProps;
        msgProps.msgId = "msg_id";

        // send text
        {
            channel->basicPublish(exchange, queueName, "congratulation", &msgProps);
        }

        // send bytes
        {
            AMQP::Message msgSend;

            msgSend.len = 10;
            std::unique_ptr<char[]> ptr(new char[msgSend.len]);
            msgSend.bytes = ptr.get();

            channel->basicPublish(exchange, queueName, msgSend, &msgProps);
        }

        timeval timeOut = {30, 0};

        // easy to consume and ack
        {
            // channel->basicQos(1);

            std::string cmsTag = channel->basicConsume(queueName);

            auto envelope = channel->getMessage(timeOut);
            channel->basicAck(envelope, true);

            if (envelope.msg.bytes)
            {
                string text((char*)envelope.msg.bytes, envelope.msg.len);
                cout << text << endl;
            }

            channel->basicCancel(cmsTag);
        }

        channel->deleteExchange(exchange);
    }
    catch (const exception& e)
    {
        cout << e.what() << endl;
    }

    _CrtDumpMemoryLeaks();
    system("pause");
    return 0;
}
