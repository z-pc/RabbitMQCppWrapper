//////////////////////////////////////////////////////////////////////////
// File: Channel.h
// Description: The implement of channel
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

#ifndef AMQP_CPP_CHANNEL_H__
#define AMQP_CPP_CHANNEL_H__

#include "Connection.h"
#include "amqp.h"
#include <string>
#include <utility>

#define AMQP_FIELD_EXPIRES                "x-expires"
#define AMQP_FIELD_SINGLE_ACTIVE_CONSUMER "x-single-active-consumer"
#define AMQP_FIELD_CONNECTION_NAME        "connection_name"

namespace AMQP
{

class Channel : noncopyable
{
    friend class TCPConnection;

public:
    typedef std::shared_ptr<Channel> ptr;
    typedef std::unique_ptr<Channel> u_ptr;

    virtual ~Channel();

    AMQP::TCPConnection& connection() { return _tcpConn; };

    /**
     * Consume on queue.
     *
     * @param strQueue
     * @param consumerTag
     * @param bNoLocal
     * @param bNoAck
     * @param bExclusive
     * @param args
     * @return consume tag
     */
    virtual std::string basicConsume(const std::string& strQueue,
                                     const std::string& consumerTag = "", bool bNoLocal = false,
                                     bool bNoAck = false, bool bExclusive = false,
                                     const Table* args = nullptr);
    virtual std::string basicConsume(amqp_bytes_t strQueue, const std::string& consumerTag = "",
                                     bool bNoLocal = false, bool bNoAck = false,
                                     bool bExclusive = false, const Table* args = nullptr);

    virtual void basicCancel(const std::string& consumerTag);

    virtual AMQP::Envelope getMessage(const struct timeval& timeOut) final;

    /**
     * @brief Ack a message.
     * @param en: Evenlope to ack
     * @param multiple
     */
    virtual int basicAck(const AMQP::Envelope& en, bool multiple = false);
    virtual int basicAck(std::uint64_t deliveryTag, bool multiple = false);

    virtual int basicNAck(const AMQP::Envelope& en, bool multiple = false, bool requeue = false);
    virtual int basicNAck(std::uint64_t deliveryTag, bool multiple = false, bool requeue = false);

    virtual void basicQos(std::uint16_t perfectCount, std::uint32_t perfectSize = 0,
                          bool global = false);

    /**
     * @brief Send a message to broker, automatically connect if necessary.
     */
    virtual void basicPublish(const std::string& exchange, const std::string& routingKey,
                              const AMQP::Message& message,
                              const AMQP::MessageProps* msgProps = nullptr);
    virtual void basicPublish(const std::string& exchange, const std::string& routingKey,
                              const std::string& message,
                              const AMQP::MessageProps* msgProps = nullptr);

    /**
     * Declare an exchange.
     *
     * @param exchangeName
     * @param type
     * @param passive
     * @param durable
     * @param autoDel
     * @param internal
     * @param noWait
     * @param args
     * @return The exchange name
     */
    virtual std::string declareExchange(std::string exchangeName, std::string type = "direct",
                                        bool passive = false, bool durable = true,
                                        bool autoDel = false, bool internal = false,
                                        bool noWait = true, const AMQP::Table* args = nullptr);

    virtual void bindExchange(std::string destination, std::string source, std::string routingKey,
                              const AMQP::Table* args = nullptr);

    virtual void deleteExchange(std::string exchange, bool ifUnUsed = true, bool noWait = false);

    virtual void unbindExchange(std::string destination, std::string source, std::string routingKey,
                                const AMQP::Table* args = nullptr);

    /**
     * Declare a queue.
     *
     * @param queueName
     * @param passive
     * @param durable
     * @param exclusive
     * @param autoDelete
     * @param args
     * @return The queue name
     */
    virtual std::string declareQueue(const std::string& queueName, bool passive = false,
                                     bool durable = false, bool exclusive = false,
                                     bool autoDelete = false, const AMQP::Table* args = nullptr);

    virtual void bindQueue(const std::string& exchange, const std::string& queue,
                           const std::string& bindingkey);
    virtual void unbindQueue(const std::string& exchange, const std::string& queue,
                             const std::string& bindingkey);
    virtual void purgeQueue(const std::string& queue);
    virtual void deleteQueue(const std::string& queue, bool ifUnused = true, bool ifEmpty = true);

    /**
     * @brief Close channel. This will be implicitly called by deconstructor.
     */
    void close();

    bool isOpened() { return _tcpConn.getChannelState(_channelId) == USING; };
    std::uint16_t getId() { return _channelId; }

protected:
    Channel(AMQP::TCPConnection&, std::uint16_t id = 1);

    virtual void open(std::uint16_t id = 1);
    virtual void assertConnection();
    virtual void dumpBasicProps(const AMQP::MessageProps* pProps,
                                amqp_basic_properties_t& pAmqpProps);
    virtual void initBasicProps(amqp_basic_properties_t& pAmqpProps);
    virtual void updateChannelState(ChannelState state);

    AMQP::TCPConnection& _tcpConn;
    const std::uint16_t _channelId;
};
} // namespace AMQP

#endif // !AMQP_CPP_CHANNEL_H__
