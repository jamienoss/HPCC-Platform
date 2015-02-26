/*##############################################################################

    HPCC SYSTEMS software Copyright (C) 2015 HPCC Systems.

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
############################################################################## */

#include "platform.h"
#include "hiredis/adapters/libev.h"//include this here due to conflict with our macro def of 'loop' and that from libev
#include "eclrtl.hpp"
#include "jstring.hpp"
#include "jsem.hpp"
#include "jbuff.hpp"
#include "jthread.hpp"
#include "jmutex.hpp"
#include "redisplugin.hpp"
#include "redissync.hpp"
#include "redisasync.hpp"
#include "hiredis/async.h"

namespace RedisPlugin {
static CriticalSection crit;
static const char * REDIS_LOCK_PREFIX = "redis_ecl_lock";

typedef void (EvTimeoutCBFn)(struct ev_loop * evLoop, ev_timer *w, int revents);

class EvDataContainer
{
public :
    EvDataContainer() : eventLoop(NULL), timer(NULL), data(NULL), exceptionMessage(NULL), exceptionCode(0) { }
    EvDataContainer(struct ev_loop * _eventLoop, ev_timer * _timer, void * _data, int _exceptionCode, const char * _exceptionMessage)
      : eventLoop(_eventLoop), timer(_timer), data(_data), exceptionMessage(_exceptionMessage), exceptionCode(_exceptionCode) { }
    ~EvDataContainer() { eventLoop = NULL; timer = NULL; data = NULL; }

    void stopTimeoutTimer()
    {
        if (eventLoop && timer)//leave to time out rather than throw specific exception
            ev_timer_stop(eventLoop, timer);
    }
    static void stopTimeoutTimer(void * privdata);

public :
    struct ev_loop * eventLoop;
    ev_timer * timer;
    void * data;
    StringAttr exceptionMessage;
    int exceptionCode;
};
class AsyncConnection : public Connection
{
public :
    AsyncConnection(ICodeContext * ctx, const char * _options, unsigned __int64 _database, const char * password, unsigned __int64 timeout);
    AsyncConnection(ICodeContext * ctx, RedisServer * _server, unsigned __int64 _database, const char * password, unsigned __int64 timeout);
    ~AsyncConnection();

    static AsyncConnection * createConnection(ICodeContext * ctx, const char * options, unsigned __int64 database, const char * password, unsigned __int64 timeout)
    {
        return new AsyncConnection(ctx, options, database, password, timeout);
    }
    static AsyncConnection * createConnection(ICodeContext * ctx, RedisServer * _server, unsigned __int64 database, const char * password, unsigned __int64 timeout)
    {
        return new AsyncConnection(ctx, _server, database, password, timeout);
    }

    //get
    void get(ICodeContext * ctx, const char * key, MemoryAttr * retVal, const char * password);
    template <class type> void get(ICodeContext * ctx, const char * key, size_t & valueLength, type * & value, const char * password);
    void getVoidPtrLenPair(ICodeContext * ctx, const char * key, size_t & valueLength, void * & value, const char * password);
    //set
    template<class type> void set(ICodeContext * ctx, const char * key, type value, unsigned expire);
    template<class type> void set(ICodeContext * ctx, const char * key, size32_t valueLength, const type * value, unsigned expire);

    bool missThenLock(ICodeContext * ctx, const char * key);
    static void assertContextErr(const redisAsyncContext * context);
    static void assertConnection(const redisAsyncContext * context);
    static void assertContext(const redisAsyncContext * context, const char * _msg);

protected :
    virtual void selectDb(ICodeContext * ctx);
    virtual void assertOnError(const redisReply * reply, const char * _msg);
    void createAndAssertConnection(ICodeContext * ctx, const char * password);
    void assertRedisErr(int reply, const char * _msg);
    void handleLockOnGet(ICodeContext * ctx, const char * key, MemoryAttr * retVal, const char * password);
    void handleLockOnSet(ICodeContext * ctx, const char * key, const char * value, size_t size, unsigned expire);
    //void subscribe(const char * channel, StringAttr & value);
    //void sendUnsubscribeCommand(const char * channel);
    void ensureEventLoopAttached(struct ev_loop * eventLoop);
    bool lock(ICodeContext * ctx, const char * key, const char * channel);
    //void awaitResponceViaEventLoop(struct ev_loop * evLoop, void * data, EvTimeoutCBFn * callback);
    void encodeChannel(StringBuffer & channel, const char * key) const;
    void authenticate(ICodeContext * ctx, const char * password);
    void sendCommandAndWait(ICodeContext * ctx, struct ev_loop * eventLoop, EvTimeoutCBFn * timeoutCallback, void * timeoutData, int exceptionCode, const char * exceptionMessage, \
                            redisCallbackFn, void * callabckData, const char * command, ...);

