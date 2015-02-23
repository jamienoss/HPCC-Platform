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

class AsyncConnection : public Connection
{
public :
    AsyncConnection(ICodeContext * ctx, const char * _options, unsigned __int64 _database, const char * password, unsigned __int64 timeout);
    AsyncConnection(ICodeContext * ctx, RedisServer * _server, unsigned __int64 _database, const char * password, unsigned __int64 timeout);
    virtual ~AsyncConnection();

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
    void ensureEventLoop();
    bool lock(const char * key, const char * channel);
    void awaitResponceViaEventLoop(struct ev_loop * evLoop);
    void encodeChannel(StringBuffer & channel, const char * key) const;
    void authenticate(ICodeContext * ctx, const char * password);

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
    void wait() { assertTimeout(messageSemaphore.wait(timeout)); }
    void signal() { messageSemaphore.signal(); }
    void activateSubscriptionSemaphore() { activeSubscriptionSemaphore.signal(); }
    void waitForSubscriptionActivation() { assertTimeout(activeSubscriptionSemaphore.wait(timeout)); }
    void subscribe(struct ev_loop * evLoop);
    void unsubscribe();
    void waitForMessage(MemoryAttr * value);
    void start();
    void stopEvLoop();
    static void subCB(redisAsyncContext * context, void * _reply, void * privdata);

