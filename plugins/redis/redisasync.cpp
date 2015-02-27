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

#ifdef loop
#undef loop
#endif

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
   /* static AsyncConnection * createConnection(ICodeContext * ctx, RedisServer * _server, unsigned __int64 database, const char * password, unsigned __int64 timeout)
    {
        return new AsyncConnection(ctx, _server, database, password, timeout);
    }*/

    //get
    void get(ICodeContext * ctx, const char * key, MemoryAttr * retVal, const char * password);
    template <class type> void get(ICodeContext * ctx, const char * key, size_t & valueLength, type * & value, const char * password);
    //set
    template<class type> void set(ICodeContext * ctx, const char * key, type value, unsigned expire);
    template<class type> void set(ICodeContext * ctx, const char * key, size32_t valueLength, const type * value, unsigned expire);

    static void delRead(redisAsyncContext * context);
    static void delWrite(redisAsyncContext * context);
    bool missThenLock(ICodeContext * ctx, const char * key);

protected :
    virtual void assertRedisErr(int reply, const char * _msg);
    virtual void rtlFail(int code, const char * msg) const { ::rtlFail(code, msg); }
    virtual void ensureEventLoopAttached(struct ev_loop * eventLoop);

    void createAndAssertConnection(ICodeContext * ctx, const char * password);
    void selectDb(ICodeContext * ctx);
    void authenticate(ICodeContext * ctx, const char * password);
    void handleLockOnGet(ICodeContext * ctx, const char * key, MemoryAttr * retVal, const char * password);
    void handleLockOnSet(ICodeContext * ctx, const char * key, const char * value, size_t size, unsigned expire);
    void encodeChannel(StringBuffer & channel, const char * key) const;
    void sendCommandAndWait(ICodeContext * ctx, struct ev_loop * eventLoop, EvTimeoutCBFn * timeoutCallback, void * timeoutData, int exceptionCode, const char * exceptionMessage, \
                            redisCallbackFn, void * callabckData, const char * command, ...);
    bool lock(ICodeContext * ctx, const char * key, const char * channel);

    //callbacks
    static void assertCallbackError(redisAsyncContext * context, const redisReply * reply, const char * _msg);
    static void connectCB(const redisAsyncContext *c, int status);
    static void disconnectCB(const redisAsyncContext *c, int status);
    static void hiredisLessThan011ConnectCB(const redisAsyncContext *c);
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

    virtual void assertRedisErr(int reply, const char * _msg);
    virtual void rtlFail(int code, const char * msg);
    virtual void ensureEventLoopAttached(struct ev_loop * eventLoop);

    void setMessage(const char * _message) { message.set(_message, strlen(_message)); }
    void waitForPublication() { assertSemaphoreTimeout(messageSemaphore.wait(timeout/1000)); }//timeout is in micro-secs and wait() requires ms
    void signalPublicationReceived() { messageSemaphore.signal(); }
    void signalSubscriptionActivated() { activeSubscriptionSemaphore.signal(); }
    void waitForSubscriptionActivation() { assertSemaphoreTimeout(activeSubscriptionSemaphore.wait(timeout/1000)); }//timeout is in micro-secs and wait() requires ms
    void waitForPublication(MemoryAttr * value);
    void startThread();
    void stopEvLoop();
    void join();
    static void subCB(redisAsyncContext * context, void * _reply, void * privdata);
    static void StTimeoutCB(struct ev_loop * evLoop, ev_timer *w, int revents);

    IMPLEMENT_IINTERFACE;

protected :
    virtual void main();
    void assertSemaphoreTimeout(bool timedout);

