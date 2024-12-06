/**
 * @file Channel.h
 * @brief The implementation of the AMQP channel class, which handles
 * message consumption, publishing, and managing exchanges and queues.
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

#ifndef AMQP_CPP_CHANNEL_H__
#define AMQP_CPP_CHANNEL_H__

#include "Connection.h"
#include "rabbitmq-c/amqp.h"
#include <string>
#include <utility>

#define AMQP_FIELD_EXPIRES                "x-expires"
#define AMQP_FIELD_SINGLE_ACTIVE_CONSUMER "x-single-active-consumer"
#define AMQP_FIELD_CONNECTION_NAME        "connection_name"

namespace AMQP
{
/**
 * @class Channel
 * @brief Represents a communication channel with the AMQP broker.
 *
 * The Channel class provides methods for interacting with a broker's channel, such as declaring exchanges,
 * queues, publishing messages, and consuming messages. A channel is a virtual connection within an AMQP connection.
 */
class Channel : noncopyable
{
    friend class Connection;

public:
    /**
     * @typedef ptr
     * @brief Alias for a shared pointer to a Channel.
     */
    typedef std::shared_ptr<Channel> ptr;

    /**
     * @typedef u_ptr
     * @brief Alias for a unique pointer to a Channel.
     */
    typedef std::unique_ptr<Channel> u_ptr;

    /**
     * @brief Destructor.
     *
     * Closes the channel when it is destroyed.
     */
    virtual ~Channel();

    /**
     * @brief Gets the connection associated with this channel.
     * @return A reference to the associated AMQP connection.
     */
    AMQP::Connection& connection() { return _tcpConn; };

    /**
     * @brief Consumes messages from the specified queue.
     *
     * @param strQueue The name of the queue to consume from.
     * @param consumerTag The consumer tag to associate with this consumer.
     * @param bNoLocal Specifies whether to disable local consumption.
     * @param bNoAck Specifies whether to disable acknowledgments.
     * @param bExclusive Specifies whether the consumer should be exclusive.
     * @param args Optional arguments to pass with the consume request.
     * @return The consumer tag.
     */
    virtual std::string basicConsume(const std::string& strQueue, const std::string& consumerTag = "",
                                     bool bNoLocal = false, bool bNoAck = false, bool bExclusive = false,
                                     const Table* args = nullptr);

    /**
     * @brief Consumes messages from the specified queue using amqp_bytes_t for queue name.
     *
     * @param strQueue The queue name as amqp_bytes_t.
     * @param consumerTag The consumer tag to associate with this consumer.
     * @param bNoLocal Specifies whether to disable local consumption.
     * @param bNoAck Specifies whether to disable acknowledgments.
     * @param bExclusive Specifies whether the consumer should be exclusive.
     * @param args Optional arguments to pass with the consume request.
     * @return The consumer tag.
     */
    virtual std::string basicConsume(amqp_bytes_t strQueue, const std::string& consumerTag = "", bool bNoLocal = false,
                                     bool bNoAck = false, bool bExclusive = false, const Table* args = nullptr);

    /**
     * @brief Cancels a consumer.
     * @param consumerTag The tag of the consumer to cancel.
     */
    virtual void basicCancel(const std::string& consumerTag);

    /**
     * @brief Gets a message from the queue.
     * @param timeOut The timeout for waiting for a message.
     * @return The received message in an envelope.
     */
    virtual AMQP::Envelope getMessage(const struct timeval& timeOut) final;

    /**
     * @brief Acknowledges the receipt of a message.
     * @param en The envelope of the message to acknowledge.
     * @param multiple Whether to acknowledge multiple messages.
     * @return The result code of the acknowledgment.
     */
    virtual int basicAck(const AMQP::Envelope& en, bool multiple = false);

    /**
     * @brief Acknowledges a message by its delivery tag.
     * @param deliveryTag The delivery tag of the message to acknowledge.
     * @param multiple Whether to acknowledge multiple messages.
     * @return The result code of the acknowledgment.
     */
    virtual int basicAck(std::uint64_t deliveryTag, bool multiple = false);

    /**
     * @brief Negative acknowledgment of a message.
     *
     * This function is used to send a negative acknowledgment (nack) to the broker, indicating that a message was not
     * successfully processed and may be requeued or discarded. The message is identified by the provided envelope.
     *
     * @param en The envelope of the message to be negatively acknowledged.
     * @param multiple If set to `true`, it acknowledges all messages up to and including the provided delivery tag. If
     *                 set to `false`, only the specified message is negatively acknowledged.
     * @param requeue If set to `true`, the message will be requeued for further processing by another consumer. If set
     *                to `false`, the message will be discarded.
     *
     * @return Returns a status code indicating the result of the operation.
     * @see basicNAck(std::uint64_t deliveryTag, bool, bool)
     */
    virtual int basicNAck(const AMQP::Envelope& en, bool multiple = false, bool requeue = false);