    //callbacks
    static void assertCallbackError(const redisAsyncContext * context, const redisReply * reply, const char * _msg);
    static void connectCB(const redisAsyncContext *c, int status);
    static void disconnectCB(const redisAsyncContext *c, int status);
    static void hiredisLessThan011ConnectCB(const redisAsyncContext *c);
    //static void subCB(redisAsyncContext * context, void * _reply, void * privdata);
    static void pubCB(redisAsyncContext * context, void * _reply, void * privdata);
    static void setCB(redisAsyncContext * context, void * _reply, void * privdata);
    static void getCB(redisAsyncContext * context, void * _reply, void * privdata);
    static void unsubCB(redisAsyncContext * context, void * _reply, void * privdata);
    static void setLockCB(redisAsyncContext * context, void * _reply, void * privdata);
    static void selectCB(redisAsyncContext * context, void * _reply, void * privdata);
    static void timeoutCB(struct ev_loop * evLoop, ev_timer *w, int revents);
    static void authenticationCB(redisAsyncContext * context, void * _reply, void * privdata);

protected :
    redisAsyncContext * context;
};
class SubscriptionThread : implements IThreaded, implements IInterface, public AsyncConnection
{
public :
    SubscriptionThread(ICodeContext * ctx, RedisServer * _server, const char * channel, redisCallbackFn * _callback, unsigned __int64 _database, const char * password, unsigned __int64 timeout);
    ~SubscriptionThread();

    void setMessage(const char * _message) { message.set(_message, strlen(_message)); }
    void waitForMessage() { assertSemaphoreTimeout(messageSemaphore.wait(timeout/1000)); }//timeout is in micro-secs and wait() requires ms
    void signalMessageReceived() { messageSemaphore.signal(); }
    void activateSubscriptionSemaphore() { activeSubscriptionSemaphore.signal(); }
    void waitForSubscriptionActivation() { assertSemaphoreTimeout(activeSubscriptionSemaphore.wait(timeout/1000)); }//timeout is in micro-secs and wait() requires ms
    //void subscribe(struct ev_loop * evLoop);
    void unsubscribe();
    void waitForMessage(MemoryAttr * value);
    void start();
    void stopEvLoop();
    void rtlFail(int code, const char * msg);
    static void subCB(redisAsyncContext * context, void * _reply, void * privdata);
    static void StTimeoutCB(struct ev_loop * evLoop, ev_timer *w, int revents);
    void join();