    IMPLEMENT_IINTERFACE;

private :
    void main();
    void assertTimeout(bool timedout);

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
    stopEvLoop();
    wait();//stopEvLoop() is an async operation in stopping any current read/write listening. We need to wait for this before closing this thread.
    if (context)
    {
        //redis can auto disconnect upon certain errors, disconnectCB is called to handle this and is automatically
        //disconnected after this freeing the context
        if (context->err == REDIS_OK)//prevent double free due to the above reason
            redisAsyncDisconnect(context);
    }
    thread.stopped.signal();
    thread.join();

}
void SubscriptionThread::start()
{
    thread.start();
    waitForSubscriptionActivation();//wait for subscription to be acknowledged by redis
}
void SubscriptionThread::stopEvLoop()
{
    CriticalBlock block(crit);
    context->ev.delRead((void*)context->ev.data);
    context->ev.delWrite((void*)context->ev.data);
}
void SubscriptionThread::assertTimeout(bool timedout)
{
    if (timedout)
    {
    	printf("timedout\n");
        rtlFail(0, "RedisPlugin : subscription timed out");
    }
}
void SubscriptionThread::main()
{
    evLoop = ev_loop_new(0);
    subscribe(evLoop);
    signal();
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
    ensureEventLoop();
    VStringBuffer cmd("SELECT %" I64F "u", database);
    assertRedisErr(redisAsyncCommand(context, selectCB, NULL, cmd.str()), "SELECT (lock) buffer write error");
    awaitResponceViaEventLoop(EV_DEFAULT);
}
void AsyncConnection::authenticate(ICodeContext * ctx, const char * password)
{
    if (strlen(password) > 0)
    {
        assertRedisErr(redisAsyncCommand(context, authenticationCB, NULL, "AUTH %b", password, strlen(password)), "AUTH buffer write error");
        awaitResponceViaEventLoop(EV_DEFAULT);
    }
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
void AsyncConnection::awaitResponceViaEventLoop(struct ev_loop * evLoop)
{
    ev_tstamp _timeout = timeout/1000000;
    ev_timer timer;
    ev_timer_init(&timer, timeoutCB, _timeout, 0.);
    ev_timer_again(evLoop, &timer);
    //the above creates a timeout timer for the main event loop (below)
    ev_run(evLoop, 0);//the actual event loop
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
        if (strcmp("subscribe", reply->element[0]->str) == 0 )
            subscriber->activateSubscriptionSemaphore();
        else if (strcmp("message", reply->element[0]->str) == 0 )
        {
            subscriber->setMessage(reply->element[2]->str);
            const char * channel = reply->element[1]->str;
            redisAsyncCommand(context, NULL, NULL, "UNSUBSCRIBE %b", channel, strlen(channel));
            redisAsyncHandleWrite(context);//send unsubscribe command but don't bother to wait as we close socket read event below, which unregisters this callback
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
}
void AsyncConnection::pubCB(redisAsyncContext * context, void * _reply, void * privdata)
{
    redisReply * reply = (redisReply*)_reply;
    assertCallbackError(context, reply, "get callback fail");
    redisLibevDelRead((void*)context->ev.data);
}
void AsyncConnection::sendUnsubscribeCommand(const char * channel)
{
    ensureEventLoop();
    assertRedisErr(redisAsyncCommand(context, NULL, NULL, "UNSUBSCRIBE %b", channel, strlen(channel)), "UNSUBSCRIBE buffer write error");
    redisAsyncHandleWrite(context);
}
/*void AsyncConnection::subscribe(const char * channel, StringAttr & value)
{
    ensureEventLoop();
    assertRedisErr(redisAsyncCommand(context, subCB, (void*)&value, "SUBSCRIBE %b", channel, strlen(channel)), "SUBSCRIBE buffer write error");
    awaitResponceViaEventLoop(EV_DEFAULT);
}*/
void SubscriptionThread::subscribe(struct ev_loop * evLoop)
{
    assertRedisErr(redisLibevAttach(evLoop, context), "failure to attach to libev");
    assertRedisErr(redisAsyncCommand(context, callback, (void*)this, "SUBSCRIBE %b", channel.str(), channel.length()), "SUBSCRIBE buffer write error");
    awaitResponceViaEventLoop(evLoop);
}
void SubscriptionThread::unsubscribe()
{
    ensureEventLoop();
    assertRedisErr(redisAsyncCommand(context, NULL, NULL, "UNSUBSCRIBE %b", channel.str(), channel.length()), "UNSUBSCRIBE buffer write error");
    redisAsyncHandleWrite(context);
}
bool AsyncConnection::missThenLock(ICodeContext * ctx, const char * key)
{
    StringBuffer channel;
    encodeChannel(channel, key);
    return lock(key, channel);
}
void AsyncConnection::ensureEventLoop()
{
    if (context->ev.data)
        return;//already attached
    assertRedisErr(redisLibevAttach(EV_DEFAULT_ context), "failure to attach to libev");//attach and report on fail
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
    ensureEventLoop();
    assertRedisErr(redisAsyncCommand(context, setLockCB, (void*)&locked, cmd.str(), key, strlen(key), channel, strlen(channel)), "SET NX (lock) buffer write error");
    awaitResponceViaEventLoop(EV_DEFAULT);
    return locked;
}
void AsyncConnection::handleLockOnGet(ICodeContext * ctx, const char * key, MemoryAttr * retVal, const char * password)
{
    StringBuffer channel;
    encodeChannel(channel, key);
    Owned<SubscriptionThread> subscriptionThread = new SubscriptionThread(ctx, server.getLink(), channel.str(), NULL, database, password, timeout);
    subscriptionThread->start();//subscribe and wait for 1st callback that redis received sub. Do not block for main message callback this is the point of the thread.

    //GET command
    ensureEventLoop();
    assertRedisErr(redisAsyncCommand(context, getCB, (void*)retVal, "GET %b", key, strlen(key)), "GET buffer write error");
    awaitResponceViaEventLoop(EV_DEFAULT);

    //check if value is locked
    if (strncmp(static_cast<const char*>(retVal->get()), REDIS_LOCK_PREFIX, strlen(REDIS_LOCK_PREFIX)) == 0 )
        subscriptionThread->waitForMessage(retVal);//locked so subscribe for value
}
void AsyncConnection::handleLockOnSet(ICodeContext * ctx, const char * key, const char * value, size_t size, unsigned expire)
{
    StringBuffer cmd("SET %b %b");
    RedisPlugin::appendExpire(cmd, expire);
    ensureEventLoop();

    //Due to locking logic surfacing into ECL, any locking.set (such as this is) assumes that they own the lock and therefore go ahead and set
    //It is possible for a process/call to 'own' a lock and store this info in the LockObject, however, this prevents sharing between clients.
    assertRedisErr(redisAsyncCommand(context, setCB, NULL, cmd.str(), key, strlen(key), value, size), "SET buffer write error");
    awaitResponceViaEventLoop(EV_DEFAULT);

    StringBuffer channel;
    encodeChannel(channel, key);
    assertRedisErr(redisAsyncCommand(context, pubCB, NULL, "PUBLISH %b %b", channel.str(), channel.length(), value, size), "PUBLISH buffer write error");
    awaitResponceViaEventLoop(EV_DEFAULT);//this only waits to ensure redis received pub cmd. MORE: reply contains number of subscribers on that channel - utilise this?
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