    /**
     * @brief Negative acknowledgment of a message.
     *
     * This function is used to send a negative acknowledgment (nack) to the broker using a delivery tag, indicating
     * that the message was not successfully processed and may be requeued or discarded.
     *
     * @param deliveryTag The delivery tag identifying the message to be negatively acknowledged.
     * @param multiple If set to `true`, it acknowledges all messages up to and including the provided delivery tag. If
     *                 set to `false`, only the specified message is negatively acknowledged.
     * @param requeue If set to `true`, the message will be requeued for further processing by another consumer. If set
     *                to `false`, the message will be discarded.
     *
     * @return Returns a status code indicating the result of the operation.
     * @see basicNAck(const AMQP::Envelope&, bool, bool)
     */
    virtual int basicNAck(std::uint64_t deliveryTag, bool multiple = false, bool requeue = false);

    /**
     * @brief Rejects a message.
     * @param deliveryTag The delivery tag of the message to reject.
     * @param requeue Whether to requeue the rejected message.
     * @return The result code of the rejection.
     */
    virtual int basicReject(std::uint64_t deliveryTag, bool requeue);

    /**
     * @brief Set Quality of Service (QoS) parameters for the channel.
     *
     * The `basicQos` function is used to specify the QoS settings for the channel. This controls how messages are
     * delivered to the consumer. Specifically, it allows setting the maximum number of messages or the maximum size of
     * messages that can be sent over the channel before an acknowledgment is received.
     *
     * @param perfectCount The maximum number of unacknowledged messages that can be sent to the consumer. If the number
     * of unacknowledged messages exceeds this value, the broker will stop sending more messages until an acknowledgment
     * is received.
     * @param perfectSize The maximum size (in bytes) of messages that can be sent to the consumer. The broker will stop
     *                    sending messages if the total size of unacknowledged messages exceeds this value.
     * @param global If set to `true`, the QoS setting applies to all consumers on the channel. If set to `false`, the
     * QoS settings apply only to the current consumer.
     *
     * @note The `perfectCount` parameter should generally be set to a number that balances between message delivery
     * rate and the consumer's ability to process messages. Setting `perfectSize` to zero means there is no limit on the
     *       size of messages.
     */
    virtual void basicQos(std::uint16_t perfectCount, std::uint32_t perfectSize = 0, bool global = false);

    /**
     * @brief Publish a message to an exchange.
     *
     * This function sends a message to the specified exchange with a routing key. The message is represented by an
     * `AMQP::Message` object, which can contain the message body and properties. The message properties are optional
     * and can be provided via `msgProps`.
     *
     * @param exchange The name of the exchange to which the message should be published.
     * @param routingKey The routing key used to route the message to the appropriate queue(s).
     * @param message The message to be sent, encapsulated in an `AMQP::Message` object.
     * @param msgProps Optional message properties, such as headers or delivery modes, that will be applied to the
     * message. If not provided, default properties will be used.
     *
     * @return void
     *
     * @see AMQP::Message, AMQP::MessageProps
     */
    virtual void basicPublish(const std::string& exchange, const std::string& routingKey, const AMQP::Message& message,
                              const AMQP::MessageProps* msgProps = nullptr);

    /**
     * @brief Publish a message to an exchange.
     *
     * This function sends a message to the specified exchange with a routing key. The message is represented by a
     * string, which will be converted to the appropriate message format. Message properties are optional and can
     * be provided via `msgProps`.
     *
     * @param exchange The name of the exchange to which the message should be published.
     * @param routingKey The routing key used to route the message to the appropriate queue(s).
     * @param message The message to be sent, provided as a string.
     * @param msgProps Optional message properties, such as headers or delivery modes, that will be applied to the
     * message. If not provided, default properties will be used.
     *
     * @return void
     *
     * @see AMQP::MessageProps
     */
    virtual void basicPublish(const std::string& exchange, const std::string& routingKey, const std::string& message,
                              const AMQP::MessageProps* msgProps = nullptr);

    /**
     * @brief Declares a queue.
     *
     * @param queueName The name of the queue.
     * @param passive Specifies whether to check if the queue exists.
     * @param durable Specifies whether the queue should be durable.
     * @param exclusive Specifies whether the queue should be exclusive.
     * @param autoDelete Specifies whether the queue should auto-delete.
     * @param args Optional arguments for queue declaration.
     * @return The name of the declared queue.
     */
    virtual std::string declareQueue(const std::string& queueName, bool passive = false, bool durable = false,
                                     bool exclusive = false, bool autoDelete = false,
                                     const AMQP::Table* args = nullptr);

