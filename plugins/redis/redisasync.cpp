/*##############################################################################

    HPCC SYSTEMS software Copyright (C) 2014 HPCC Systems.

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
#include "hiredis/adapters/libev.h"

#include "platform.h"
#include "eclrtl.hpp"
#include "jstring.hpp"
#include "jsem.hpp"
#include "jsocket.hpp"
#include "jthread.hpp"
#include "jmutex.hpp"
#include "redisplugin.hpp"
#include "redissync.hpp"
#include "redisasync.hpp"

#include "hiredis/async.h"

namespace Lock
{
static CriticalSection crit;
static const char * REDIS_LOCK_PREFIX = "redis_ecl_lock";// needs to be a large random value uniquely individual per client
static const unsigned REDIS_LOCK_EXPIRE = 60; //(secs)
static const unsigned REDIS_MAX_LOCKS = 9999;

class KeyLock : public CInterface
{
public :
    KeyLock(const char * _options, const char * _key, const char * _lock);
    ~KeyLock();

    inline const char * getKey()     const { return key.str(); }
    inline const char * getOptions() const { return options.str(); }
    inline const char * getChannel()  const { return channel.str(); }

private :
    StringAttr options; //shouldn't be needed, tidy isSameConnection to pass 'master' & 'port'
    StringAttr master;
    int port;
    StringAttr key;
    StringAttr channel;
};
KeyLock::KeyLock(const char * _options, const char * _key, const char * _lockId)
{
    options.setown(_options);
    key.set(_key);
    channel.set(_lockId);
    RedisPlugin::parseOptions(_options, master, port);
}
KeyLock::~KeyLock()
{
    redisContext * context = redisConnectWithTimeout(master, port, RedisPlugin::REDIS_TIMEOUT);
    StringAttr lockIdFound;
    CriticalBlock block(crit);
    OwnedReply reply = RedisPlugin::createReply(redisCommand(context, getCmd, key.str(), strlen(key.str())));
    lockIdFound.setown(reply->query()->str);

    if (strcmp(channel, lockIdFound) == 0)
        OwnedReply reply = RedisPlugin::createReply(redisCommand(context, "DEL %b", key.str(), strlen(key.str())));

    if (context)
        redisFree(context);
}
}//close Lock Namespace

namespace Async
{
static CriticalSection crit;
static const unsigned REDIS_TIMEOUT = 1000;//ms
class SubHolder;

class ReturnValue //: public CInterface
{
public :
    ReturnValue() : size(0), value(NULL) { }
    ~ReturnValue()
    {
        if (value)
            free(value);
    }

    void cpy(size_t _size, const char * _value) { size = _size; value = (char*)malloc(size); memcpy(value, _value, size); }
    inline size_t getSize() const { return size; }
    inline const char * str() const { return value; }
    char * getStr() { char * tmp = value; value = NULL; size = 0; return tmp; }
    inline bool isEmpty() const { return size == 0; }//!value||!*value; }

private :
    size_t size;
    char * value;
};
class Connection : public RedisPlugin::Connection
{
public :
    Connection(ICodeContext * ctx, const char * _options);
    ~Connection();

    //get
    void get(ICodeContext * ctx, const char * key, ReturnValue * retVal, const char * channel);
    //template <class type> void get(ICodeContext * ctx, const char * key, type & value, const char * channel);
    template <class type> void get(ICodeContext * ctx, const char * key, size_t & valueLength, type * & value, const char * channel);
    void getVoidPtrLenPair(ICodeContext * ctx, const char * key, size_t & valueLength, void * & value, const char * channel);
    //set
    template<class type> void set(ICodeContext * ctx, const char * key, type value, unsigned expire, const char * channel);
    template<class type> void set(ICodeContext * ctx, const char * key, size32_t valueLength, const type * value, unsigned expire, const char * channel);

    bool missThenLock(ICodeContext * ctx, Lock::KeyLock * keyPtr);

protected :
    void createAndAssertConnection();
    virtual void assertOnError(const redisReply * reply, const char * _msg);
    virtual void assertConnection();
    void assertRedisErr(int reply, const char * _msg);
    void handleLockForGet(ICodeContext * ctx, const char * key, const char * channel, ReturnValue * retVal);
    void handleLockForSet(ICodeContext * ctx, const char * key, const char * value, size_t size, const char * channel, unsigned expire);
    bool lock(const char * key, const char * channel);
    void subscribe(const char * channel, StringAttr & value);
    void unsubscribe(const char * channel);

    //callbacks
    static void assertCallbackError(const redisReply * reply, const char * _msg);
    static void connectCB(const redisAsyncContext *c, int status);
    static void disconnectCB(const redisAsyncContext *c, int status);
    static void subCB(redisAsyncContext * context, void * _reply, void * privdata);
    static void pubCB(redisAsyncContext * context, void * _reply, void * privdata);
    static void setCB(redisAsyncContext * context, void * _reply, void * privdata);
    static void getCB(redisAsyncContext * context, void * _reply, void * privdata);
    static void unsubCB(redisAsyncContext * context, void * _reply, void * privdata);

protected :
    redisAsyncContext * context;
};

class SubHolder : public Async::Connection
{
public :
    SubHolder(ICodeContext * ctx, const char * options, const char * _channel);
    SubHolder(ICodeContext * ctx, const char * options, const char * _channel, redisCallbackFn * _callback);
    ~SubHolder()
    {
        unsubscribe();
    }

    inline void setCB(redisCallbackFn * _callback) { callback = _callback; }
    inline redisCallbackFn * getCB() { return callback; }
    inline void setChannel(const char * _channel) { channel.set(_channel); }
    inline const char * getChannel() { return channel.str(); }
    inline void setMessage(const char * _message) { message.set(_message); }
    inline const char * getMessage() { return message.str(); }
    inline void wait(unsigned timeout) { msgSem.wait(timeout); }
    inline void signal() { msgSem.signal(); }
    inline void subActivated() { subActiveSem.signal(); }
    inline void subActivationWait(unsigned timeout) { subActiveSem.wait(timeout); }
    void subscribe(struct ev_loop * evLoop);
    void unsubscribe();

    void stopEvLoop()
    {
        CriticalBlock block(crit);
        context->ev.delRead((void*)context->ev.data);
        context->ev.delWrite((void*)context->ev.data);
    }
    //callback
    static void subCB(redisAsyncContext * context, void * _reply, void * privdata);

protected :
    Semaphore msgSem;
    Semaphore subActiveSem;
    redisCallbackFn * callback;
    StringAttr message;
    StringAttr channel;
};
class SubscriptionThread : implements IThreaded, implements IInterface, public SubHolder
{
public :
    SubscriptionThread(ICodeContext * ctx, const char * options, const char * channel) : SubHolder(ctx, options, channel), thread("SubscriptionThread", (IThreaded*)this)
    {
        evLoop = NULL;
        //thread.start();
        //thread.init(pIThreaded);
        subActivationWait(REDIS_TIMEOUT);
    }
    virtual ~SubscriptionThread()
    {
        stopEvLoop();
        wait(REDIS_TIMEOUT);//give stopEvLoop time to complete
        thread.stopped.signal();
        thread.join();
    }

    void start() { thread.start(); }

    IMPLEMENT_IINTERFACE;

private :
    void main()
    {
        evLoop = ev_loop_new(0);
        subscribe(evLoop);
        signal();
    }

private :
    CThreaded  thread;
    struct ev_loop * evLoop;
};
SubHolder::SubHolder(ICodeContext * ctx, const char * options, const char * _channel) : Connection(ctx, options)
{
    channel.set(_channel);
    callback = subCB;
}
SubHolder::SubHolder(ICodeContext * ctx, const char * options, const char * _channel, redisCallbackFn * _callback) : Connection(ctx, options)
{
    channel.set(_channel);
    callback = _callback;
}
Connection::Connection(ICodeContext * ctx, const char * _options) : RedisPlugin::Connection(ctx, _options)
{
    createAndAssertConnection();
}
Connection::~Connection()
{
    if (context)
    {
        //redis can auto disconnect upon certain errors, disconnectCB is called to handle this and is automatically
        //disconnected after this freeing the context
        if (context->err == REDIS_OK)
            redisAsyncDisconnect(context);
    }
}
void Connection::createAndAssertConnection()
{
    context = NULL;
    context = redisAsyncConnect(master.str(), port);
    assertConnection();
    context->data = (void*)this;
    assertRedisErr(redisAsyncSetConnectCallback(context, connectCB), "failed to set connect callback");
    assertRedisErr(redisAsyncSetDisconnectCallback(context, disconnectCB), "failed to set disconnect callback");
}
void Connection::assertConnection()
{
    if (!context)
        rtlFail(0, "Redis Plugin: async context mem alloc fail.");

    if (context->err)
    {
        VStringBuffer msg("Redis Plugin : failed to create connection context for %s:%d - %s", master.str(), port, context->errstr);
        rtlFail(0, msg.str());
    }
}
void Connection::connectCB(const redisAsyncContext * context, int status)
{
    if (status != REDIS_OK)//&& status != 2)
    {
        if (context->data)
        {
            const Connection * connection = (Connection *)context->data;
            VStringBuffer msg("Redis Plugin : failed to connect to %s:%d - %s", connection->master.str(), connection->port, context->errstr);
            rtlFail(0, msg.str());
        }
        else
        {
            VStringBuffer msg("Redis Plugin : failed to connect - %s", context->errstr);
            rtlFail(0, msg.str());
        }
    }
}
void Connection::disconnectCB(const redisAsyncContext * context, int status)
{
    if (status != REDIS_OK && context->err != 2)//err = 2:  ERR only (P)SUBSCRIBE / (P)UNSUBSCRIBE / QUIT allowed in this contex
    {
        if (context->data)
        {
            const Connection  * connection = (Connection*)context->data;
            VStringBuffer msg("Redis Plugin : server (%s:%d) forced disconnect - %s", connection->master.str(), connection->port, context->errstr);
            rtlFail(0, msg.str());
        }
        else
        {
            VStringBuffer msg("Redis Plugin : server forced disconnect - %s", context->errstr);
            rtlFail(0, msg.str());
        }
    }
}

Connection * createConnection(ICodeContext * ctx, const char * options)
{
    return new Connection(ctx, options);
}

//-----------------------------------------------------------------------------
void Connection::assertRedisErr(int reply, const char * _msg)
{
    if (reply != REDIS_OK)
    {
        VStringBuffer msg("Redis Plugin: %s", _msg);
        rtlFail(0, msg.str());
    }
}
void Connection::assertOnError(const redisReply * reply, const char * _msg)
{
    if (!reply)
    {
        assertConnection();
        //There should always be a connection error
        VStringBuffer msg("Redis Plugin: %s%s", _msg, "no 'reply' nor connection error");
        rtlFail(0, msg.str());
    }
    else if (reply->type == REDIS_REPLY_ERROR)
    {
        VStringBuffer msg("Redis Plugin: %s%s", _msg, reply->str);
        rtlFail(0, msg.str());
    }
}
void Connection::assertCallbackError(const redisReply * reply, const char * _msg)
{
    if (reply && reply->type == REDIS_REPLY_ERROR)
    {
        VStringBuffer msg("Redis Plugin: %s%s", _msg, reply->str);
        rtlFail(0, msg.str());
    }
}
//Async callbacks-----------------------------------------------------------------
void SubHolder::subCB(redisAsyncContext * context, void * _reply, void * privdata)
{
    if (_reply == NULL || privdata == NULL)
        return;

    redisReply * reply = (redisReply*)_reply;
    assertCallbackError(reply, "callback fail");

    if (reply->type == REDIS_REPLY_ARRAY)
    {
        Async::SubHolder * holder = (Async::SubHolder*)privdata;
        if (strcmp("subscribe", reply->element[0]->str) == 0 )
        {
            holder->subActivated();
        }
        else if (strcmp("message", reply->element[0]->str) == 0 )
        {
            holder->setMessage(reply->element[2]->str);
            const char * channel = reply->element[1]->str;
            redisAsyncCommand(context, NULL, NULL, "UNSUBSCRIBE %b", channel, strlen(channel));
            redisAsyncHandleWrite(context);
            context->ev.delRead((void*)context->ev.data);
        }
    }
}
void Connection::subCB(redisAsyncContext * context, void * _reply, void * privdata)
{
    if (_reply == NULL)
        return;

    redisReply * reply = (redisReply*)_reply;
    assertCallbackError(reply, "callback fail");

    if (reply->type == REDIS_REPLY_ARRAY && strcmp("message", reply->element[0]->str) == 0 )
    {
        ((StringAttr*)privdata)->set(reply->element[2]->str);
        const char * channel = reply->element[1]->str;
        redisAsyncCommand(context, NULL, NULL, "UNSUBSCRIBE %b", channel, strlen(channel));
        redisLibevDelRead((void*)context->ev.data);
    }
}
void Connection::unsubCB(redisAsyncContext * context, void * _reply, void * privdata)
{
    redisReply * reply = (redisReply*)_reply;
    assertCallbackError(reply, "get callback fail");
    redisLibevDelRead((void*)context->ev.data);
}
void Connection::getCB(redisAsyncContext * context, void * _reply, void * privdata)
{
    redisReply * reply = (redisReply*)_reply;
    assertCallbackError(reply, "get callback fail");

    ReturnValue * retVal = (ReturnValue*)privdata;
    retVal->cpy((size_t)reply->len, reply->str);
    redisLibevDelRead((void*)context->ev.data);
}
void Connection::setCB(redisAsyncContext * context, void * _reply, void * privdata)
{
    redisReply * reply = (redisReply*)_reply;
    assertCallbackError(reply, "set callback fail");
    redisLibevDelRead((void*)context->ev.data);
}
void Connection::pubCB(redisAsyncContext * context, void * _reply, void * privdata)
{
    redisReply * reply = (redisReply*)_reply;
    assertCallbackError(reply, "get callback fail");
    redisLibevDelRead((void*)context->ev.data);
}
void Connection::unsubscribe(const char * channel)
{
    assertRedisErr(redisAsyncCommand(context, NULL, NULL, "UNSUBSCRIBE %b", channel, strlen(channel)), "UNSUBSCRIBE buffer write error");
    redisAsyncHandleWrite(context);
}
void Connection::subscribe(const char * channel, StringAttr & value)
{
    assertRedisErr(redisAsyncCommand(context, subCB, (void*)&value, "SUBSCRIBE %b", channel, strlen(channel)), "SUBSCRIBE buffer write error");
    ev_loop(EV_DEFAULT_ 0);
}
void SubHolder::subscribe(struct ev_loop * evLoop)
{
    assertRedisErr(redisLibevAttach(evLoop, context), "failure to attach to libev");
    assertRedisErr(redisAsyncCommand(context, callback, (void*)this, "SUBSCRIBE %b", channel.str(), channel.length()), "SUBSCRIBE buffer write error");
    ev_loop(evLoop, 0);
}
void SubHolder::unsubscribe()
{
    assertRedisErr(redisAsyncCommand(context, NULL, NULL, "UNSUBSCRIBE %b", channel.str(), channel.length()), "UNSUBSCRIBE buffer write error");
    redisAsyncHandleWrite(context);
}
bool Connection::missThenLock(ICodeContext * ctx, Lock::KeyLock * keyPtr)
{
    return lock(keyPtr->getKey(), keyPtr->getChannel());
}

bool Connection::lock(const char * key, const char * channel)
{
    StringBuffer cmd("SET %b %b NX EX ");
    cmd.append(Lock::REDIS_LOCK_EXPIRE);

    redisContext * c = redisConnect(master, port);
    OwnedReply reply = RedisPlugin::createReply(redisCommand(c, cmd.str(), key, strlen(key), channel, strlen(channel)));
    const redisReply * actReply = reply->query();

    if (!actReply)
    {
        VStringBuffer msg("Redis Plugin: reply failure when locking key '%s' with error - %s", key, c->errstr);
        redisFree(c);
        rtlFail(0, msg.str());
    }
    redisFree(c);

    switch(actReply->type)
    {
    case REDIS_REPLY_STATUS :
        return strcmp(actReply->str, "OK") == 0;
    case REDIS_REPLY_NIL :
        return false;
    case REDIS_REPLY_ERROR :
    {
        VStringBuffer msg("Redis Plugin: failure to lock key '%s' with error - %s", key, actReply->str);
        rtlFail(0, msg.str());
    }
    }
    return false;
}
void Connection::handleLockForGet(ICodeContext * ctx, const char * key, const char * channel, ReturnValue * retVal)
{
    bool ignoreLock = (channel == NULL);
    Owned<SubscriptionThread> subThread;//ctx, options.str(), channel);
    if (!ignoreLock)
    {
        subThread.set(new SubscriptionThread(ctx, options.str(), channel));
        subThread->start();
    }

    assertRedisErr(redisLibevAttach(EV_DEFAULT_ context), "failure to attach to libev");
    assertRedisErr(redisAsyncCommand(context, getCB, (void*)retVal, "GET %b", key, strlen(key)), "GET buffer write error");
    ev_loop(EV_DEFAULT_ 0);

    //Main logic
    if (ignoreLock)
        return;

    if (retVal->isEmpty())
    {
        if (lock(key, channel))
            return;
        else
            subThread->wait(REDIS_TIMEOUT);
    }
    else
    {
        if (strncmp(retVal->str(), Lock::REDIS_LOCK_PREFIX, strlen(Lock::REDIS_LOCK_PREFIX)) == 0 )
            subThread->wait(REDIS_TIMEOUT);
        else
            return;
    }
}
void Connection::handleLockForSet(ICodeContext * ctx, const char * key, const char * value, size_t size, const char * channel, unsigned expire)
{
    StringBuffer cmd(setCmd);
    RedisPlugin::appendExpire(cmd, expire);
    assertRedisErr(redisLibevAttach(EV_DEFAULT_ context), "failure to attach to libev");
    if(channel)
    {
        assertRedisErr(redisAsyncCommand(context, setCB, NULL, cmd.str(), key, strlen(key), value, size), "SET buffer write error");
        ev_loop(EV_DEFAULT_ 0);
        assertRedisErr(redisAsyncCommand(context, pubCB, NULL, "PUBLISH %b %b", channel, strlen(channel), value, size), "PUBLISH buffer write error");
        ev_loop(EV_DEFAULT_ 0);
    }
    else
    {
        ReturnValue retVal;
        StringBuffer cmd2("GETSET %b %b");
        RedisPlugin::appendExpire(cmd2, expire);
        //obtain channel
        assertRedisErr(redisAsyncCommand(context, getCB, (void*)&retVal, cmd2.str(), key, strlen(key), value, size), "SET buffer write error");
        ev_loop(EV_DEFAULT_ 0);

        if (strncmp(retVal.str(), Lock::REDIS_LOCK_PREFIX, strlen(Lock::REDIS_LOCK_PREFIX)) == 0 )
        {
            assertRedisErr(redisAsyncCommand(context, pubCB, NULL, "PUBLISH %b %b", channel, strlen(channel), value, size), "PUBLISH buffer write error");
            ev_loop(EV_DEFAULT_ 0);
        }
    }
}







//----------------------------------GET----------------------------------------
/*template<class type> void Connection::get(ICodeContext * ctx, const char * key, type & returnValue, const char * channel )
{
    ReturnValue retVal;
    handleLock(ctx, key, channel, &retVal);

    StringBuffer keyMsg = getFailMsg;
    //assertOnError(reply->query(), appendIfKeyNotFoundMsg(reply->query(), key, keyMsg));

    size_t returnSize = retVal.getSize();//*sizeof(type);
    if (sizeof(type)!=returnSize)
    {
        VStringBuffer msg("RedisPlugin: ERROR - Requested type of different size (%uB) from that stored (%uB).", (unsigned)sizeof(type), (unsigned)returnSize);
        rtlFail(0, msg.str());
    }
    memcpy(&returnValue, retVal.str(), returnSize);//one less cpy possible by using retVal.getStr()
}
*/
//-------------------------------------------GET-----------------------------------------
//---OUTER---
template<class type> void RGet(ICodeContext * ctx, const char * options, const char * key, type & returnValue, const char * channel = NULL)
{
    Owned<Async::Connection> master = Async::createConnection(ctx, options);
    ReturnValue retVal;
    master->get(ctx, key, &retVal, channel);
    StringBuffer keyMsg = getFailMsg;

    size_t returnSize = retVal.getSize();
    if (sizeof(type)!=returnSize)
    {
        VStringBuffer msg("RedisPlugin: ERROR - Requested type of different size (%uB) from that stored (%uB).", (unsigned)sizeof(type), (unsigned)returnSize);
        rtlFail(0, msg.str());
    }
    memcpy(&returnValue, retVal.str(), returnSize);
}
/*template<class type> void RGet(ICodeContext * ctx, const char * options, const char * key, type & returnValue, const char * channel = NULL)
{
    Owned<Async::Connection> master = Async::createConnection(ctx, options);
    master->get(ctx, key, returnValue, channel);
}*/
template<class type> void RGet(ICodeContext * ctx, const char * options, const char * key, size_t & returnLength, type * & returnValue, const char * channel = NULL)
{
    Owned<Async::Connection> master = Async::createConnection(ctx, options);
    master->get(ctx, key, returnLength, returnValue, channel);
}
void RGetVoidPtrLenPair(ICodeContext * ctx, const char * options, const char * key, size_t & returnLength, void * & returnValue, const char * channel = NULL)
{
    Owned<Async::Connection> master = Async::createConnection(ctx, options);
    master->getVoidPtrLenPair(ctx, key, returnLength, returnValue, channel);
}
//---INNER---
void Connection::get(ICodeContext * ctx, const char * key, ReturnValue * retVal, const char * channel )
{
    handleLockForGet(ctx, key, channel, retVal);
}
template<class type> void Connection::get(ICodeContext * ctx, const char * key, size_t & returnSize, type * & returnValue, const char * channel)
{
    ReturnValue retVal;
    handleLockForGet(ctx, key, channel, &retVal);

    returnSize = retVal.getSize();
    returnValue = reinterpret_cast<type*>(retVal.getStr());
}
void Connection::getVoidPtrLenPair(ICodeContext * ctx, const char * key, size_t & returnSize, void * & returnValue, const char * channel )
{
    ReturnValue retVal;
    handleLockForGet(ctx, key, channel, &retVal);

    returnSize = retVal.getSize();
    returnValue = reinterpret_cast<void*>(cpy(retVal.str(), returnSize));
}
//-------------------------------------------SET-----------------------------------------
//---OUTER---
template<class type> void RSet(ICodeContext * ctx, const char * _options, const char * key, type value, unsigned expire, const char * channel)
{
    Owned<Connection> master = Async::createConnection(ctx, _options);
    master->set(ctx, key, value, expire, channel);
}
//Set pointer types
template<class type> void RSet(ICodeContext * ctx, const char * _options, const char * key, size32_t valueLength, const type * value, unsigned expire, const char * channel)
{
    Owned<Connection> master = Async::createConnection(ctx, _options);
    master->set(ctx, key, valueLength, value, expire, channel);
}
//---INNER---
template<class type> void Connection::set(ICodeContext * ctx, const char * key, type value, unsigned expire, const char * channel)
{
    const char * _value = reinterpret_cast<const char *>(&value);//Do this even for char * to prevent compiler complaining
    handleLockForSet(ctx, key, _value, sizeof(type), channel, expire);
}
template<class type> void Connection::set(ICodeContext * ctx, const char * key, size32_t valueSize, const type * value, unsigned expire, const char * channel)
{
    const char * _value = reinterpret_cast<const char *>(value);//Do this even for char * to prevent compiler complaining
    handleLockForSet(ctx, key, _value, (size_t)valueSize, channel, expire);
}
//-----------------------------------SET------------------------------------------
ECL_REDIS_API void ECL_REDIS_CALL RSetStr(ICodeContext * ctx, const char * options, const char * key, size32_t valueLength, const char * value, unsigned expire /* = 0 (ECL default)*/)
{
    Async::RSet(ctx, options, key, valueLength*sizeof(char), value, expire, NULL);
}
ECL_REDIS_API void ECL_REDIS_CALL RSetUChar(ICodeContext * ctx, const char * options, const char * key, size32_t valueLength, const UChar * value, unsigned expire /* = 0 (ECL default)*/)
{
    Async::RSet(ctx, options, key, (valueLength)*sizeof(UChar), value, expire, NULL);
}
ECL_REDIS_API void ECL_REDIS_CALL RSetInt(ICodeContext * ctx, const char * options, const char * key, signed __int64 value, unsigned expire /* = 0 (ECL default)*/)
{
    Async::RSet(ctx, options, key, value, expire, NULL);
}
ECL_REDIS_API void ECL_REDIS_CALL RSetUInt(ICodeContext * ctx, const char * options, const char * key, unsigned __int64 value, unsigned expire /* = 0 (ECL default)*/)
{
    Async::RSet(ctx, options, key, value, expire, NULL);
}
ECL_REDIS_API void ECL_REDIS_CALL RSetReal(ICodeContext * ctx, const char * options, const char * key, double value, unsigned expire /* = 0 (ECL default)*/)
{
    Async::RSet(ctx, options, key, value, expire, NULL);
}
ECL_REDIS_API void ECL_REDIS_CALL RSetBool(ICodeContext * ctx, const char * options, const char * key, bool value, unsigned expire)
{
    Async::RSet(ctx, options, key, value, expire, NULL);
}
ECL_REDIS_API void ECL_REDIS_CALL RSetData(ICodeContext * ctx, const char * options, const char * key, size32_t valueLength, const void * value, unsigned expire)
{
    Async::RSet(ctx, options, key, valueLength, value, expire, NULL);
}
ECL_REDIS_API void ECL_REDIS_CALL RSetUtf8(ICodeContext * ctx, const char * options, const char * key, size32_t valueLength, const char * value, unsigned expire /* = 0 (ECL default)*/)
{
    Async::RSet(ctx, options, key, rtlUtf8Size(valueLength, value), value, expire, NULL);
}
//-------------------------------------GET----------------------------------------
ECL_REDIS_API bool ECL_REDIS_CALL RGetBool(ICodeContext * ctx, const char * options, const char * key)
{
    bool value;
    Async::RGet(ctx, options, key, value);
    return value;
}
ECL_REDIS_API double ECL_REDIS_CALL RGetDouble(ICodeContext * ctx, const char * options, const char * key)
{
    double value;
    Async::RGet(ctx, options, key, value);
    return value;
}
ECL_REDIS_API signed __int64 ECL_REDIS_CALL RGetInt8(ICodeContext * ctx, const char * options, const char * key)
{
    signed __int64 value;
    Async::RGet(ctx, options, key, value);
    return value;
}
ECL_REDIS_API unsigned __int64 ECL_REDIS_CALL RGetUint8(ICodeContext * ctx, const char * options, const char * key)
{
    unsigned __int64 value;
    Async::RGet(ctx, options, key, value);
    return value;
}
ECL_REDIS_API void ECL_REDIS_CALL RGetStr(ICodeContext * ctx, size32_t & returnLength, char * & returnValue, const char * options, const char * key)
{
    size_t _returnLength;
    Async::RGet(ctx, options, key, _returnLength, returnValue);
    returnLength = static_cast<size32_t>(_returnLength);
}
ECL_REDIS_API void ECL_REDIS_CALL RGetUChar(ICodeContext * ctx, size32_t & returnLength, UChar * & returnValue,  const char * options, const char * key)
{
    size_t returnSize = 0;
    Async::RGet(ctx, options, key, returnSize, returnValue);
    returnLength = static_cast<size32_t>(returnSize/sizeof(UChar));
}
ECL_REDIS_API void ECL_REDIS_CALL RGetUtf8(ICodeContext * ctx, size32_t & returnLength, char * & returnValue, const char * options, const char * key)
{
    size_t returnSize = 0;
    Async::RGet(ctx, options, key, returnSize, returnValue);
    returnLength = static_cast<size32_t>(rtlUtf8Length(returnSize, returnValue));
}
ECL_REDIS_API void ECL_REDIS_CALL RGetData(ICodeContext * ctx, size32_t & returnLength, void * & returnValue, const char * options, const char * key)
{
    size_t _returnLength = 0;
    Async::RGet(ctx, options, key, _returnLength, returnValue);
    returnLength = static_cast<size32_t>(_returnLength);
}
}//close Async namespace






