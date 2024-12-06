#include "../src/Channel.h"
#include "../src/Connection.h"
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

using namespace std;

int main(void)
{
    std::string host = "localhost";
    int port = 5672;
    string user = "user";
    string pass = "pass";
    string vhost = "/";

    try
    {
        AMQP::Connection::ptr cnn = AMQP::createConnection(host, port);
        cnn->login(vhost, user, pass);
    }
    catch (const exception& e)
    {
        cout << e.what() << endl;
    }

    system("pause");
    return 0;
}
