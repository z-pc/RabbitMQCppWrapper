//////////////////////////////////////////////////////////////////////////
// File: Connection.cpp
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

#include "Connection.h"
#include "Channel.h"
#include "Exception.h"
#include <amqp_tcp_socket.h>
#include <limits>

using namespace AMQP;

TCPConnection::TCPConnection() {}

AMQP::TCPConnection::TCPConnection(const std::string& host, uint16_t port) : TCPConnection()
{
    connect(host, port);
}

AMQP::TCPConnection::~TCPConnection()
{ /*if (isConnected()) */
    disconnect();
}

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

AMQP::Channel::u_ptr AMQP::TCPConnection::createChannel(int32_t id)
{
    if (!isLogined()) throw AMQP::Exception("The connection is not existed.");
    // if specify channel number, but it is being used, does nothings.
    if (id > 0 && (getChannelState(id) == ChannelState::USING))
        throw AMQP::Exception("The channel is using.");
    // if get default arguments, get next ready channel
    if (id <= 0) id = getReadyChannel();
    if (id > 0) return std::unique_ptr<Channel>(new Channel(*this, (std::uint16_t)id));
    return nullptr;
}

ChannelState AMQP::TCPConnection::getChannelState(std::uint16_t id)
{
    auto resFound = _channelsState.find(id);
    if (resFound == _channelsState.end()) return ChannelState::READY;
    return resFound->second;
}