private :
    Semaphore messageSemaphore;
    Semaphore activeSubscriptionSemaphore;
    redisCallbackFn * callback;
    StringAttr message;
    StringAttr channel;
    CThreaded  thread;
    struct ev_loop * evLoop;

    bool exceptionThrown;
    int exceptionCode;
    StringAttr exceptionMessage;
};
SubscriptionThread::SubscriptionThread(ICodeContext * ctx, RedisServer * _server, const char * _channel, redisCallbackFn * _callback, unsigned __int64 _database, const char * password, unsigned __int64 timeout)
  : AsyncConnection(ctx, _server, _database, password, timeout), thread("SubscriptionThread", (IThreaded*)this), evLoop(EV_DEFAULT), exceptionThrown(false), exceptionCode(0)
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
    stopEvLoop();
    thread.join();//timeout?
}
void SubscriptionThread::startThread()
{
    thread.start();
    waitForSubscriptionActivation();//wait for subscription to be acknowledged by redis
}
void SubscriptionThread::main()
{
#if (0)
    sendCommandAndWait(NULL, ev_loop_new(0), StTimeoutCB, (void*)this, 0, NULL, callback, (void*)this, "SUBSCRIBE %b", channel.str(), channel.length());
#else
    ensureEventLoopAttached(ev_loop_new(0));
    EvDataContainer container(evLoop, NULL, (void*)this, 0, "subscription thread fail");
    assertRedisErr(redisAsyncCommand(context, callback, (void*)&container, "SUBSCRIBE %b", channel.str(), channel.length()), "SUBSCRIBE buffer write error");
   // awaitResponceViaEventLoop(evLoop, static_cast<void*>(this), StTimeoutCB);
    ev_run(evLoop, 0);//wait for response WITHOUT a timeout. This is stopped from the parent thread in handleLockOnGet
#endif
    printf("loop stopped\n");
}
void SubscriptionThread::stopEvLoop()
{
    delRead(context);
    delWrite(context);
}
void AsyncConnection::delRead(redisAsyncContext * context)
{
    assertex(context);
    assertex(context->ev.data);
    CriticalBlock block(crit);
    redisLibevDelRead(context->ev.data);//this is a hiredis function
}
void AsyncConnection::delWrite(redisAsyncContext * context)
{
    assertex(context);
    assertex(context->ev.data);
    CriticalBlock block(crit);
    redisLibevDelWrite(context->ev.data);//this is a hiredis function
}
void SubscriptionThread::join()
{
    stopEvLoop();
    //CriticalBlock block(crit);
    thread.join();//timeout?
    if (exceptionThrown)
        ::rtlFail(exceptionCode, exceptionMessage.str());
}
void SubscriptionThread::assertSemaphoreTimeout(bool didNotTimeout)
{
    if (!didNotTimeout)
        rtlFail(0, "RedisPlugin : internal subscription thread timed out");
}
void SubscriptionThread::waitForPublication(MemoryAttr * value)
{
    waitForPublication();
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
    assertex(privdata);
    EvDataContainer * container = static_cast<EvDataContainer*>(privdata);
    ev_timer_stop(container->eventLoop, container->timer);
}
void AsyncConnection::selectDb(ICodeContext * ctx)
{
    if (database == 0)//connections are not cached so only check that not default rather than against previous connection selected database
        return;
    VStringBuffer cmd("SELECT %" I64F "u", database);
    sendCommandAndWait(ctx, EV_DEFAULT, timeoutCB, (void*)this, 0, NULL, selectCB, NULL, cmd.str());
}
void AsyncConnection::authenticate(ICodeContext * ctx, const char * password)
{
    if (strlen(password) > 0)
        sendCommandAndWait(ctx, EV_DEFAULT, timeoutCB, (void*)this, 0, NULL, authenticationCB, NULL, "AUTH %b", password, strlen(password));
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
    assertex(context);
    if (context->err)
    {
        VStringBuffer msg("Redis Plugin : failed to create connection context for %s:%d - %s", server->getIp(), server->getPort(), context->errstr);
        rtlFail(0, msg.str());
    }

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
void AsyncConnection::assertRedisErr(int reply, const char * _msg)
{
    if (reply != REDIS_OK)
    {
        VStringBuffer msg("Redis Plugin: %s", _msg);
        rtlFail(0, msg.str());
    }
}
void AsyncConnection::assertCallbackError(redisAsyncContext * context, const redisReply * reply, const char * _msg)
{
    assertex(context);
    assertex(context->data);
    AsyncConnection * connection = static_cast<AsyncConnection*>(context->data);
    if (!reply)
    {
        VStringBuffer msg("Redis Plugin: %s - NULL reply", _msg);
        connection->rtlFail(0, msg.str());
    }
    else if (reply->type == REDIS_REPLY_ERROR)
    {
        VStringBuffer msg("Redis Plugin: %s - %s", _msg, reply->str);
        connection->rtlFail(0, msg.str());
    }
}
void SubscriptionThread::assertRedisErr(int reply, const char * _msg)
{
    if (reply != REDIS_OK)
    {
        VStringBuffer msg("Redis Plugin: %s", _msg);
        rtlFail(0, msg.str());
    }
}
void SubscriptionThread::rtlFail(int code, const char * msg)
{
    CriticalBlock block(crit);
    if(exceptionThrown)//only report first
        return;
    exceptionCode = code;
    exceptionMessage.set(msg, strlen(msg));
    exceptionThrown = true;
}
//callbacks-----------------------------------------------------------------
void AsyncConnection::timeoutCB(struct ev_loop * evLoop, ev_timer *w, int revents)
{
    assertex(w);
    EvDataContainer * container = (EvDataContainer*)w->data;
    StringBuffer msg("Redis Plugin : async operation timed out");
    if (container)
    {
        msg.append(" - ").append(container->exceptionMessage.str());
        if (container->data)
        {
            AsyncConnection * connection = static_cast<AsyncConnection*>(container->data);
            connection->rtlFail(container->exceptionCode, msg.str());
        }
    }
    throwUnexpected();
}
void SubscriptionThread::StTimeoutCB(struct ev_loop * evLoop, ev_timer *w, int revents)
{
    assertex(w);
    EvDataContainer * container = (EvDataContainer*)w->data;
    assertex(container);
    assertex(container->data);

    StringBuffer msg("Redis Plugin : async operation timed out");
    msg.append(" - ").append(container->exceptionMessage.str());
    SubscriptionThread * thread = (SubscriptionThread*)container->data;
    thread->rtlFail(0, msg.str());
    thread->stopEvLoop();
}
void AsyncConnection::hiredisLessThan011ConnectCB(const redisAsyncContext * context)
{
    assertex(context);
    if (context->err)
    {
        if (context->data)
        {
            const AsyncConnection * connection = (AsyncConnection *)context->data;
            VStringBuffer msg("Redis Plugin : failed to connect to %s:%d - %s", connection->ip(), connection->port(), context->errstr);
            connection->rtlFail(0, msg.str());
        }
        else
        {
            VStringBuffer msg("Redis Plugin : failed to connect - %s", context->errstr);
            ::rtlFail(0, msg.str());
        }
    }
}
void AsyncConnection::connectCB(const redisAsyncContext * context, int status)
{
    assertex(context);
    if (status != REDIS_OK)
    {
        if (context->data)
        {
            const AsyncConnection * connection = (AsyncConnection *)context->data;
            VStringBuffer msg("Redis Plugin : failed to connect to %s:%d - %s", connection->ip(), connection->port(), context->errstr);
            connection->rtlFail(0, msg.str());
        }
        else
        {
            VStringBuffer msg("Redis Plugin : failed to connect - %s", context->errstr);
            ::rtlFail(0, msg.str());
        }
    }
}
void AsyncConnection::disconnectCB(const redisAsyncContext * context, int status)
{
    assertex(context);
    if (status != REDIS_OK)
    {
        if (context->data)
        {
            const AsyncConnection  * connection = (AsyncConnection*)context->data;
            VStringBuffer msg("Redis Plugin : server (%s:%d) forced disconnect - %s", connection->ip(), connection->port(), context->errstr);
            connection->rtlFail(0, msg.str());
        }
        else
        {
            VStringBuffer msg("Redis Plugin : server forced disconnect - %s", context->errstr);
            ::rtlFail(0, msg.str());
        }
    }
}
void SubscriptionThread::subCB(redisAsyncContext * context, void * _reply, void * privdata)
{
    if (!_reply || !privdata)//look into needing this, it's important. e.g. hwat happens when redis closes an connection with a sub on it
        return;
    redisReply * reply = (redisReply*)_reply;

    assertCallbackError(context, reply, "subscription thread sub callback fail");

    if (reply->type == REDIS_REPLY_ARRAY && privdata)
    {
        EvDataContainer * container = (EvDataContainer*)privdata;
        Owned<SubscriptionThread> subscriber = LINK((SubscriptionThread*)container->data);
        if (subscriber)
        {
            if (strcmp("subscribe", reply->element[0]->str) == 0 )//Upon subscribing redis replies with an 'OK' acknowledging the subscription
            {
                subscriber->signalSubscriptionActivated();
            }
            else if (strcmp("message", reply->element[0]->str) == 0 )//Any publication will be tagged as a 'message'
            {
                subscriber->setMessage(reply->element[2]->str);
                subscriber->signalPublicationReceived();

                //const char * channel = reply->element[1]->str;//We could extract the channel from 'SubscriptionThread * subscriber'
                //redisAsyncCommand(context, NULL, NULL, "UNSUBSCRIBE %b", channel, strlen(channel));
                //redisAsyncHandleWrite(context);//send unsubscribe command but don't bother to wait as we close socket read event below

                delRead(context);
            }
        }
    }
}
void AsyncConnection::unsubCB(redisAsyncContext * context, void * _reply, void * privdata)
{
    EvDataContainer::stopTimeoutTimer(privdata);
    redisReply * reply = (redisReply*)_reply;
    const char * msg = "unsub callback fail";
    assertCallbackError(context, reply, msg);
    delRead(context);
}
void AsyncConnection::getCB(redisAsyncContext * context, void * _reply, void * privdata)
{
    EvDataContainer::stopTimeoutTimer(privdata);
    redisReply * reply = (redisReply*)_reply;
    const char * msg =  "get callback fail";
    assertCallbackError(context, reply, msg);

    EvDataContainer * container = (EvDataContainer*)privdata;
    if (container && container->data)
    {
        MemoryAttr * retVal = (MemoryAttr*)container->data;
        retVal->set((size32_t)reply->len, (const void*)reply->str);
    }
    delRead(context);
}
void AsyncConnection::setCB(redisAsyncContext * context, void * _reply, void * privdata)
{
    EvDataContainer::stopTimeoutTimer(privdata);
    redisReply * reply = (redisReply*)_reply;
    const char * msg = "set callback fail";
    assertCallbackError(context, reply, msg);
    delRead(context);
}
void AsyncConnection::setLockCB(redisAsyncContext * context, void * _reply, void * privdata)
{
    EvDataContainer::stopTimeoutTimer(privdata);
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
    delRead(context);
}
void AsyncConnection::selectCB(redisAsyncContext * context, void * _reply, void * privdata)
{
    EvDataContainer::stopTimeoutTimer(privdata);
    redisReply * reply = (redisReply*)_reply;
    const char * msg = "select callback fail";
    assertCallbackError(context, reply, msg);
    delRead(context);
}
void AsyncConnection::authenticationCB(redisAsyncContext * context, void * _reply, void * privdata)
{
    EvDataContainer::stopTimeoutTimer(privdata);
    redisReply * reply = (redisReply*)_reply;
    const char * msg = "password authentication callback fail";
    assertCallbackError(context, reply, msg);
    delRead(context);
}
void AsyncConnection::pubCB(redisAsyncContext * context, void * _reply, void * privdata)
{
    EvDataContainer::stopTimeoutTimer(privdata);
    redisReply * reply = (redisReply*)_reply;
    const char * msg = "pub callback fail";
    assertCallbackError(context, reply, msg);
    delRead(context);
}
//end of callbacks------------------------------------------------------
bool AsyncConnection::missThenLock(ICodeContext * ctx, const char * key)
{
    StringBuffer channel;
    encodeChannel(channel, key);
    return lock(ctx, key, channel);
}
void AsyncConnection::ensureEventLoopAttached(struct ev_loop * eventLoop)
{
    CriticalBlock block(crit);
    assertex(context);
    if (!context->ev.data)
    {
        assertRedisErr(redisLibevAttach(eventLoop, context), "failed to attach evLoop to context");
        return;
    }

    redisLibevEvents * contextEv = reinterpret_cast<redisLibevEvents*>(context->ev.data);

    if (contextEv->loop == eventLoop)
        return;//correct loop already attached
    else
    {
        redisLibevCleanup(context->ev.data);//delete any outstanding events on the old and then free
        context->ev.data = NULL;//required otherwise redisLibevAttach() (below) will fail
        assertRedisErr(redisLibevAttach(eventLoop, context), "failed to attach new evLoop to context");
    }
}
void SubscriptionThread::ensureEventLoopAttached(struct ev_loop * eventLoop)
{
    CriticalBlock block(crit);
    assertex(context);
    redisLibevEvents * contextEv = reinterpret_cast<redisLibevEvents*>(context->ev.data);
    struct ev_loop * contextEvLoop = NULL;

    //is the requested loop attached to our redis context?
    if (contextEv)
    {
        if (contextEv->loop != eventLoop)
        {
            contextEvLoop = contextEv->loop;//keep required of old so as to only free the once
            redisLibevCleanup(contextEv);
            context->ev.data = NULL;
            assertRedisErr(redisLibevAttach(eventLoop, context), "failed to attach new evLoop to context");
        }
    }
    else
    {
        assertRedisErr(redisLibevAttach(eventLoop, context), "failed to attach new evLoop to context");
    }

    //is the requested loop the one stored?
    if (evLoop)
    {
        if (evLoop != eventLoop)
        {
            if (evLoop != contextEvLoop)//was this freed above?
                redisLibevCleanup(evLoop);
            evLoop = eventLoop;
        }
    }
    else
        evLoop = eventLoop;
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
    sendCommandAndWait(ctx, EV_DEFAULT, timeoutCB, (void*)this, 0, NULL, setLockCB, (void*)&locked, cmd.str(), key, strlen(key), channel, strlen(channel));
    return locked;
}
void AsyncConnection::handleLockOnGet(ICodeContext * ctx, const char * key, MemoryAttr * retVal, const char * password)
{
    StringBuffer channel;
    encodeChannel(channel, key);
    Owned<SubscriptionThread> subscriptionThread = new SubscriptionThread(ctx, server.getLink(), channel.str(), NULL, database, password, timeout);
    subscriptionThread->startThread();//subscribe and wait for 1st callback that redis received sub. Do not block for main message callback this is the point of the thread.

    //GET command
    sendCommandAndWait(ctx, EV_DEFAULT, timeoutCB, (void*)this, 0, NULL, getCB, (void*)retVal, "GET %b", key, strlen(key));

    //check if value is locked
    if (strncmp(reinterpret_cast<const char*>(retVal->get()), REDIS_LOCK_PREFIX, strlen(REDIS_LOCK_PREFIX)) == 0 )
        subscriptionThread->waitForPublication(retVal);//locked so subscribe for value
    subscriptionThread->join();
}
void AsyncConnection::handleLockOnSet(ICodeContext * ctx, const char * key, const char * value, size_t size, unsigned expire)
{
    StringBuffer cmd("SET %b %b");
    RedisPlugin::appendExpire(cmd, expire);

    //Due to locking logic surfacing into ECL, any locking.set (such as this is) assumes that they own the lock and therefore go ahead and set
    //It is possible for a process/call to 'own' a lock and store this info in the LockObject, however, this prevents sharing between clients.
    sendCommandAndWait(ctx, EV_DEFAULT, timeoutCB, (void*)this, 0, NULL, setCB, NULL, cmd.str(), key, strlen(key), value, size);

    StringBuffer channel;
    encodeChannel(channel, key);
    //this only waits to ensure redis received pub cmd. MORE: reply contains number of subscribers on that channel - utilise this?
    sendCommandAndWait(ctx, EV_DEFAULT, timeoutCB, (void*)this, 0, NULL, pubCB, NULL, "PUBLISH %b %b", channel.str(), channel.length(), value, size);
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