namespace Lock {
ECL_REDIS_API unsigned __int64 ECL_REDIS_CALL RGetLockObject(ICodeContext * ctx, const char * options, const char * key)
{
    Owned<KeyLock> keyPtr;
    StringBuffer lockId;
    lockId.set(Lock::REDIS_LOCK_PREFIX);
    lockId.append("_").append(key).append("_");
    CriticalBlock block(crit);
    //srand(unsigned int seed);
    lockId.append(rand()%REDIS_MAX_LOCKS+1);

    keyPtr.set(new Lock::KeyLock(options, key, lockId.str()));
    return reinterpret_cast<unsigned long long>(keyPtr.get());
}
ECL_REDIS_API bool ECL_REDIS_CALL RMissThenLock(ICodeContext * ctx, unsigned __int64 _keyPtr)
{
    KeyLock * keyPtr = (KeyLock*)_keyPtr;
    const char * channel = keyPtr->getChannel();
    if (!keyPtr || strlen(channel) == 0)
    {
        VStringBuffer msg("Redis Plugin : ERROR 'Locking.ExistLockSub' called without sufficient LockObject.");
        rtlFail(0, msg.str());
    }
    const char * options = keyPtr->getOptions();
    Owned<Async::Connection> master = Async::createConnection(ctx, options);
    return master->missThenLock(ctx, keyPtr);
}
//-----------------------------------SET------------------------------------------
ECL_REDIS_API void ECL_REDIS_CALL RSetStr(ICodeContext * ctx, size32_t & returnLength, char * & returnValue, unsigned __int64 _keyPtr, size32_t valueLength, const char * value, unsigned expire /* = 0 (ECL default)*/)
{
    KeyLock * keyPtr = (KeyLock*)_keyPtr;
    Async::RSet(ctx, keyPtr->getOptions(), keyPtr->getKey(), valueLength, value, expire, keyPtr->getChannel());
    returnLength = valueLength;
    memcpy(&returnValue, value, returnLength);
}
ECL_REDIS_API void ECL_REDIS_CALL RSetUChar(ICodeContext * ctx, size32_t & returnLength, UChar * & returnValue, unsigned __int64 _keyPtr, size32_t valueLength, const UChar * value, unsigned expire /* = 0 (ECL default)*/)
{
    KeyLock * keyPtr = (KeyLock*)_keyPtr;
    Async::RSet(ctx, keyPtr->getOptions(), keyPtr->getKey(), (valueLength)*sizeof(UChar), value, expire, keyPtr->getChannel());
    returnLength = valueLength;
    memcpy(&returnValue, value, returnLength);
}
ECL_REDIS_API signed __int64 ECL_REDIS_CALL RSetInt(ICodeContext * ctx, unsigned __int64 _keyPtr, signed __int64 value, unsigned expire /* = 0 (ECL default)*/)
{
    KeyLock * keyPtr = (KeyLock*)_keyPtr;
    Async::RSet(ctx, keyPtr->getOptions(), keyPtr->getKey(), value, expire, keyPtr->getChannel());
    return value;
}
ECL_REDIS_API  unsigned __int64 ECL_REDIS_CALL RSetUInt(ICodeContext * ctx, unsigned __int64 _keyPtr, unsigned __int64 value, unsigned expire /* = 0 (ECL default)*/)
{
    KeyLock * keyPtr = (KeyLock*)_keyPtr;
    Async::RSet(ctx, keyPtr->getOptions(), keyPtr->getKey(), value, expire, keyPtr->getChannel());
    return value;
}
ECL_REDIS_API double ECL_REDIS_CALL RSetReal(ICodeContext * ctx, unsigned __int64 _keyPtr, double value, unsigned expire /* = 0 (ECL default)*/)
{
    KeyLock * keyPtr = (KeyLock*)_keyPtr;
    Async::RSet(ctx, keyPtr->getOptions(), keyPtr->getKey(), value, expire, keyPtr->getChannel());
    return value;
}
ECL_REDIS_API bool ECL_REDIS_CALL RSetBool(ICodeContext * ctx, unsigned __int64 _keyPtr,  bool value, unsigned expire /* = 0 (ECL default)*/)
{
    KeyLock * keyPtr = (KeyLock*)_keyPtr;
    Async::RSet(ctx, keyPtr->getOptions(), keyPtr->getKey(), value, expire, keyPtr->getChannel());
    return value;
}
ECL_REDIS_API void ECL_REDIS_CALL RSetData(ICodeContext * ctx, size32_t & returnLength, void * & returnValue, unsigned __int64 _keyPtr, size32_t valueLength, const void * value, unsigned expire /* = 0 (ECL default)*/)
{
    KeyLock * keyPtr = (KeyLock*)_keyPtr;
    Async::RSet(ctx, keyPtr->getOptions(), keyPtr->getKey(), valueLength, value, expire, keyPtr->getChannel());
    returnLength = valueLength;
    memcpy(&returnValue, value, returnLength);
}
ECL_REDIS_API void ECL_REDIS_CALL RSetUtf8(ICodeContext * ctx, size32_t & returnLength, char * & returnValue, unsigned __int64 _keyPtr, size32_t valueLength, const char * value, unsigned expire /* = 0 (ECL default)*/)
{
    KeyLock * keyPtr = (KeyLock*)_keyPtr;
    Async::RSet(ctx, keyPtr->getOptions(), keyPtr->getKey(), rtlUtf8Size(valueLength, value), value, expire, keyPtr->getChannel());
    returnLength = valueLength;
    memcpy(&returnValue, value, returnLength);
}
//-------------------------------------GET----------------------------------------
ECL_REDIS_API bool ECL_REDIS_CALL RGetBool(ICodeContext * ctx, unsigned __int64 _keyPtr)
{
    bool value;
    Lock::KeyLock * keyPtr = (Lock::KeyLock*)_keyPtr;
    Async::RGet(ctx, keyPtr->getOptions(), keyPtr->getKey(), value, keyPtr->getChannel());
    return value;
}
ECL_REDIS_API double ECL_REDIS_CALL RGetDouble(ICodeContext * ctx, unsigned __int64 _keyPtr)
{
    double value;
    Lock::KeyLock * keyPtr = (Lock::KeyLock*)_keyPtr;
    Async::RGet(ctx, keyPtr->getOptions(), keyPtr->getKey(), value, keyPtr->getChannel());
    return value;
}
ECL_REDIS_API signed __int64 ECL_REDIS_CALL RGetInt8(ICodeContext * ctx, unsigned __int64 _keyPtr)
{
    signed __int64 value;
    Lock::KeyLock * keyPtr = (Lock::KeyLock*)_keyPtr;
    Async::RGet(ctx, keyPtr->getOptions(), keyPtr->getKey(), value, keyPtr->getChannel());
    return value;
}
ECL_REDIS_API unsigned __int64 ECL_REDIS_CALL RGetUint8(ICodeContext * ctx, unsigned __int64 _keyPtr)
{
    unsigned __int64 value;
    Lock::KeyLock * keyPtr = (Lock::KeyLock*)_keyPtr;
    Async::RGet(ctx, keyPtr->getOptions(), keyPtr->getKey(), value, keyPtr->getChannel());
    return value;
}
ECL_REDIS_API void ECL_REDIS_CALL RGetStr(ICodeContext * ctx, size32_t & returnLength, char * & returnValue, unsigned __int64 _keyPtr)
{
    size_t _returnLength;
    Lock::KeyLock * keyPtr = (Lock::KeyLock*)_keyPtr;
    Async::RGet(ctx, keyPtr->getOptions(), keyPtr->getKey(), _returnLength, returnValue, keyPtr->getChannel());
    returnLength = static_cast<size32_t>(_returnLength);
}
ECL_REDIS_API void ECL_REDIS_CALL RGetUChar(ICodeContext * ctx, size32_t & returnLength, UChar * & returnValue,  unsigned __int64 _keyPtr)
{
    size_t returnSize = 0;
    Lock::KeyLock * keyPtr = (Lock::KeyLock*)_keyPtr;
    Async::RGet(ctx, keyPtr->getOptions(), keyPtr->getKey(), returnSize, returnValue, keyPtr->getChannel());
    returnLength = static_cast<size32_t>(returnSize/sizeof(UChar));
}
ECL_REDIS_API void ECL_REDIS_CALL RGetUtf8(ICodeContext * ctx, size32_t & returnLength, char * & returnValue, unsigned __int64 _keyPtr)
{
    size_t returnSize = 0;
    Lock::KeyLock * keyPtr = (Lock::KeyLock*)_keyPtr;
    Async::RGet(ctx, keyPtr->getOptions(), keyPtr->getKey(), returnSize, returnValue, keyPtr->getChannel());
    returnLength = static_cast<size32_t>(rtlUtf8Length(returnSize, returnValue));
}
ECL_REDIS_API void ECL_REDIS_CALL RGetData(ICodeContext * ctx, size32_t & returnLength, void * & returnValue, unsigned __int64 _keyPtr)
{
    size_t _returnLength = 0;
    Lock::KeyLock * keyPtr = (Lock::KeyLock*)_keyPtr;
    Async::RGet(ctx, keyPtr->getOptions(), keyPtr->getKey(), _returnLength, returnValue, keyPtr->getChannel());
    returnLength = static_cast<size32_t>(_returnLength);
}

}//close Lock namespace

