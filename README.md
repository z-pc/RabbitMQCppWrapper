# RabbitMQ Cpp Wrapper

A simple wrapper of the  [rabbitmq-c](https://github.com/alanxz/rabbitmq-c) Rabbitmq-c library. Usage is extremely easy.

### Pre-required 
- [rabbitmq-c](https://github.com/alanxz/rabbitmq-c) 

### Installing
- The fastest and easiest way is to add all the files in the "./src/\*.\*" directory directly to your project.
- If you want to test this project, run the following command:

```
mkdir build
cd build
cmake..
```
### Using the wrapper
Very easy, some basic function
Create connection
```
auto cnn = AMQP::TCPConnection::createConnection("localhost", 5672);
```
Handle and create channel
```
auto channel = cnn->createChannel();
```
Declare exchange
```
auto exchange = channel->declareExchange("test_declare_exchange", "direct");
```
Declare queue with properties

```
AMQP::Table queuePrps;
queuePrps.addEntry(AMQP_FIELD_EXPIRES, 300000,AMQP_FIELD_SINGLE_ACTIVE_CONSUMER, true);

queueName = channel->declareQueue("", false, false, false, true, &queuePrps);
channel->bindQueue(exchange, queueName, queueName);
```
Puslish a message  
```
channel->basicPublish(exchange, queueName, "congratulation", &msgProps);
```

Consume and get a message on queue
```
std::string cmsTag = channel->basicConsume(queueName);
auto envelope = channel->getMessage(timeOut);
channel->basicAck(envelope, true);
```
Don't think about freeing memory!

