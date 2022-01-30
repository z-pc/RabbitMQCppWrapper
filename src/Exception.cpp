#include "Exception.h"
#include <sstream>
using namespace AMQP;

AMQP::Exception::Exception()
{
    this->m_message = "unknown exception";
    this->m_code = -1;
}

Exception::Exception(const std::string& message, int error_code)
{
    this->m_message = message;
    if (error_code != -1)
        this->m_message = this->m_message + "\nDetails: " + amqp_error_string2(error_code);
    this->m_code = error_code;
}

AMQP::Exception::Exception(const std::string& message, const amqp_rpc_reply_t& res)
{
    std::string msg;
    Exception::replyToString(res, msg, this->m_code);
    if (message.empty())
        this->m_message = std::move(msg);
    else
        this->m_message = message + "\nDetails: " + msg;
}

int Exception::GetReplyCode() const { return m_code; }
std::string Exception::GetErrMsg() const
{
    std::stringstream msg;
    msg << m_message << "\nError code: " << m_code;
    return msg.str();
}

const char* AMQP::Exception::what() const throw() { return m_message.c_str(); }

void AMQP::Exception::replyToString(const amqp_rpc_reply_t& res, std::string& msg, int& code)
{
    if (res.reply_type == AMQP_RESPONSE_LIBRARY_EXCEPTION)
    {
        // this->msg = res.library_error ? strerror(res.library_error) : "end-of-stream";
        msg = amqp_error_string2(res.library_error);
        code = res.library_error;
    }
    else if (res.reply_type == AMQP_RESPONSE_SERVER_EXCEPTION)
    {
        char buf[512];
        memset(buf, 0, 512);
        code = 0;

        if (res.reply.id == AMQP_CONNECTION_CLOSE_METHOD)
        {
            amqp_connection_close_t* m = (amqp_connection_close_t*)res.reply.decoded;
            code = m->reply_code;

            sprintf(buf, "server connection error %uh, message: %.*s", m->reply_code,
                    (int)m->reply_text.len, (char*)m->reply_text.bytes);
        }
        else if (res.reply.id == AMQP_CHANNEL_CLOSE_METHOD)
        {
            amqp_channel_close_t* n = (amqp_channel_close_t*)res.reply.decoded;
            code = n->reply_code;

            sprintf(buf, "server channel error %d, message: %.*s class=%d method=%d", n->reply_code,
                    (int)n->reply_text.len, (char*)n->reply_text.bytes, (int)n->class_id,
                    n->method_id);
        }
        else
        {
            sprintf(buf, "unknown server error, method id 0x%08X", res.reply.id);
        }
        msg = buf;
    }
}
