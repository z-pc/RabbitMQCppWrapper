/**
 * @file Connection.h
 * @brief Implementation of AMQP connection functionalities.
 *
 * This file provides the implementation for establishing, managing, and interacting with AMQP connections
 * and associated data structures such as messages, envelopes, and tables. It includes support for both
 * standard and SSL connections.
 *
 * @author Le Xuan Tuan Anh
 * @copyright 2022
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed under the License is
 * distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and limitations under the License.
 */

#ifndef AMQP_CPP_CONNECTION_H__
#define AMQP_CPP_CONNECTION_H__

#include "noncopyable.h"
#include "rabbitmq-c/amqp.h"
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace AMQP
{

/**
 * @def AMQP_TIMEOUT_DEFAULT
 * @brief Default timeout value in seconds.
 */
#define AMQP_TIMEOUT_DEFAULT 45

/**
 * @def AMQP_CONSUME_MAX_PREFETCH_COUNT
 * @brief Maximum prefetch count for AMQP consumers.
 */
#define AMQP_CONSUME_MAX_PREFETCH_COUNT 65535

/**
 * @enum ChannelState
 * @brief Represents the state of a channel.
 */
enum ChannelState
{
    READY = 0, ///< Channel is ready for use.
    USING = 1, ///< Channel is in use.
    CLOSED = 2 ///< Channel is closed.
};

/**
 * @typedef Message
 * @brief Alias for `amqp_bytes_t` used to represent AMQP messages.
 */
typedef amqp_bytes_t Message;

/**
 * @struct MessageProps
 * @brief Represents the properties of a message.
 */
struct MessageProps
{
    std::string replyQueue;        ///< Reply queue name.
    std::string correlationId;     ///< Correlation ID.
    std::string contentType;       ///< Content type of the message.
    std::string msgId;             ///< Message ID.
    std::uint8_t deliveryMode = 0; ///< Delivery mode (e.g., persistent or transient).
};

/**
 * @struct Envelope
 * @brief Represents an AMQP envelope containing message and routing details.
 */
struct Envelope
{
    bool redelivered;          ///< Whether the message has been redelivered.
    std::uint16_t channelNum;  ///< Channel number.
    std::uint64_t deliveryTag; ///< Delivery tag.
    std::string consumerTag;   ///< Consumer tag.
    std::string exchange;      ///< Exchange name.
    std::string routingKey;    ///< Routing key.
    AMQP::Message msg;         ///< AMQP message.

    Envelope();
    Envelope(bool redelivered, std::uint16_t channel, std::uint64_t delivery_tag, std::string consumer_tag,
             std::string exchange, std::string routing_key, AMQP::Message msg);
    virtual ~Envelope();

    /**
     * @brief Destroy the envelope, cleaning up resources.
     *
     * @details The `destroy()` method is automatically called in the destructor of the `Envelope` class, so you do not
     * need to explicitly call it. When the `Envelope` object is destructed, this function is invoked to ensure the
     * proper cleanup of resources.
     *
     */
    void destroy();

    Envelope(Envelope&& en) noexcept;            ///< Move constructor.
    Envelope& operator=(Envelope&& en) noexcept; ///< Move assignment operator.
    Envelope(const Envelope& en);                ///< Copy constructor.
    Envelope& operator=(const Envelope& en);     ///< Copy assignment operator.
};

/**
 * @class Table
 * @brief Represents a collection of key-value entries for AMQP tables.
 */
class Table
{
public:
    typedef std::vector<amqp_table_entry_t_> Entries;

    Table();
    Table(const Table&);
    Table(Table&&) noexcept;
    virtual ~Table();

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
    void addEntry(const char* key, const T& value, const char* key2, Args&&... args);

    void removeEntry(const char* key);
    const amqp_table_entry_t_* getEntries() const;
    std::size_t getEntriesSize() const;

    Table& operator=(const Table&);
    Table& operator=(Table&&) noexcept;

    void empty();

private:
    std::shared_ptr<std::vector<amqp_table_entry_t_>> _entries;
};

/**
 * @class Connection
 * @brief Represents a TCP connection to an AMQP broker.
 */
class Connection : noncopyable
{
    friend class Channel;

public:
    typedef std::shared_ptr<Connection> ptr;                    ///< Alias for shared pointer to Connection.
    typedef std::map<std::uint16_t, ChannelState> ChannelsList; ///< Alias for list of channel states.

    /**
     * @brief Default constructor.
     */
    Connection();

    /**
     * @brief Constructor with host and port.
     *        Automatically connects to the AMQP broker.
     * @param host Hostname or IP address of the AMQP broker.
     * @param port Port number of the AMQP broker.
     */
    Connection(const std::string& host, std::uint16_t port);

    /**
     * @brief Destructor.
     *        Closes the connection if it is still open.
     */
    virtual ~Connection() noexcept;

    /**
     * @brief Connects to the AMQP server.
     *        If already connected, does nothing. Call `disconnect()` before reconnecting to a new host.
     * @param host Hostname or IP address of the AMQP broker.
     * @param port Port number of the AMQP broker.
     */
    virtual void connect(const std::string& host, std::uint16_t port);

    /**
     * @brief Logs in to the AMQP broker with the specified credentials.
     * @param vhost Virtual host on the AMQP broker.
     * @param user Username for authentication.
     * @param pass Password for authentication.
     * @param properties Optional properties for the connection.
     * @param heartBeat Heartbeat interval (in seconds).
     * @param channelMax Maximum number of channels.
     * @param frameMax Maximum frame size.
     * @param saslMethod SASL authentication method.
     */
    virtual void login(const std::string& vhost, const std::string& user, const std::string& pass,
                       const AMQP::Table* properties = nullptr, int heartBeat = 60, int channelMax = 0,
                       int frameMax = 131072, amqp_sasl_method_enum saslMethod = AMQP_SASL_METHOD_PLAIN);

    template <typename... Args>
    inline void login(const std::string& vhost, const std::string& user, const std::string& pass, int heartBeat,
                      const AMQP::Table* properties, int channelMax, int frameMax, amqp_sasl_method_enum saslMethod,
                      Args&&...);

    /**
     * @brief Disconnects from the AMQP broker.
     * @return 0 if successful, otherwise an error code.
     */
    virtual int disconnect() noexcept;

    /**
     * @brief Sets the RPC timeout for the connection.
     * @param timeOut Timeout duration (in seconds).
     */
    virtual void setRPCTimeOut(const struct timeval& timeOut);

    /**
     * @brief Creates a new channel for communication.
     *        If a specific channel number is provided but already in use, returns nullptr.
     * @param id Channel number (default is 0, which auto-generates an available channel).
     * @return Unique pointer to the created channel, or nullptr if no available channel.
     */
    virtual std::unique_ptr<Channel> createChannel(std::int32_t id = 0);

    /**
     * @brief Gets the state of a specific channel.
     * @param id Channel number.
     * @return State of the channel.
     */
    ChannelState getChannelState(std::uint16_t id);

    /**
     * @brief Finds a usable (ready) channel.
     * @return Channel number if available, otherwise 0.
     */
    std::uint16_t getReadyChannel();

    /**
     * @brief Checks if the connection is established.
     * @return True if connected, otherwise false.
     */
    bool isConnected() const { return _isConnected; };

    /**
     * @brief Checks if the connection is authenticated (logged in).
     * @return True if logged in, otherwise false.
     */
    bool isLogined() const { return _isLogined; };

    /**
     * @brief Gets the port number of the connection.
     * @return Port number.
     */
    std::uint16_t getPort() const { return _port; };

    /**
     * @brief Gets the hostname of the connection.
     * @return Hostname as a string.
     */
    std::string getHost() const { return _hostName; };

    /**
     * @brief Gets the virtual host of the connection.
     * @return Virtual host as a string.
     */
    std::string getVHost() const { return _vHost; };

    /**
     * @brief Gets the username used for login.
     * @return Username as a string.
     */
    std::string getUser() const { return _username; };

    /**
     * @brief Gets the heartbeat interval.
     * @return Heartbeat interval in seconds.
     */
    int getHeartBeat();

    /**
     * @brief Checks if there are frames enqueued for sending.
     * @return True if frames are enqueued, otherwise false.
     */
    bool framesEnqueued();

    /**
     * @brief Checks if there is data in the input buffer.
     * @return True if data exists in the buffer, otherwise false.
     */
    bool dataInBuffer();

    /**
     * @brief Gets the maximum frame size.
     * @return Maximum frame size in bytes.
     */
    int getFrameMax();

    /**
     * @brief Asserts the result of an RPC operation.
     *        Throws an exception if the operation failed.
     * @param msgThrow Error message to include in the exception.
     */
    void assertRpcReply(const std::string& msgThrow);

    /**
     * @brief Asserts the result of an RPC operation.
     * @param msgThrow Error message to include in the exception.
     * @param res Result of the RPC operation.
     */
    void assertRpcReply(const std::string& msgThrow, const amqp_rpc_reply_t& res);

protected:
    amqp_connection_state_t connnection() { return _pConn; }; ///< Gets the AMQP connection state.
    void reset();                                             ///< Resets the connection state.

    std::string _hostName;                                ///< Hostname of the AMQP broker.
    std::string _vHost;                                   ///< Virtual host.
    std::string _username;                                ///< Username for login.
    std::string _password;                                ///< Password for login.
    std::uint16_t _port{0};                               ///< Port number.
    bool _isLogined{false};                               ///< Login status.
    bool _isConnected{false};                             ///< Connection status.
    amqp_socket_t* _pSocket{nullptr};                     ///< Pointer to the AMQP socket.
    amqp_connection_state_t _pConn{nullptr};              ///< Connection state object.
    std::map<std::uint16_t, ChannelState> _channelsState; ///< Map of channel states.
};

/**
 * @class SSLConnection
 * @brief Represents a secure (SSL/TLS) connection to an AMQP broker.
 */
class SSLConnection : public Connection
{
public:
    /**
     * @brief Constructs an SSL connection.
     * @param strHost Hostname or IP address of the AMQP broker.
     * @param nPort Port number of the AMQP broker.
     */
    SSLConnection(const std::string& strHost, uint16_t nPort);

    /**
     * @brief Destructor for SSLConnection.
     */
    virtual ~SSLConnection(){};

    /**
     * @brief Establishes a secure connection to the AMQP broker.
     * @param strHost Hostname or IP address of the AMQP broker.
     * @param nPort Port number of the AMQP broker.
     */
    virtual void connect(const std::string& strHost, uint16_t nPort);
};

/**
 * @brief Create a connection to the AMQP server (prioritizing SSL, fallback to TCP if SSL is unavailable).
 *
 * This function attempts to create an SSL connection to the AMQP server. If SSL is not available, it will fall back
 * to creating a standard TCP connection.
 *
 * @param host The server address (or hostname) to connect to.
 * @param port The server port (defaults to 5672).
 * @return A smart pointer `ptr` to the created `Connection` object.
 *
 */
Connection::ptr createConnection(const std::string& host, uint16_t port = 5672);

/**
 * @brief Retrieves the version of the AMQP library.
 * @return AMQP version as an unsigned 32-bit integer.
 */
inline uint32_t getAMQPVersion() { return amqp_version_number(); };

} // namespace AMQP

template <typename... Args>
inline void AMQP::Connection::login(const std::string& strVHost, const std::string& strUser, const std::string& strPass,
                                    int heartBeat, const AMQP::Table* properties, int channelMax, int frameMax,
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

        res = amqp_login_with_properties(_pConn, _vHost.c_str(), channelMax, frameMax, heartBeat, &prps, saslMethod,
                                         _username.c_str(), _password.c_str(), std::forward<Args>(args)...);
    }
    else
    {
        res = amqp_login(_pConn, _vHost.c_str(), channelMax, frameMax, heartBeat, saslMethod, _username.c_str(),
                         _password.c_str(), std::forward<Args>(args)...);
    }

    assertRpcReply("Login failed", res);

    _isLogined = true;
}

template <typename T, typename... Args>
void AMQP::Table::addEntry(const char* key, const T& value, const char* key2, Args&&... args)
{
    addEntry(key, value);
    addEntry(key2, std::forward<Args>(args)...);
}

#endif // AMQP_CPP_CONNECTION_H__
