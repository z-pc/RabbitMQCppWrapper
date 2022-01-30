#include "Connection.h"
#include "Channel.h"
#include "Exception.h"
#include <limits>
#include <rabbitmq-c/tcp_socket.h>

using namespace AMQP;

TCPConnection::TCPConnection()
{
    _hostName = "";
    _vHost = "";
    _username = "";
    _password = "";
    _port = 0;

    _pSocket = nullptr;
    _pConn = nullptr;

    _isLogined = false;
    _isConnected = false;
}

AMQP::TCPConnection::TCPConnection(const std::string& host, uint16_t port) : TCPConnection()
{
    connect(host, port);
}

AMQP::TCPConnection::~TCPConnection() { /*if (isConnected()) */ disconnect(); }

void AMQP::TCPConnection::connect(const std::string& host, uint16_t port)
{
    if (isConnected()) return;

    _hostName = host;
    _port = port;

    // create and open socket
    {
        _pConn = amqp_new_connection();
        _pSocket = amqp_tcp_socket_new(_pConn);
        if (!_pSocket)
        {
            throw Exception("Connect to host failed", AMQP_STATUS_SOCKET_ERROR);
        }
        int status = amqp_socket_open(_pSocket, _hostName.c_str(), _port);
        if (status)
        {
            throw Exception("Connect to host failed", AMQP_STATUS_SOCKET_ERROR);
        }
    }
    _isConnected = true;
}

void AMQP::TCPConnection::login(const std::string& strVHost, const std::string& strUser,
                                const std::string& strPass, const Table* properties /*= nullptr*/,
                                int heartBeat /*= 60*/, int channelMax /*= 0*/,
                                int frameMax /*= 131072*/,
                                amqp_sasl_method_enum saslMethod /*= AMQP_SASL_METHOD_PLAIN*/)
{
    login(strVHost, strUser, strPass, heartBeat, properties, channelMax, frameMax, saslMethod);
}

void AMQP::TCPConnection::disconnect() noexcept
{
    amqp_connection_close(_pConn, AMQP_REPLY_SUCCESS);
    int r = amqp_destroy_connection(_pConn);
    _isConnected = false;
}

void AMQP::TCPConnection::setRPCTimeOut(const timeval& timeOut)
{
    if (isConnected()) amqp_set_rpc_timeout(_pConn, &timeOut);
}

int AMQP::TCPConnection::getHeartBeat() { return amqp_get_heartbeat(_pConn); }

std::shared_ptr<TCPConnection> AMQP::TCPConnection::createConnection(const std::string& host,
                                                                     std::uint16_t port)
{
    return std::make_shared<TCPConnection>(host, port);
}

AMQP::Channel::u_ptr AMQP::TCPConnection::createChannel(int32_t channel)
{
    if (isLogined())
    {
        // if specify channel number, but it is being used, does nothings.
        if (channel > 0 && (getChannelState(channel) == ChannelState::USING)) return nullptr;

        // if get default arguments, get next ready channel
        if (channel <= 0) channel = getReadyChannel();

        if (channel > 0)
            return std::unique_ptr<Channel>(new Channel(*this, (std::uint16_t)channel));
    }

    return nullptr;
}

ChannelState AMQP::TCPConnection::getChannelState(std::uint16_t channel)
{
    auto& resFound = _channelsState.find(channel);

    if (resFound == _channelsState.end()) return ChannelState::READY;

    return resFound->second;
}

std::uint16_t AMQP::TCPConnection::getReadyChannel()
{
    auto maxNum = (std::numeric_limits<std::uint16_t>::max)();
    std::uint16_t channel = 1;

    ChannelsList::const_iterator found = _channelsState.find(channel);

    while (found != _channelsState.end())
    {
        if (found->second == ChannelState::READY || found->second == ChannelState::CLOSED) break;

        if (channel == maxNum) return 0;
        // throw AMQP::Exception("The number of channels has reached the limit.");

        found = _channelsState.find(++channel);
    }

    return channel;
}

void AMQP::TCPConnection::assertRpcReply(const std::string& msgThrow)
{
    auto res = amqp_get_rpc_reply(_pConn);
    if (res.reply_type != AMQP_RESPONSE_NORMAL) throw Exception(msgThrow, res);
}

void AMQP::TCPConnection::assertRpcReply(const std::string& msgThrow, const amqp_rpc_reply_t& res)
{
    if (res.reply_type != AMQP_RESPONSE_NORMAL) throw Exception(msgThrow, res);
}
