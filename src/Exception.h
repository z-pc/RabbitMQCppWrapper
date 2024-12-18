//////////////////////////////////////////////////////////////////////////
// File: Exception.h
// Description: The implement of exception
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

#ifndef AMQP_CPP_EXCEPTION_H__
#define AMQP_CPP_EXCEPTION_H__

#include "rabbitmq-c/amqp.h"
#include <exception>
#include <string>

namespace AMQP
{
class Exception : public std::exception
{
public:
    Exception();
    Exception(const std::string& message, const amqp_rpc_reply_t& res);
    Exception(const std::string& message, int errorCode = -1);

    virtual ~Exception() throw() {}
    int GetReplyCode() const;

    virtual const char* what() const throw();

    static void replyToString(const amqp_rpc_reply_t& res, std::string& msg, int& code);
    static std::string errorToString(int err);

protected:
    std::string _message;
    int _code;
};
} // namespace AMQP

#endif // !AMQP_CPP_EXCEPTION_H__