std::uint16_t AMQP::TCPConnection::getReadyChannel()
{
    constexpr auto maxNum = (std::numeric_limits<std::uint16_t>::max)();
    std::uint16_t channel = 1;
    ChannelsList::const_iterator found = _channelsState.find(channel);

    while (found != _channelsState.end())
    {
        if (found->second == ChannelState::READY || found->second == ChannelState::CLOSED) break;
        if (channel == maxNum) return 0;
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


AMQP::Table::Table() { _entries = std::make_shared<std::vector<amqp_table_entry_t_>>(); }

AMQP::Table::Table(Table&& other) noexcept { *this = std::move(other); }

AMQP::Table::Table(const Table& other) { *this = other; }

Table& AMQP::Table::operator=(Table&& other) noexcept
{
    if (this != &other)
    {
        empty();
        this->_entries = other._entries;
        other._entries = std::make_shared<Entries>();
    }
    return *this;
}

void AMQP::Table::empty()
{
    if (_entries && _entries->size())
    {
        for (auto it = _entries->begin(); it != _entries->end(); it++)
        {
            if (it->value.kind == AMQP_FIELD_KIND_BYTES) amqp_bytes_free(it->value.value.bytes);
            amqp_bytes_free(it->key);
        }
    }
    _entries = std::make_shared<Entries>();
}

Table& AMQP::Table::operator=(const Table& other)
{

    if (this != &other)
    {
        empty();

        for (auto it = other._entries->begin(); it != other._entries->end(); it++)
        {
            switch (it->value.kind)
            {
            case AMQP_FIELD_KIND_BYTES:
            {
                addEntry((const char*)it->key.bytes, (const char*)it->value.value.bytes.bytes);
            }
            break;
            case AMQP_FIELD_KIND_BOOLEAN:
            {
                addEntry((const char*)it->key.bytes, it->value.value.boolean);
            }
            break;
            case AMQP_FIELD_KIND_F32:
            {
                addEntry((const char*)it->key.bytes, it->value.value.f32);
            }
            break;
            case AMQP_FIELD_KIND_F64:
            {
                addEntry((const char*)it->key.bytes, it->value.value.f64);
            }
            break;
            case AMQP_FIELD_KIND_I8:
            {
                addEntry((const char*)it->key.bytes, it->value.value.i8);
            }
            break;
            case AMQP_FIELD_KIND_U8:
            {
                addEntry((const char*)it->key.bytes, it->value.value.u8);
            }
            break;
            case AMQP_FIELD_KIND_I16:
            {
                addEntry((const char*)it->key.bytes, it->value.value.i16);
            }
            break;
            case AMQP_FIELD_KIND_U16:
            {
                addEntry((const char*)it->key.bytes, it->value.value.u16);
            }
            break;
            case AMQP_FIELD_KIND_I32:
            {
                addEntry((const char*)it->key.bytes, it->value.value.i32);
            }
            break;
            case AMQP_FIELD_KIND_U32:
            {
                addEntry((const char*)it->key.bytes, it->value.value.u32);
            }
            break;
            default:
                break;
            }
        }
    }
    return *this;
}

void AMQP::Table::addEntry(const char* key, const char* value)
{
    if (_entries == nullptr) return;

    amqp_table_entry_t entry;

    entry.key = amqp_bytes_malloc_dup(amqp_cstring_bytes(key));
    entry.value.kind = AMQP_FIELD_KIND_BYTES;
    entry.value.value.bytes = amqp_bytes_malloc_dup(amqp_cstring_bytes(value));

    _entries->push_back(entry);
}

void AMQP::Table::addEntry(const char* key, bool value)
{
    if (_entries == nullptr) return;

    amqp_table_entry_t entry;

    entry.key = amqp_bytes_malloc_dup(amqp_cstring_bytes(key));
    entry.value.kind = AMQP_FIELD_KIND_BOOLEAN;
    entry.value.value.boolean = (amqp_boolean_t)value;

    _entries->push_back(entry);
}

void AMQP::Table::addEntry(const char* key, float value)
{
    if (_entries == nullptr) return;

    amqp_table_entry_t entry;

    entry.key = amqp_bytes_malloc_dup(amqp_cstring_bytes(key));
    entry.value.kind = AMQP_FIELD_KIND_F32;
    entry.value.value.f32 = value;

    _entries->push_back(entry);
}

void AMQP::Table::addEntry(const char* key, double value)
{
    if (_entries == nullptr) return;

    amqp_table_entry_t entry;

    entry.key = amqp_bytes_malloc_dup(amqp_cstring_bytes(key));
    entry.value.kind = AMQP_FIELD_KIND_F64;
    entry.value.value.f64 = value;

    _entries->push_back(entry);
}

void AMQP::Table::addEntry(const char* key, std::int8_t value)
{
    if (_entries == nullptr) return;

    amqp_table_entry_t entry;

    entry.key = amqp_bytes_malloc_dup(amqp_cstring_bytes(key));
    entry.value.kind = AMQP_FIELD_KIND_I8;
    entry.value.value.i8 = value;

    _entries->push_back(entry);
}

void AMQP::Table::addEntry(const char* key, std::uint8_t value)
{
    if (_entries == nullptr) return;

    amqp_table_entry_t entry;

    entry.key = amqp_bytes_malloc_dup(amqp_cstring_bytes(key));
    entry.value.kind = AMQP_FIELD_KIND_U8;
    entry.value.value.u8 = value;

    _entries->push_back(entry);
}

void AMQP::Table::addEntry(const char* key, std::int16_t value)
{
    if (_entries == nullptr) return;

    amqp_table_entry_t entry;

    entry.key = amqp_bytes_malloc_dup(amqp_cstring_bytes(key));
    entry.value.kind = AMQP_FIELD_KIND_I16;
    entry.value.value.i16 = value;

    _entries->push_back(entry);
}

void AMQP::Table::addEntry(const char* key, std::uint16_t value)
{
    if (_entries == nullptr) return;

    amqp_table_entry_t entry;

    entry.key = amqp_bytes_malloc_dup(amqp_cstring_bytes(key));
    entry.value.kind = AMQP_FIELD_KIND_U16;
    entry.value.value.u16 = value;

    _entries->push_back(entry);
}

void AMQP::Table::addEntry(const char* key, std::int32_t value)
{
    if (_entries == nullptr) return;

    amqp_table_entry_t entry;

    entry.key = amqp_bytes_malloc_dup(amqp_cstring_bytes(key));
    entry.value.kind = AMQP_FIELD_KIND_I32;
    entry.value.value.i32 = value;

    _entries->push_back(entry);
}

void AMQP::Table::addEntry(const char* key, std::uint32_t value)
{
    if (_entries == nullptr) return;

    amqp_table_entry_t entry;

    entry.key = amqp_bytes_malloc_dup(amqp_cstring_bytes(key));
    entry.value.kind = AMQP_FIELD_KIND_U32;
    entry.value.value.u32 = value;

    _entries->push_back(entry);
}

void AMQP::Table::removeEntry(const char* key)
{
    if (_entries && _entries->size())
    {
        for (auto it = _entries->begin(); it != _entries->end(); it++)
        {
            if (std::string((char*)it->key.bytes, it->key.len) == key)
            {
                if (it->value.kind == AMQP_FIELD_KIND_BYTES) amqp_bytes_free(it->value.value.bytes);
                amqp_bytes_free(it->key);

                _entries->erase(it);
                break;
            }
        }
    }
}