    IMPLEMENT_IINTERFACE;

private :
    void main();
    void assertSemaphoreTimeout(bool timedout);

private :
    Semaphore messageSemaphore;
    Semaphore activeSubscriptionSemaphore;
    redisCallbackFn * callback;
    StringAttr message;
    StringAttr channel;
    CThreaded  thread;
    struct ev_loop * evLoop;
};
SubscriptionThread::SubscriptionThread(ICodeContext * ctx, RedisServer * _server, const char * _channel, redisCallbackFn * _callback, unsigned __int64 _database, const char * password, unsigned __int64 timeout)
  : AsyncConnection(ctx, _server, _database, password, timeout), thread("SubscriptionThread", (IThreaded*)this), evLoop(NULL)
{
    channel.set(_channel, strlen(_channel));
    if (_callback)
        callback = _callback;
    else
        callback = subCB;
}
SubscriptionThread::~SubscriptionThread()
{
    //sendUnsubscribeCommand();//This could be shaved off. Since the connection/context belongs to this class, closing a connection with the server unsubscribes all channels on it.
    join();
}
void SubscriptionThread::start()
{
    thread.start();
    waitForSubscriptionActivation();//wait for subscription to be acknowledged by redis
}
void SubscriptionThread::main()
{
    evLoop = ev_loop_new(0);
#if (1)
    sendCommandAndWait(NULL, evLoop, StTimeoutCB, (void*)this, 0, NULL, callback, (void*)this, "SUBSCRIBE %b", channel.str(), channel.length());
#else
    ensureEventLoopAttached(evLoop);
    EvDataContainer container(evLoop, NULL, (void*)this, 0, "subscription thread fail");
    assertRedisErr(redisAsyncCommand(context, callback, (void*)&container, "SUBSCRIBE %b", channel.str(), channel.length()), "SUBSCRIBE buffer write error");
   // awaitResponceViaEventLoop(evLoop, static_cast<void*>(this), StTimeoutCB);
    ev_run(evLoop, 0);//wait for response WITHOUT a timeout. This is manually stopped in ~SubscriptionThread() from the parent in handleLockOnGet
#endif
    printf("loop stopped\n");
}
void SubscriptionThread::stopEvLoop()
{
    CriticalBlock block(crit);
    assertContext(context, "internal fail within stopEvLoop()");
    context->ev.delRead(context->ev.data);
    context->ev.delWrite(context->ev.data);
}
void SubscriptionThread::join()
{
    CriticalBlock block(crit);
    stopEvLoop();
    thread.join();
}
void SubscriptionThread::assertSemaphoreTimeout(bool didNotTimeout)
{
    if (!didNotTimeout)
        this->rtlFail(0, "RedisPlugin : internal subscription thread timed out");
}
void SubscriptionThread::waitForMessage(MemoryAttr * value)
{
    waitForMessage();
    value->setOwn(message.length(), message.detach());
}
AsyncConnection::AsyncConnection(ICodeContext * ctx, const char * _options, unsigned __int64 _database, const char * password, unsigned __int64 timeout)
  : Connection(ctx, _options, password, timeout), context(NULL)
{
    createAndAssertConnection(ctx, password);
}
AsyncConnection::AsyncConnection(ICodeContext * ctx, RedisServer * server, unsigned __int64 _database, const char * password, unsigned __int64 timeout)
  : Connection(ctx, server, password, timeout), context(NULL)
{
    createAndAssertConnection(ctx, password);
}
void EvDataContainer::stopTimeoutTimer(void * privdata)
{
    if (privdata)
    {
        EvDataContainer * container = static_cast<EvDataContainer*>(privdata);
        ev_timer_stop(container->eventLoop, container->timer);
    }
}
void AsyncConnection::selectDb(ICodeContext * ctx)
{
    if (database == 0)//connections are not cached so only check that not default rather than against previous connection selected database
        return;
    ensureEventLoopAttached(EV_DEFAULT);
    VStringBuffer cmd("SELECT %" I64F "u", database);
    sendCommandAndWait(ctx, EV_DEFAULT, timeoutCB, NULL, 0, NULL, selectCB, NULL, cmd.str());
}
void AsyncConnection::authenticate(ICodeContext * ctx, const char * password)
{
    if (strlen(password) > 0)
        sendCommandAndWait(ctx, EV_DEFAULT, timeoutCB, NULL, 0, NULL, authenticationCB, NULL, "AUTH %b", password, strlen(password));
}
void AsyncConnection::sendCommandAndWait(ICodeContext * ctx, struct ev_loop * eventLoop, EvTimeoutCBFn * timeoutCallback, void * timeoutData, int exceptionCode, const char * _exceptionMessage, \
        redisCallbackFn * callback, void * callbackData, const char * command, ...)
{
    ensureEventLoopAttached(eventLoop);

    StringAttr exceptionMessage;
    if(_exceptionMessage)
        exceptionMessage.set(_exceptionMessage, strlen(_exceptionMessage));
    else
        exceptionMessage.set(command, strlen(command));

    ev_timer timer;
    EvDataContainer timeoutContainer(NULL, NULL, timeoutData, exceptionCode, exceptionMessage.str());
    timer.data = (void*)&timeoutContainer;//Make accessible from EvTimeoutCBFn * timeoutCallback.
    EvDataContainer container(eventLoop, &timer, callbackData, exceptionCode, exceptionMessage.str());

    StringBuffer redisBufferMessage(" buffer write error of - ");
    redisBufferMessage.append(command);

    va_list arguments;
    va_start(arguments, command);
    assertRedisErr(redisvAsyncCommand(context, callback, (void*)&container, command, arguments), redisBufferMessage.str());
    va_end(arguments);

    ev_tstamp evTimeout = timeout/1000000;//timeout has units micro-seconds & ev_tstamp has that of seconds.
    ev_timer_init(&timer, timeoutCallback, evTimeout, 0.);
    ev_timer_start(eventLoop, &timer);

    ev_run(eventLoop, 0);//the actual event loop
}
AsyncConnection::~AsyncConnection()
{
    if (context)
    {
        //redis can auto disconnect upon certain errors, disconnectCB is called to handle this and is automatically
        //disconnected after this freeing the context
        if (context->err == REDIS_OK)//prevent double free due to the above reason
            redisAsyncDisconnect(context);
    }
}
void AsyncConnection::createAndAssertConnection(ICodeContext * ctx, const char * password)
{
    context = redisAsyncConnect(ip(), port());
    assertConnection(context);
    context->data = (void*)this;
#if (HIREDIS_MAJOR == 0 && HIREDIS_MINOR < 11)
    assertRedisErr(redisAsyncSetConnectCallback(context, hiredisLessThan011ConnectCB), "failed to set connect callback");
#else
    assertRedisErr(redisAsyncSetConnectCallback(context, connectCB), "failed to set connect callback");
#endif
    assertRedisErr(redisAsyncSetDisconnectCallback(context, disconnectCB), "failed to set disconnect callback");

    authenticate(ctx, password);
    selectDb(ctx);
}
void AsyncConnection::assertConnection(const redisAsyncContext * context)
{
    assertContext(context, "async");
    assertContextErr(context);
}
void AsyncConnection::assertContextErr(const redisAsyncContext * context)
{
    assertContext(context, "internal fail of assertContextErr()");
    if (context->err)//NOTE: This would cause issues if caching connections and a non rtFail error was observed. Would require resetting context->err upon such occurrences
    {
        const AsyncConnection * connection = (const AsyncConnection*)context->data;
        if (connection)
        {
            VStringBuffer msg("Redis Plugin : failed to create connection context for %s:%d - %s", connection->ip(), connection->port(), context->errstr);
            rtlFail(0, msg.str());
        }
        else
        {
            VStringBuffer msg("Redis Plugin : failed to create connection context - %s", context->errstr);
            rtlFail(0, msg.str());
        }
    }
}
void AsyncConnection::assertRedisErr(int reply, const char * _msg)
{
    if (reply != REDIS_OK)
    {
        VStringBuffer msg("Redis Plugin: %s", _msg);
        rtlFail(0, msg.str());
    }
}
void AsyncConnection::assertOnError(const redisReply * reply, const char * _msg)
{
    if (!reply)
    {
        assertConnection(context);
        //There should always be a redis context error but in case there is not report via the following
        VStringBuffer msg("Redis Plugin: %s%s", _msg, "with neither a 'reply' nor connection error");
        rtlFail(0, msg.str());
    }
    else if (reply->type == REDIS_REPLY_ERROR)
    {
        VStringBuffer msg("Redis Plugin: %s%s", _msg, reply->str);
        rtlFail(0, msg.str());
    }
}
void AsyncConnection::assertCallbackError(const redisAsyncContext * context, const redisReply * reply, const char * _msg)
{
    if (!reply)
    {
        VStringBuffer msg("Redis Plugin: %s - NULL reply", _msg);
        rtlFail(0, msg.str());
    }
    else if (reply->type == REDIS_REPLY_ERROR)
    {
        VStringBuffer msg("Redis Plugin: %s - %s", _msg, reply->str);
        rtlFail(0, msg.str());
    }
}
void AsyncConnection::assertContext(const redisAsyncContext * context, const char * _msg)
{
    if (!context)
    {
        VStringBuffer msg("Redis Plugin: %s - NULL context", _msg);
        rtlFail(0, msg.str());
    }
}

void AsyncConnection::timeoutCB(struct ev_loop * evLoop, ev_timer *w, int revents)
{
    EvDataContainer * container = (EvDataContainer*)w->data;
    StringBuffer msg("Redis Plugin : async operation timed out");
    if (container)
        msg.append(" - ").append(container->exceptionMessage.str());
    rtlFail(0, msg.str());
}
void SubscriptionThread::StTimeoutCB(struct ev_loop * evLoop, ev_timer *w, int revents)
{
    EvDataContainer * container = (EvDataContainer*)w->data;
	printf("in timeout\n");
    if (container && container->data)
    {
        StringBuffer msg("Redis Plugin : async operation timed out");
        msg.append(" - ").append(container->exceptionMessage.str());
        SubscriptionThread * thread = (SubscriptionThread*)container->data;
        thread->rtlFail(0, msg.str());
    }
    //should still do something?
}
void SubscriptionThread::rtlFail(int code, const char * msg)
{
    join();
    ::rtlFail(code, msg);
}
//callbacks-----------------------------------------------------------------
void AsyncConnection::hiredisLessThan011ConnectCB(const redisAsyncContext * context)
{
    assertContext(context, "failed to connect with no error details");
    if (context->err)
    {
        if (context->data)
        {
            const AsyncConnection * connection = (AsyncConnection *)context->data;
            VStringBuffer msg("Redis Plugin : failed to connect to %s:%d - %s", connection->ip(), connection->port(), context->errstr);
            rtlFail(0, msg.str());
        }
        else
        {
            VStringBuffer msg("Redis Plugin : failed to connect - %s", context->errstr);
            rtlFail(0, msg.str());
        }
    }
}
void AsyncConnection::connectCB(const redisAsyncContext * context, int status)
{
    if (status != REDIS_OK)
    {
        assertContext(context, "failed to connect with no error details");
        if (context->data)
        {
            const AsyncConnection * connection = (AsyncConnection *)context->data;
            VStringBuffer msg("Redis Plugin : failed to connect to %s:%d - %s", connection->ip(), connection->port(), context->errstr);
            rtlFail(0, msg.str());
        }
        else
        {
            VStringBuffer msg("Redis Plugin : failed to connect - %s", context->errstr);
            rtlFail(0, msg.str());
        }
    }
}
void AsyncConnection::disconnectCB(const redisAsyncContext * context, int status)
{
    if (status != REDIS_OK)
    {
        assertContext(context, "server forced disconnect with no error details");
        if (context->data)
        {
            const AsyncConnection  * connection = (AsyncConnection*)context->data;
            VStringBuffer msg("Redis Plugin : server (%s:%d) forced disconnect - %s", connection->ip(), connection->port(), context->errstr);
            rtlFail(0, msg.str());
        }
        else
        {
            VStringBuffer msg("Redis Plugin : server forced disconnect - %s", context->errstr);
            rtlFail(0, msg.str());
        }
    }
}
void SubscriptionThread::subCB(redisAsyncContext * context, void * _reply, void * privdata)
{
    //if (_reply == NULL || privdata == NULL)
      //  return;

    redisReply * reply = (redisReply*)_reply;



    //can't rtl fail from sub thread!!!!!!!!!!!!!!!!!!!!!!!!!
    assertCallbackError(context, reply, "subscription thread sub callback fail");





    if (reply->type == REDIS_REPLY_ARRAY)
    {
        EvDataContainer * container = (EvDataContainer*)privdata;
        if (container)
        {
            SubscriptionThread * subscriber = (SubscriptionThread*)container->data;
            if (subscriber)
            {
                if (strcmp("subscribe", reply->element[0]->str) == 0 )//Upon subscribing redis replies with an 'OK' acknowledging the subscription
                {
                    subscriber->activateSubscriptionSemaphore();//Signal that the subscription has been acknowledged
                    //container->stopTimeoutTimer();//Not actually needed as the evLoop is called without a timer, but for future change
                }
                else if (strcmp("message", reply->element[0]->str) == 0 )//Any publication will be tagged as a 'message'
                {
                    subscriber->setMessage(reply->element[2]->str);
                    const char * channel = reply->element[1]->str;//We could extract the channel from 'SubscriptionThread * subscriber'
                    //redisAsyncCommand(context, NULL, NULL, "UNSUBSCRIBE %b", channel, strlen(channel));
                    //redisAsyncHandleWrite(context);//send unsubscribe command but don't bother to wait as we close socket read event below
                    assertContext(context, "subscription thread sub callback fail");
                    context->ev.delRead(context->ev.data);
                    subscriber->signalMessageReceived();
                    //container->stopTimeoutTimer();//Not actually needed as the evLoop is called without a timer, but for future change
                }
            }
        }
    }
}
void AsyncConnection::unsubCB(redisAsyncContext * context, void * _reply, void * privdata)
{
    redisReply * reply = (redisReply*)_reply;
    const char * msg = "unsub callback fail";
    assertCallbackError(context, reply, msg);
    assertContext(context, msg);
    redisLibevDelRead(context->ev.data);
    EvDataContainer::stopTimeoutTimer(privdata);
}
void AsyncConnection::getCB(redisAsyncContext * context, void * _reply, void * privdata)
{
    redisReply * reply = (redisReply*)_reply;
    const char * msg =  "get callback fail";
    assertCallbackError(context, reply, msg);

    EvDataContainer * container = (EvDataContainer*)privdata;
    if (container && container->data)
    {
        MemoryAttr * retVal = (MemoryAttr*)container->data;
        retVal->set((size32_t)reply->len, (const void*)reply->str);
    }
    assertContext(context, msg);
    redisLibevDelRead(context->ev.data);
    container->stopTimeoutTimer();
}
void AsyncConnection::setCB(redisAsyncContext * context, void * _reply, void * privdata)
{
    redisReply * reply = (redisReply*)_reply;
    const char * msg = "set callback fail";
    assertCallbackError(context, reply, msg);
    assertContext(context, msg);
    redisLibevDelRead(context->ev.data);
    EvDataContainer::stopTimeoutTimer(privdata);
}
void AsyncConnection::setLockCB(redisAsyncContext * context, void * _reply, void * privdata)
{
    redisReply * reply = (redisReply*)_reply;
    const char * msg = "set lock callback fail";
    assertCallbackError(context, reply, msg);

    //Redis, with SET NX, will return a REDIS_REPLY_STATUS with an 'OK' message upon a successful SET.
    //If the key was already present then (and thus not SET) REDIS_REPLY_NIL is used instead.
    EvDataContainer * container = (EvDataContainer*)privdata;
    if (container)
    {
        EvDataContainer * container = (EvDataContainer*)privdata;
        switch(reply->type)
        {
        case REDIS_REPLY_STATUS :
            *(bool*)container->data = strcmp(reply->str, "OK") == 0;
            break;
        case REDIS_REPLY_NIL :
            *(bool*)container->data = false;
        }
    }
    assertContext(context, msg);
    redisLibevDelRead(context->ev.data);
    container->stopTimeoutTimer();
}
void AsyncConnection::selectCB(redisAsyncContext * context, void * _reply, void * privdata)
{
    redisReply * reply = (redisReply*)_reply;
    const char * msg = "select callback fail";
    assertCallbackError(context, reply, msg);
    assertContext(context, msg);
    redisLibevDelRead(context->ev.data);
    EvDataContainer::stopTimeoutTimer(privdata);
}
void AsyncConnection::authenticationCB(redisAsyncContext * context, void * _reply, void * privdata)
{
    redisReply * reply = (redisReply*)_reply;
    const char * msg = "password authentication callback fail";
    assertCallbackError(context, reply, msg);
    assertContext(context, msg);
    redisLibevDelRead(context->ev.data);
    EvDataContainer::stopTimeoutTimer(privdata);
}
void AsyncConnection::pubCB(redisAsyncContext * context, void * _reply, void * privdata)
{
    redisReply * reply = (redisReply*)_reply;
    const char * msg = "pub callback fail";
    assertCallbackError(context, reply, msg);
    assertContext(context, msg);
    redisLibevDelRead(context->ev.data);
    EvDataContainer::stopTimeoutTimer(privdata);
}
//end of callbacks
void SubscriptionThread::unsubscribe()
{
    ensureEventLoopAttached(evLoop);
    assertRedisErr(redisAsyncCommand(context, NULL, NULL, "UNSUBSCRIBE %b", channel.str(), channel.length()), "UNSUBSCRIBE buffer write error");
    redisAsyncHandleWrite(context);
}
bool AsyncConnection::missThenLock(ICodeContext * ctx, const char * key)
{
    StringBuffer channel;
    encodeChannel(channel, key);
    return lock(ctx, key, channel);
}
void AsyncConnection::ensureEventLoopAttached(struct ev_loop * eventLoop)
{
    assertConnection(context);
    if (!context->ev.data)
        assertRedisErr(redisLibevAttach(eventLoop, context), "failed to attach evLoop to context");

    redisLibevEvents * contextEv = static_cast<redisLibevEvents*>(context->ev.data);

//move this to beginning or header?
#ifdef loop
#undef loop
#endif
    if (contextEv->loop == eventLoop)
        return;//already attached
    else
    {
        context->ev.data = NULL;//required otherwise redisLibevAttach() (below) will fail
        //The above may not be a good idea, perhaps we should let redisLibevAttach() fail?
        assertRedisErr(redisLibevAttach(eventLoop, context), "failed to attach new evLoop to context");
    }
}
void AsyncConnection::encodeChannel(StringBuffer & channel, const char * key) const
{
    channel.append(REDIS_LOCK_PREFIX).append("_").append(key).append("_").append(database).append("_").append(server->getIp()).append("_").append(server->getPort());
}
bool AsyncConnection::lock(ICodeContext * ctx, const char * key, const char * channel)
{
    bool locked = false;

    StringBuffer cmd("SET %b %b NX EX ");
    cmd.append(timeout);
    sendCommandAndWait(ctx, EV_DEFAULT, timeoutCB, NULL, 0, NULL, setLockCB, (void*)&locked, cmd.str(), key, strlen(key), channel, strlen(channel));

    return locked;
}
void AsyncConnection::handleLockOnGet(ICodeContext * ctx, const char * key, MemoryAttr * retVal, const char * password)
{
    StringBuffer channel;
    encodeChannel(channel, key);
    Owned<SubscriptionThread> subscriptionThread = new SubscriptionThread(ctx, server.getLink(), channel.str(), NULL, database, password, timeout);
    subscriptionThread->start();//subscribe and wait for 1st callback that redis received sub. Do not block for main message callback this is the point of the thread.

    //GET command
    sendCommandAndWait(ctx, EV_DEFAULT, timeoutCB, NULL, 0, NULL, getCB, (void*)retVal, "GET %b", key, strlen(key));
    printf("GET complete - %s\n", reinterpret_cast<const char*>(retVal->get()));
    //check if value is locked
    if (strncmp(reinterpret_cast<const char*>(retVal->get()), REDIS_LOCK_PREFIX, strlen(REDIS_LOCK_PREFIX)) == 0 )
        subscriptionThread->waitForMessage(retVal);//locked so subscribe for value
    //subscriptionThread->join();//test case only. Called within ~SubscriptionThread()
}
void AsyncConnection::handleLockOnSet(ICodeContext * ctx, const char * key, const char * value, size_t size, unsigned expire)
{
    StringBuffer cmd("SET %b %b");
    RedisPlugin::appendExpire(cmd, expire);

    //Due to locking logic surfacing into ECL, any locking.set (such as this is) assumes that they own the lock and therefore go ahead and set
    //It is possible for a process/call to 'own' a lock and store this info in the LockObject, however, this prevents sharing between clients.
    sendCommandAndWait(ctx, EV_DEFAULT, timeoutCB, NULL, 0, NULL, setCB, NULL, cmd.str(), key, strlen(key), value, size);

    StringBuffer channel;
    encodeChannel(channel, key);
    //this only waits to ensure redis received pub cmd. MORE: reply contains number of subscribers on that channel - utilise this?
    sendCommandAndWait(ctx, EV_DEFAULT, timeoutCB, NULL, 0, NULL, pubCB, NULL, "PUBLISH %b %b", channel.str(), channel.length(), value, size);
}
//-------------------------------------------GET-----------------------------------------
//---OUTER---
template<class type> void AsyncRGet(ICodeContext * ctx, const char * options, const char * key, type & returnValue, unsigned __int64 database, const char * password, unsigned __int64 timeout)
{
    Owned<AsyncConnection> master = AsyncConnection::createConnection(ctx, options, database, password, timeout);
    MemoryAttr retVal;
    master->get(ctx, key, &retVal, password);
    StringBuffer keyMsg = getFailMsg;

    size_t returnSize = retVal.length();
    if (sizeof(type)!=returnSize)
    {
        VStringBuffer msg("RedisPlugin: ERROR - Requested type of different size (%uB) from that stored (%uB).", (unsigned)sizeof(type), (unsigned)returnSize);
        rtlFail(0, msg.str());
    }
    returnValue = *(static_cast<const type*>(retVal.get()));
    //memcpy(&returnValue, retVal.str(), returnSize);
}
template<class type> void AsyncRGet(ICodeContext * ctx, const char * options, const char * key, size_t & returnLength, type * & returnValue, unsigned __int64 database, const char * password, unsigned __int64 timeout)
{
    Owned<AsyncConnection> master = AsyncConnection::createConnection(ctx, options, database, password, timeout);
    master->get(ctx, key, returnLength, returnValue, password);
}
void AsyncRGetVoidPtrLenPair(ICodeContext * ctx, const char * options, const char * key, size_t & returnLength, void * & returnValue, unsigned __int64 database, const char * password, unsigned __int64 timeout)
{
    Owned<AsyncConnection> master = AsyncConnection::createConnection(ctx, options, database, password, timeout);
    master->getVoidPtrLenPair(ctx, key, returnLength, returnValue, password);
}
//---INNER---
void AsyncConnection::get(ICodeContext * ctx, const char * key, MemoryAttr * retVal, const char * password)
{
    handleLockOnGet(ctx, key, retVal, password);
}
template<class type> void AsyncConnection::get(ICodeContext * ctx, const char * key, size_t & returnSize, type * & returnValue, const char * password)
{
    MemoryAttr retVal;
    handleLockOnGet(ctx, key, &retVal, password);

    returnSize = retVal.length();
    returnValue = reinterpret_cast<type*>(retVal.detach());
}
void AsyncConnection::getVoidPtrLenPair(ICodeContext * ctx, const char * key, size_t & returnSize, void * & returnValue, const char * password)
{
    MemoryAttr retVal;
    handleLockOnGet(ctx, key, &retVal, password);

    returnSize = retVal.length();
    returnValue = retVal.detach();//cpy(retVal.str(), returnSize));
}
//-------------------------------------------SET-----------------------------------------
//---OUTER---
template<class type> void AsyncRSet(ICodeContext * ctx, const char * _options, const char * key, type value, unsigned expire, unsigned __int64 database, const char * password, unsigned __int64 timeout)
{
    Owned<AsyncConnection> master = AsyncConnection::createConnection(ctx, _options, database, password, timeout);
    master->set(ctx, key, value, expire);
}
//Set pointer types
template<class type> void AsyncRSet(ICodeContext * ctx, const char * _options, const char * key, size32_t valueLength, const type * value, unsigned expire, unsigned __int64 database, const char * password, unsigned __int64 timeout)
{
    Owned<AsyncConnection> master = AsyncConnection::createConnection(ctx, _options, database, password, timeout);
    master->set(ctx, key, valueLength, value, expire);
}
//---INNER---
template<class type> void AsyncConnection::set(ICodeContext * ctx, const char * key, type value, unsigned expire)
{
    const char * _value = reinterpret_cast<const char *>(&value);//Do this even for char * to prevent compiler complaining
    handleLockOnSet(ctx, key, _value, sizeof(type), expire);
}
template<class type> void AsyncConnection::set(ICodeContext * ctx, const char * key, size32_t valueSize, const type * value, unsigned expire)
{
    const char * _value = reinterpret_cast<const char *>(value);//Do this even for char * to prevent compiler complaining
    handleLockOnSet(ctx, key, _value, (size_t)valueSize, expire);
}
//-------------------------------ENTRY-POINTS-------------------------------------
ECL_REDIS_API bool ECL_REDIS_CALL AsyncRMissThenLock(ICodeContext * ctx, const char * key, const char * options, unsigned __int64 database, const char * password, unsigned __int64 timeout)
{
    Owned<AsyncConnection> master = AsyncConnection::createConnection(ctx, options, database, password, timeout);
    return master->missThenLock(ctx, key);
}
//-----------------------------------SET------------------------------------------
ECL_REDIS_API void ECL_REDIS_CALL AsyncRSetStr(ICodeContext * ctx, size32_t & returnLength, char * & returnValue, const char * key, size32_t valueLength, const char * value, const char * options, unsigned __int64 database, unsigned expire, const char * password, unsigned __int64 timeout)
{
    AsyncRSet(ctx, options, key, valueLength, value, expire,  database, password, timeout);
    returnLength = valueLength;
    returnValue = (char*)memcpy(rtlMalloc(valueLength), value, valueLength);
}
ECL_REDIS_API void ECL_REDIS_CALL AsyncRSetUChar(ICodeContext * ctx, size32_t & returnLength, UChar * & returnValue, const char * key, size32_t valueLength, const UChar * value, const char * options, unsigned __int64 database, unsigned expire, const char * password, unsigned __int64 timeout)
{
    AsyncRSet(ctx, options, key, (valueLength)*sizeof(UChar), value, expire, database, password, timeout);
    returnLength = valueLength;
    returnValue = (UChar*)memcpy(rtlMalloc(valueLength), value, valueLength);
}
ECL_REDIS_API signed __int64 ECL_REDIS_CALL AsyncRSetInt(ICodeContext * ctx, const char * key, signed __int64 value, const char * options, unsigned __int64 database, unsigned expire, const char * password, unsigned __int64 timeout)
{
    AsyncRSet(ctx, options, key, value, expire, database, password, timeout);
    return value;
}
ECL_REDIS_API unsigned __int64 ECL_REDIS_CALL AsyncRSetUInt(ICodeContext * ctx, const char * key, unsigned __int64 value, const char * options, unsigned __int64 database, unsigned expire, const char * password, unsigned __int64 timeout)
{
    AsyncRSet(ctx, options, key, value, expire, database, password, timeout);
    return value;
}
ECL_REDIS_API double ECL_REDIS_CALL AsyncRSetReal(ICodeContext * ctx, const char * key, double value, const char * options, unsigned __int64 database, unsigned expire, const char * password, unsigned __int64 timeout)
{
    AsyncRSet(ctx, options, key, value, expire, database, password, timeout);
    return value;
}
ECL_REDIS_API bool ECL_REDIS_CALL AsyncRSetBool(ICodeContext * ctx, const char * key, bool value, const char * options, unsigned __int64 database, unsigned expire, const char * password, unsigned __int64 timeout)
{
    AsyncRSet(ctx, options, key, value, expire, database, password, timeout);
    return value;
}
ECL_REDIS_API void ECL_REDIS_CALL AsyncRSetData(ICodeContext * ctx, size32_t & returnLength, void * & returnValue, const char * options, const char * key, size32_t valueLength, const void * value, unsigned __int64 database, unsigned expire, const char * password, unsigned __int64 timeout)
{
    AsyncRSet(ctx, options, key, valueLength, value, expire, database, password, timeout);
    returnLength = valueLength;
    returnValue = memcpy(rtlMalloc(valueLength), value, valueLength);
}
ECL_REDIS_API void ECL_REDIS_CALL AsyncRSetUtf8(ICodeContext * ctx, size32_t & returnLength, char * & returnValue, const char * key, size32_t valueLength, const char * value, const char * options, unsigned __int64 database, unsigned expire, const char * password, unsigned __int64 timeout)
{
    AsyncRSet(ctx, options, key, rtlUtf8Size(valueLength, value), value, expire, database, password, timeout);
    returnLength = valueLength;
    returnValue = (char*)memcpy(rtlMalloc(valueLength), value, valueLength);
}
//-------------------------------------GET----------------------------------------
ECL_REDIS_API bool ECL_REDIS_CALL AsyncRGetBool(ICodeContext * ctx, const char * key, const char * options, unsigned __int64 database, const char * password, unsigned __int64 timeout)
{
    bool value;
    AsyncRGet(ctx, options, key, value, database, password, timeout);
    return value;
}
ECL_REDIS_API double ECL_REDIS_CALL AsyncRGetDouble(ICodeContext * ctx, const char * key, const char * options, unsigned __int64 database, const char * password, unsigned __int64 timeout)
{
    double value;
    AsyncRGet(ctx, options, key, value, database, password, timeout);
    return value;
}
ECL_REDIS_API signed __int64 ECL_REDIS_CALL AsyncRGetInt8(ICodeContext * ctx, const char * key, const char * options, unsigned __int64 database, const char * password, unsigned __int64 timeout)
{
    signed __int64 value;
    AsyncRGet(ctx, options, key, value, database, password, timeout);
    return value;
}
ECL_REDIS_API unsigned __int64 ECL_REDIS_CALL AsyncRGetUint8(ICodeContext * ctx, const char * key, const char * options, unsigned __int64 database, const char * password, unsigned __int64 timeout)
{
    unsigned __int64 value;
    AsyncRGet(ctx, options, key, value, database, password, timeout);
    return value;
}
ECL_REDIS_API void ECL_REDIS_CALL AsyncRGetStr(ICodeContext * ctx, size32_t & returnLength, char * & returnValue, const char * key, const char * options, unsigned __int64 database, const char * password, unsigned __int64 timeout)
{
    size_t _returnLength;
    AsyncRGet(ctx, options, key, _returnLength, returnValue, database, password, timeout);
    returnLength = static_cast<size32_t>(_returnLength);
}
ECL_REDIS_API void ECL_REDIS_CALL AsyncRGetUChar(ICodeContext * ctx, size32_t & returnLength, UChar * & returnValue,  const char * key, const char * options, unsigned __int64 database, const char * password, unsigned __int64 timeout)
{
    size_t returnSize = 0;
    AsyncRGet(ctx, options, key, returnSize, returnValue, database, password, timeout);
    returnLength = static_cast<size32_t>(returnSize/sizeof(UChar));
}
ECL_REDIS_API void ECL_REDIS_CALL AsyncRGetUtf8(ICodeContext * ctx, size32_t & returnLength, char * & returnValue, const char * key, const char * options, unsigned __int64 database, const char * password, unsigned __int64 timeout)
{
    size_t returnSize = 0;
    AsyncRGet(ctx, options, key, returnSize, returnValue, database, password, timeout);
    returnLength = static_cast<size32_t>(rtlUtf8Length(returnSize, returnValue));
}
ECL_REDIS_API void ECL_REDIS_CALL AsyncRGetData(ICodeContext * ctx, size32_t & returnLength, void * & returnValue, const char * key, const char * options, unsigned __int64 database, const char * password, unsigned __int64 timeout)
{
    size_t _returnLength = 0;
    AsyncRGet(ctx, options, key, _returnLength, returnValue, database, password, timeout);
    returnLength = static_cast<size32_t>(_returnLength);
}
}//close namespace
