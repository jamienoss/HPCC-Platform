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
    EvDataContainer() : eventLoop(NULL), timer(NULL), data(NULL) { }
    EvDataContainer(struct ev_loop * _eventLoop, ev_timer * _timer, void * _data) : eventLoop(_eventLoop), timer(_timer), data(_data) { }
    ~EvDataContainer() { eventLoop = NULL; timer = NULL; data = NULL; }

public :
    struct ev_loop * eventLoop;
    ev_timer * timer;
    void * data;
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
    static void assertContextErr(const redisAsyncContext * context);//public so can be called from callbacks

protected :
    virtual void selectDb(ICodeContext * ctx);
    virtual void assertOnError(const redisReply * reply, const char * _msg);
    virtual void assertConnection();
    void createAndAssertConnection(ICodeContext * ctx, const char * password);
    void assertRedisErr(int reply, const char * _msg);
    void handleLockOnGet(ICodeContext * ctx, const char * key, MemoryAttr * retVal, const char * password);
    void handleLockOnSet(ICodeContext * ctx, const char * key, const char * value, size_t size, unsigned expire);
    //void subscribe(const char * channel, StringAttr & value);
    void sendUnsubscribeCommand(const char * channel);
    void ensureEventLoop(struct ev_loop * eventLoop);
    bool lock(const char * key, const char * channel);
    void awaitResponceViaEventLoop(struct ev_loop * evLoop, void * data, EvTimeoutCBFn * callback);
    void encodeChannel(StringBuffer & channel, const char * key) const;
    void authenticate(ICodeContext * ctx, const char * password);
    void sendCommandAndWait(ICodeContext * ctx, struct ev_loop * eventLoop, EvTimeoutCBFn * timeoutCallback, void * timeoutData, const char * exceptionMessage, \
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
    void wait() { assertSemaphoreTimeout(messageSemaphore.wait(timeout/1000)); }//timeout is in micro-secs and wait() requires ms
    void signal() { messageSemaphore.signal(); }
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

    IMPLEMENT_IINTERFACE;

protected :
    void main();
    void assertSemaphoreTimeout(bool timedout);
    void join();

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
    thread.start();//This
    waitForSubscriptionActivation();//wait for subscription to be acknowledged by redis
}
void SubscriptionThread::stopEvLoop()
{
    if(context)
    {
        context->ev.delRead((void*)context->ev.data);
        context->ev.delWrite((void*)context->ev.data);
        //wait();//stopEvLoop() is an async operation in stopping any current read/write listening. We need to wait for this before closing this thread.
    }
}
void SubscriptionThread::join()
{
    stopEvLoop();
    thread.join();
}
void SubscriptionThread::assertSemaphoreTimeout(bool didNotTimeout)
{
    if (!didNotTimeout)
    {
    	printf("timedout\n");
        this->rtlFail(0, "RedisPlugin : subscription timed out");
    }
}
void SubscriptionThread::main()
{
    evLoop = ev_loop_new(0);
    ensureEventLoop(evLoop);//this also attaches the loop to the hiredis adapters
    assertRedisErr(redisAsyncCommand(context, callback, (void*)this, "SUBSCRIBE %b", channel.str(), channel.length()), "SUBSCRIBE buffer write error");
   // awaitResponceViaEventLoop(evLoop, static_cast<void*>(this), StTimeoutCB);
    ev_run(evLoop, 0);//wait for response WITHOUT a timeout. This is manually stopped in ~SubscriptionThread() from the parent in handleLockOnGet
}
void SubscriptionThread::waitForMessage(MemoryAttr * value)
{
    wait();
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
void AsyncConnection::selectDb(ICodeContext * ctx)
{
    if (database == 0)//connections are not cached so only check that not default rather than against previous connection selected database
        return;
    ensureEventLoop(EV_DEFAULT);
    VStringBuffer cmd("SELECT %" I64F "u", database);
    assertRedisErr(redisAsyncCommand(context, selectCB, NULL, cmd.str()), "SELECT (lock) buffer write error");
    awaitResponceViaEventLoop(EV_DEFAULT, NULL, timeoutCB);
}
void AsyncConnection::authenticate(ICodeContext * ctx, const char * password)
{
    if (strlen(password) > 0)
    {
        sendCommandAndWait(ctx, EV_DEFAULT, timeoutCB, NULL, "AUTH buffer write error", authenticationCB, NULL, "AUTH %b", password, strlen(password));
        //assertRedisErr(redisAsyncCommand(context, authenticationCB, NULL, "AUTH %b", password, strlen(password)), "AUTH buffer write error");
        //awaitResponceViaEventLoop(EV_DEFAULT, NULL, timeoutCB);
    }
}
void AsyncConnection::sendCommandAndWait(ICodeContext * ctx, struct ev_loop * eventLoop, EvTimeoutCBFn * timeoutCallback, void * timeoutData, const char * exceptionMessage, \
        redisCallbackFn * callback, void * callbackData, const char * command, ...)
{
	printf("%s\n", exceptionMessage);
    ensureEventLoop(eventLoop);

    ev_timer timer;
    EvDataContainer container(eventLoop, &timer, callbackData);

    timer.data = timeoutData;//Make accessible from EvTimeoutCBFn * timeoutCallback.

    va_list arguments;
    va_start(arguments, command);
    assertRedisErr(redisvAsyncCommand(context, callback, static_cast<void*>(&container), command, arguments), exceptionMessage);
    va_end(arguments);

    ev_tstamp evTimeout = timeout/1000000;//timeout has units micro-seconds & ev_tstamp has that of seconds.
    ev_timer_init(&timer, timeoutCallback, evTimeout, 0.);
    //ev_timer_again(eventLoop, &timer);
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
    assertConnection();
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
void AsyncConnection::assertConnection()
{
    if (!context)
        rtlFail(0, "Redis Plugin: async context memory allocation fail.");
    assertContextErr(context);
}
void AsyncConnection::assertContextErr(const redisAsyncContext * context)
{
    if (context && context->err)//NOTE: This would cause issues if caching connections and a non rtFail error was observed. Would require resetting context->err upon such occurrences
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
        assertConnection();
        //There should always be a redis context error but in case there is not report via the following
        VStringBuffer msg("Redis Plugin: %s%s", _msg, "no 'reply' nor connection error");
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
    if (reply && reply->type == REDIS_REPLY_ERROR)
    {
        VStringBuffer msg("Redis Plugin: %s - %s", _msg, reply->str);
        rtlFail(0, msg.str());
    }
    assertContextErr(context);
}
void AsyncConnection::timeoutCB(struct ev_loop * evLoop, ev_timer *w, int revents)
{
    rtlFail(0, "Redis Plugin : async operation timed out");
}
void SubscriptionThread::StTimeoutCB(struct ev_loop * evLoop, ev_timer *w, int revents)
{
	printf("in timeout\n");
    if (w->data)
    {
        SubscriptionThread * thread = static_cast<SubscriptionThread*>(w->data);
        thread->rtlFail(0, "Redis Plugin : async operation timed out");
    }
    //should still do something?
}
void SubscriptionThread::rtlFail(int code, const char * msg)
{
    join();
    rtlFail(code, msg);
}
void AsyncConnection::awaitResponceViaEventLoop(struct ev_loop * eventLoop, void * data, EvTimeoutCBFn * callback)
{
    ev_tstamp evTimeout = timeout/1000000;//timeout has units micro-seconds & ev_tstamp has that of seconds.
    ev_timer timer;
    timer.data = data;//Accessible from EvTimeoutCBFn * callback.
    ev_timer_init(&timer, callback, evTimeout, 0.);
    //ev_timer_again(eventLoop, &timer);
    ev_timer_start(eventLoop, &timer);
    printf("here\n");
    //the above creates a timeout timer for the main event loop below
    ev_run(eventLoop, 0);//the actual event loop
    printf("loop over\n");
    ev_timer_stop(eventLoop, &timer);
}
//callbacks-----------------------------------------------------------------
void AsyncConnection::hiredisLessThan011ConnectCB(const redisAsyncContext * context)
{
    if (context && context->err)
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
    if (_reply == NULL || privdata == NULL)
        return;

    redisReply * reply = (redisReply*)_reply;
    assertCallbackError(context, reply, "callback fail");

    if (reply->type == REDIS_REPLY_ARRAY)
    {
        SubscriptionThread * subscriber = (SubscriptionThread*)privdata;
        if (strcmp("subscribe", reply->element[0]->str) == 0 )//Upon subscribing redis replies with an 'OK' acknowledging the subscription
            subscriber->activateSubscriptionSemaphore();//Signal that the subscription has been acknowledged
        else if (strcmp("message", reply->element[0]->str) == 0 )//Any publication will be tagged as a 'message'
        {
            subscriber->setMessage(reply->element[2]->str);
            const char * channel = reply->element[1]->str;//We could extract the channel from 'SubscriptionThread * subscriber'
            redisAsyncCommand(context, NULL, NULL, "UNSUBSCRIBE %b", channel, strlen(channel));
            redisAsyncHandleWrite(context);//send unsubscribe command but don't bother to wait as we close socket read event below
            context->ev.delRead((void*)context->ev.data);
        }
    }
}
/*void AsyncConnection::subCB(redisAsyncContext * context, void * _reply, void * privdata)
{
    if (_reply == NULL)
        return;

    redisReply * reply = (redisReply*)_reply;
    assertCallbackError(context, reply, "callback fail");

    if (reply->type == REDIS_REPLY_ARRAY && strcmp("message", reply->element[0]->str) == 0 )
    {
        ((StringAttr*)privdata)->set(reply->element[2]->str);
        const char * channel = reply->element[1]->str;
        redisAsyncCommand(context, NULL, NULL, "UNSUBSCRIBE %b", channel, strlen(channel));
        redisLibevDelRead((void*)context->ev.data);
    }
}*/
void AsyncConnection::unsubCB(redisAsyncContext * context, void * _reply, void * privdata)
{
    redisReply * reply = (redisReply*)_reply;
    assertCallbackError(context, reply, "get callback fail");
    redisLibevDelRead((void*)context->ev.data);
}
void AsyncConnection::getCB(redisAsyncContext * context, void * _reply, void * privdata)
{
    redisReply * reply = (redisReply*)_reply;
    assertCallbackError(context, reply, "get callback fail");

    MemoryAttr * retVal = (MemoryAttr*)privdata;
    retVal->set((size32_t)reply->len, (const void*)reply->str);
    redisLibevDelRead((void*)context->ev.data);
}
void AsyncConnection::setCB(redisAsyncContext * context, void * _reply, void * privdata)
{
    redisReply * reply = (redisReply*)_reply;
    assertCallbackError(context, reply, "set callback fail");
    redisLibevDelRead((void*)context->ev.data);
}
void AsyncConnection::setLockCB(redisAsyncContext * context, void * _reply, void * privdata)
{
    redisReply * reply = (redisReply*)_reply;
    assertCallbackError(context, reply, "set lock callback fail");

    //Redis, with SET NX, will return a REDIS_REPLY_STATUS with an 'OK' message upon a successful SET.
    //If the key was already present then (and thus not SET) REDIS_REPLY_NIL is used instead.
    switch(reply->type)
    {
    case REDIS_REPLY_STATUS :
        *(bool*)privdata = strcmp(reply->str, "OK") == 0;
        break;
    case REDIS_REPLY_NIL :
        *(bool*)privdata = false;
    }
    redisLibevDelRead((void*)context->ev.data);
}
void AsyncConnection::selectCB(redisAsyncContext * context, void * _reply, void * privdata)
{
    redisReply * reply = (redisReply*)_reply;
    assertCallbackError(context, reply, "select callback fail");
    redisLibevDelRead((void*)context->ev.data);
}
void AsyncConnection::authenticationCB(redisAsyncContext * context, void * _reply, void * privdata)
{
    redisReply * reply = (redisReply*)_reply;
    assertCallbackError(context, reply, "password authentication callback fail");
    redisLibevDelRead((void*)context->ev.data);

    if (privdata)
    {
        EvDataContainer * container = static_cast<EvDataContainer*>(privdata);
        ev_timer_stop(container->eventLoop, container->timer);
    }
}
void AsyncConnection::pubCB(redisAsyncContext * context, void * _reply, void * privdata)
{
    redisReply * reply = (redisReply*)_reply;
    assertCallbackError(context, reply, "get callback fail");
    redisLibevDelRead((void*)context->ev.data);
}
void AsyncConnection::sendUnsubscribeCommand(const char * channel)
{
    ensureEventLoop(EV_DEFAULT);
    assertRedisErr(redisAsyncCommand(context, NULL, NULL, "UNSUBSCRIBE %b", channel, strlen(channel)), "UNSUBSCRIBE buffer write error");
    redisAsyncHandleWrite(context);
}
/*void AsyncConnection::subscribe(const char * channel, StringAttr & value)
{
    ensureEventLoop(EV_DEFAULT);
    assertRedisErr(redisAsyncCommand(context, subCB, (void*)&value, "SUBSCRIBE %b", channel, strlen(channel)), "SUBSCRIBE buffer write error");
    awaitResponceViaEventLoop(EV_DEFAULT);
}*/
/*void SubscriptionThread::subscribe(struct ev_loop * evLoop)
{
    ensureEventLoop(evLoop);
    assertRedisErr(redisAsyncCommand(context, callback, (void*)this, "SUBSCRIBE %b", channel.str(), channel.length()), "SUBSCRIBE buffer write error");
    awaitResponceViaEventLoop(evLoop, (void*)this, SubscriptionThread::timeoutCB);
}*/
void SubscriptionThread::unsubscribe()
{
    ensureEventLoop(evLoop);
    assertRedisErr(redisAsyncCommand(context, NULL, NULL, "UNSUBSCRIBE %b", channel.str(), channel.length()), "UNSUBSCRIBE buffer write error");
    redisAsyncHandleWrite(context);
}
bool AsyncConnection::missThenLock(ICodeContext * ctx, const char * key)
{
    StringBuffer channel;
    encodeChannel(channel, key);
    return lock(key, channel);
}
void AsyncConnection::ensureEventLoop(struct ev_loop * eventLoop)
{
    if (context->ev.data)
        return;//already attached
    assertRedisErr(redisLibevAttach(eventLoop, context), "failure to attach to libev");//attach and report on fail
}
void AsyncConnection::encodeChannel(StringBuffer & channel, const char * key) const
{
    channel.append(REDIS_LOCK_PREFIX).append("_").append(key).append("_").append(database).append("_").append(server->getIp()).append("_").append(server->getPort());
}
bool AsyncConnection::lock(const char * key, const char * channel)
{
    StringBuffer cmd("SET %b %b NX EX ");
    cmd.append(timeout);

    bool locked = false;
    ensureEventLoop(EV_DEFAULT);
    assertRedisErr(redisAsyncCommand(context, setLockCB, (void*)&locked, cmd.str(), key, strlen(key), channel, strlen(channel)), "SET NX (lock) buffer write error");
    awaitResponceViaEventLoop(EV_DEFAULT, NULL, timeoutCB);
    return locked;
}
void AsyncConnection::handleLockOnGet(ICodeContext * ctx, const char * key, MemoryAttr * retVal, const char * password)
{
    StringBuffer channel;
    encodeChannel(channel, key);
    Owned<SubscriptionThread> subscriptionThread = new SubscriptionThread(ctx, server.getLink(), channel.str(), NULL, database, password, timeout);
    subscriptionThread->start();//subscribe and wait for 1st callback that redis received sub. Do not block for main message callback this is the point of the thread.

    //GET command
    ensureEventLoop(EV_DEFAULT);
    assertRedisErr(redisAsyncCommand(context, getCB, (void*)retVal, "GET %b", key, strlen(key)), "GET buffer write error");
    awaitResponceViaEventLoop(EV_DEFAULT, NULL, timeoutCB);

    //check if value is locked
    if (strncmp(static_cast<const char*>(retVal->get()), REDIS_LOCK_PREFIX, strlen(REDIS_LOCK_PREFIX)) == 0 )
        subscriptionThread->waitForMessage(retVal);//locked so subscribe for value
    //subscriptionThread->stopEvLoop();//test case only. Called within ~SubscriptionThread()
}
void AsyncConnection::handleLockOnSet(ICodeContext * ctx, const char * key, const char * value, size_t size, unsigned expire)
{
    StringBuffer cmd("SET %b %b");
    RedisPlugin::appendExpire(cmd, expire);
    ensureEventLoop(EV_DEFAULT);

    //Due to locking logic surfacing into ECL, any locking.set (such as this is) assumes that they own the lock and therefore go ahead and set
    //It is possible for a process/call to 'own' a lock and store this info in the LockObject, however, this prevents sharing between clients.
    assertRedisErr(redisAsyncCommand(context, setCB, NULL, cmd.str(), key, strlen(key), value, size), "SET buffer write error");
    awaitResponceViaEventLoop(EV_DEFAULT, NULL, timeoutCB);

    StringBuffer channel;
    encodeChannel(channel, key);
    assertRedisErr(redisAsyncCommand(context, pubCB, NULL, "PUBLISH %b %b", channel.str(), channel.length(), value, size), "PUBLISH buffer write error");
    awaitResponceViaEventLoop(EV_DEFAULT, NULL, timeoutCB);//this only waits to ensure redis received pub cmd. MORE: reply contains number of subscribers on that channel - utilise this?
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