    /**
     * @brief Declares an exchange.
     * @param exchangeName The name of the exchange.
     * @param type The type of the exchange (e.g., "direct", "fanout", etc.).
     * @param passive Specifies whether to check if the exchange exists.
     * @param durable Specifies whether the exchange should be durable.
     * @param autoDel Specifies whether the exchange should auto-delete.
     * @param internal Specifies whether the exchange is internal.
     * @param noWait Whether to wait for confirmation.
     * @param args Optional arguments for the exchange declaration.
     * @return The name of the declared exchange.
     */
    virtual std::string declareExchange(std::string exchangeName, std::string type = "direct", bool passive = false,
                                        bool durable = true, bool autoDel = false, bool internal = false,
                                        bool noWait = true, const AMQP::Table* args = nullptr);

    /**
     * @brief Binds an exchange to a queue.
     * @param destination The destination queue to bind.
     * @param source The source exchange to bind from.
     * @param routingKey The routing key to use.
     * @param args Optional arguments for the binding.
     */
    virtual void bindExchange(std::string destination, std::string source, std::string routingKey,
                              const AMQP::Table* args = nullptr);

    /**
     * @brief Unbinds an exchange from a queue.
     * @param destination The destination queue to unbind.
     * @param source The source exchange to unbind from.
     * @param routingKey The routing key to use.
     * @param args Optional arguments for the unbinding.
     */
    virtual void unbindExchange(std::string destination, std::string source, std::string routingKey,
                                const AMQP::Table* args = nullptr);

    /**
     * @brief Deletes an exchange.
     * @param exchange The name of the exchange to delete.
     * @param ifUnUsed Whether to delete only if the exchange is unused.
     * @param noWait Whether to wait for confirmation.
     */
    virtual void deleteExchange(std::string exchange, bool ifUnUsed = true, bool noWait = false);

    /**
     * @brief Binds a queue to an exchange.
     * @param exchange The exchange to bind to.
     * @param queue The queue to bind.
     * @param bindingkey The binding key to use.
     */
    virtual void bindQueue(const std::string& exchange, const std::string& queue, const std::string& bindingkey);

    /**
     * @brief Unbinds a queue from an exchange.
     * @param exchange The exchange to unbind from.
     * @param queue The queue to unbind.
     * @param bindingkey The binding key to use.
     */
    virtual void unbindQueue(const std::string& exchange, const std::string& queue, const std::string& bindingkey);

    /**
     * @brief Purges a queue, removing all messages.
     * @param queue The queue to purge.
     */
    virtual void purgeQueue(const std::string& queue);

    /**
     * @brief Deletes a queue.
     * @param queue The name of the queue to delete.
     * @param ifUnused Whether to delete the queue if unused.
     * @param ifEmpty Whether to delete the queue if empty.
     */
    virtual void deleteQueue(const std::string& queue, bool ifUnused = true, bool ifEmpty = true);

    /**
     * @brief Closes the channel.
     *        This is called implicitly by the destructor.
     */
    void close();

    /**
     * @brief Checks if the channel is open.
     * @return True if the channel is open, false otherwise.
     */
    bool isOpened() { return _tcpConn.getChannelState(_channelId) == USING; };

    /**
     * @brief Gets the ID of the channel.
     * @return The ID of the channel.
     */
    std::uint16_t getId() { return _channelId; }

protected:
    /**
     * @brief Constructor.
     * @param conn The connection associated with the channel.
     * @param id The channel ID (default is 1).
     */
    Channel(AMQP::Connection&, std::uint16_t id = 1);

    /**
     * @brief Opens the channel.
     * @param id The channel ID.
     */
    virtual void open(std::uint16_t id = 1);

    /**
     * @brief Asserts that the connection is valid.
     */
    virtual void assertConnection();

    /**
     * @brief Dumps the basic properties of a message.
     * @param pProps The properties to dump.
     * @param pAmqpProps The AMQP properties to update.
     */
    virtual void dumpBasicProps(const AMQP::MessageProps* pProps, amqp_basic_properties_t& pAmqpProps);

    /**
     * @brief Initializes the basic properties of a message.
     * @param pAmqpProps The AMQP properties to initialize.
     */
    virtual void initBasicProps(amqp_basic_properties_t& pAmqpProps);

    /**
     * @brief Updates the state of the channel.
     * @param state The new state of the channel.
     */
    virtual void updateChannelState(ChannelState state);

private:
    AMQP::Connection& _tcpConn;     /**< The connection associated with this channel. */
    const std::uint16_t _channelId; /**< The unique ID of the channel. */
};
} // namespace AMQP

#endif // !AMQP_CPP_CHANNEL_H__
