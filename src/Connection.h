//////////////////////////////////////////////////////////////////////////
// File: Connection.h
// Description: The implement of amqp connection
// Author: Le Xuan Tuan Anh
//
// Copyright 2022 Le Xuan Tuan Anh
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//////////////////////////////////////////////////////////////////////////

#ifndef AMQP_CPP_CONNECTION_H__
#define AMQP_CPP_CONNECTION_H__

#include "amqp.h"
#include "noncopyable.h"
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace AMQP
{
#define AMQP_TIMEOUT_DEFAULT            45
#define AMQP_CONSUME_MAX_PREFETCH_COUNT 65535

enum ChannelState
{
    READY = 0,
    USING = 1,
    CLOSED = 2
};

typedef amqp_bytes_t Message;

struct MessageProps
{
    std::string replyQueue;
    std::string correlationId;
    std::string contentType;
    std::string msgId;
    std::uint8_t deliveryMode;
    MessageProps()
    {
        replyQueue = "";
        correlationId = "";
        contentType = "";
        msgId = "";
        deliveryMode = 0;
    }
};

// Envelope
// The copy constructor or assignment operator is responsible for copying data.
struct Envelope
{
    bool redelivered;
    std::uint16_t channelNum;
    std::uint64_t deliveryTag;
    std::string consumerTag;
    std::string exchange;
    std::string routingKey;
    AMQP::Message msg;

    Envelope()
    {
        redelivered = false;
        channelNum = 0;
        deliveryTag = 0;
        consumerTag = "";
        exchange = "";
        routingKey = "";
        msg.len = 0;
        msg.bytes = nullptr;
    }

    Envelope(bool redelivered, std::uint16_t channel, std::uint64_t delivery_tag,
             std::string consumer_tag, std::string exchange, std::string routing_key,
             AMQP::Message msg)
    {
        this->redelivered = redelivered;
        this->channelNum = channel;
        this->deliveryTag = delivery_tag;
        this->consumerTag = consumer_tag;
        this->exchange = exchange;
        this->routingKey = routing_key;
        this->msg = msg;
    }

    virtual ~Envelope() { destroy(); }

    void destroy()
    {
        if (msg.bytes)
        {
            delete[] msg.bytes;
            msg.bytes = nullptr;
            msg.len = 0;
        }
    }

    Envelope(Envelope&& en) noexcept { *this = std::move(en); }
    Envelope& operator=(Envelope&& en) noexcept
    {
        if (this != &en)
        {
            redelivered = std::move(en.redelivered);
            channelNum = std::move(en.channelNum);
            deliveryTag = std::move(en.deliveryTag);
            consumerTag = std::move(en.consumerTag);
            exchange = std::move(en.exchange);
            routingKey = std::move(en.routingKey);
            msg.len = std::move(en.msg.len);
            msg.bytes = std::move(en.msg.bytes);

            en.msg.len = 0;
            en.msg.bytes = nullptr;
        }

        return *this;
    }

    Envelope(const Envelope& en) { *this = en; };
    Envelope& operator=(const Envelope& en)
    {
        if (this != &en)
        {
            redelivered = en.redelivered;
            channelNum = en.channelNum;
            deliveryTag = en.deliveryTag;
            consumerTag = en.consumerTag;
            exchange = en.exchange;
            routingKey = en.routingKey;
            msg.len = en.msg.len;

            msg.bytes = new char[msg.len];
            memcpy(msg.bytes, en.msg.bytes, msg.len);
        }

        return *this;
    }
};

// Envelope
// The copy constructor or assignment operator is responsible for copying data.
class Table
{
public:
    typedef std::vector<amqp_table_entry_t_> Entries;

    Table();
    Table(const Table&);
    Table(Table&&) noexcept;
    virtual ~Table() { empty(); };

    void addEntry(const char* key, const char* value);
    void addEntry(const char* key, bool value);
    void addEntry(const char* key, float value);
    void addEntry(const char* key, double value);
    void addEntry(const char* key, std::int8_t value);
    void addEntry(const char* key, std::uint8_t value);
    void addEntry(const char* key, std::int16_t value);
    void addEntry(const char* key, std::uint16_t value);
    void addEntry(const char* key, std::int32_t value);
    void addEntry(const char* key, std::uint32_t value);
    template <typename T, typename... Args>
    void addEntry(const char* key, const T& value, const char* key2, Args&&... args)
    {
        addEntry(key, value);
        addEntry(key2, std::forward<Args>(args)...);
    }
    void removeEntry(const char* key);

    const amqp_table_entry_t_* getEntries() const
    {
        if (_entries) return _entries->data();
        return nullptr;
    }
    std::size_t getEntriesSize() const
    {
        if (_entries) return _entries->size();
        return 0;
    }

    Table& operator=(const Table&);
    Table& operator=(Table&&) noexcept;

    void empty();

private:
    std::shared_ptr<std::vector<amqp_table_entry_t_>> _entries;
};

/**
 * @brief TCP Connection
 */
class TCPConnection : noncopyable
{
    friend class Channel;

public:
    typedef std::shared_ptr<TCPConnection> ptr;
    typedef std::map<std::uint16_t, ChannelState> ChannelsList;

    /**
     * @brief Default contructor
     */
    TCPConnection();

    /**
     * @brief A contructor with host and port, this will also connect to the server
     */
    TCPConnection(const std::string& host, std::uint16_t port);
    virtual ~TCPConnection() noexcept;

    /**
     * @brief Connect to amqp server with host and port, if was connected, do nothing.
     * Call explicitly disconnect(...) before change host.
     */
    virtual void connect(const std::string& host, std::uint16_t port);

    /**
     * @brief Login to broker
     */
    virtual void login(const std::string& vhost, const std::string& user, const std::string& pass,
                       const AMQP::Table* properties = nullptr, int heartBeat = 60,
                       int channelMax = 0, int frameMax = 131072,
                       amqp_sasl_method_enum saslMethod = AMQP_SASL_METHOD_PLAIN);
    template <typename... Args>
    inline void login(const std::string& vhost, const std::string& user, const std::string& pass,
                      int heartBeat, const AMQP::Table* properties, int channelMax, int frameMax,
                      amqp_sasl_method_enum saslMethod, Args&&...);

    /**
     * @brief Close connection, the function will be implicitly called by deconstructor.
     */
    virtual int disconnect() noexcept;

    /**
     * @brief Set rpc timeout for connection
     * @param timeOut
     */
    virtual void setRPCTimeOut(const struct timeval& timeOut);

    /**
     * @brief Create a channel. If specify channel number, but it is being used,
     * does nothings and return null, otherwise. If passing channel number <=0,
     * this function will generate the channel number for ready or closed.
     * @param channel: the number of channel
     */
    virtual std::unique_ptr<Channel> createChannel(std::int32_t id = 0);

    /**
     * @brief Get channel state
     * @param channel
     * @return
     */
    ChannelState getChannelState(std::uint16_t id);

    /**
     * @brief Looking for a usable channel, if not found, return 0
     */
    std::uint16_t getReadyChannel();

    bool isConnected() const { return _isConnected; };
    bool isLogined() const { return _isLogined; };
    std::uint16_t getPort() const { return _port; };
    std::string getHost() const { return _hostName; };
    std::string getVHost() const { return _hostName; };
    std::string getUser() const { return _username; };
    int getHeartBeat();
    bool framesEnqueued();
    bool dataInBuffer();
    int getFrameMax();

    /**
     * @brief Create and connect
     */
    static std::shared_ptr<TCPConnection> createConnection(const std::string& hostname,
                                                           std::uint16_t port);

    template <typename... Args>
    static std::shared_ptr<TCPConnection> createConnection(Args&&... args)
    {
        return std::make_shared<TCPConnection>(std::forward<Args>(args)...);
    }

    /**
     * @brief Assert rpc reply, if happend, throw an exception.
     */
    void assertRpcReply(const std::string& msgThrow);
    void assertRpcReply(const std::string& msgThrow, const amqp_rpc_reply_t& res);

protected:
    amqp_connection_state_t connnection() { return _pConn; };
    void reset();

    std::string _hostName;
    std::string _vHost;
    std::string _username;
    std::string _password;
    std::uint16_t _port{0};
    bool _isLogined{false};
    bool _isConnected{false};
    amqp_socket_t* _pSocket{nullptr};
    amqp_connection_state_t _pConn{nullptr};
    std::map<std::uint16_t, ChannelState> _channelsState;
};

template <typename... Args>
inline void AMQP::TCPConnection::login(const std::string& strVHost, const std::string& strUser,
                                       const std::string& strPass, int heartBeat,
                                       const AMQP::Table* properties, int channelMax, int frameMax,
                                       amqp_sasl_method_enum saslMethod, Args&&... args)
{
    if (!isConnected()) return;

    _vHost = strVHost;
    _username = strUser;
    _password = strPass;

    amqp_rpc_reply_t res;

    if (properties && properties->getEntriesSize() > 0)
    {
        amqp_table_t_ prps;
        prps.entries = (amqp_table_entry_t_*)properties->getEntries();
        prps.num_entries = (int)properties->getEntriesSize();

        res = amqp_login_with_properties(_pConn, _vHost.c_str(), channelMax, frameMax, heartBeat,
                                         &prps, saslMethod, _username.c_str(), _password.c_str(),
                                         std::forward<Args>(args)...);
    }
    else
    {
        res = amqp_login(_pConn, _vHost.c_str(), channelMax, frameMax, heartBeat, saslMethod,
                         _username.c_str(), _password.c_str(), std::forward<Args>(args)...);
    }

    assertRpcReply("Login failed", res);

    _isLogined = true;
}

// In plainning
class SSLConnection : public TCPConnection
{
public:
    SSLConnection(){};
    virtual ~SSLConnection(){};

private:
};

inline uint32_t getAMQPVersion() { return amqp_version_number(); };

} // namespace AMQP

#endif // !AMQP_CPP_CONNECTION_H__
