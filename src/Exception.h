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
    std::string GetErrMsg() const;
    int GetReplyCode() const;

    virtual const char* what() const throw();

    static void replyToString(const amqp_rpc_reply_t& res, std::string& msg, int& code);

protected:
    std::string m_message;
    int m_code;
};
} // namespace AMQP

#endif // !AMQP_CPP_EXCEPTION_H__
