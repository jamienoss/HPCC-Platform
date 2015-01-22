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
#include "redisplugin.hpp"
#include "redissync.hpp"
#include "redislock.hpp"

#include "hiredis/async.h"



namespace Lock
{
static CriticalSection crit;
static const char * REDIS_LOCK_PREFIX = "redis_ecl_lock";// needs to be a large random value uniquely individual per client
static const unsigned REDIS_LOCK_EXPIRE = 60; //(secs)
static const unsigned REDIS_MAX_LOCKS = 9999;

class SubHolder : public CInterface
{
public :
    SubHolder(const char * _channel) { channel.set(_channel); }
    SubHolder(const char * _channel, redisCallbackFn * _callback) { channel.set(_channel); callback = _callback; }

    inline void setCB(redisCallbackFn * _callback) { callback = _callback; }
    inline redisCallbackFn * getCB() { return callback; }
    inline void setChannel(const char * _channel) { channel.set(_channel); }
    inline const char * getChannel() { return channel.str(); }
    inline void setMessage(const char * _message) { message.set(_message); }
    inline const char * getMessage() { return message.str(); }
    inline void wait(unsigned timeout) { sem.wait(timeout); }
    inline void signal() { sem.signal(); }

private :
    Semaphore sem;
    redisCallbackFn * callback;
    StringAttr message;
    StringAttr channel;
};

class KeyLock : public CInterface
{
public :
    KeyLock(const char * _options, const char * _key, const char * _lock);
    ~KeyLock();

    inline const char * getKey()     const { return key.str(); }
    inline const char * getOptions() const { return options.str(); }
    inline const char * getChannel()  const { return channel.str(); }

private :
    StringAttr options; //shouldn't be need, tidy isSameConnection to pass 'master' & 'port'
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

}//close Lock Namespace

namespace Async
{
static CriticalSection crit;
static const unsigned REDIS_TIMEOUT = 1000;//ms
class Connection : public RedisPlugin::Connection
{
public :
    Connection(ICodeContext * ctx, const char * _options);
    ~Connection();

    //get
    template <class type> void get(ICodeContext * ctx, const char * key, type & value, Lock::KeyLock * keyPtr, const char * newValue);
    template <class type> void get(ICodeContext * ctx, const char * key, size_t & valueLength, type * & value, Lock::KeyLock * keyPtr, const char * newValue);
    void getVoidPtrLenPair(ICodeContext * ctx, const char * key, size_t & valueLength, void * & value, Lock::KeyLock * keyPtr, const char * newValue);

private :
    void createAndAssertConnection();
    virtual void assertOnError(const redisReply * reply, const char * _msg);
    virtual void assertConnection();
    void assertRedisErr(int reply, const char * _msg);
    redisReply * handleLock(const char * key);
    bool lock(const char * key, Lock::KeyLock * keyPtr);
    void subscribe(const char * channel, StringAttr & value);
    void subscribeNonBlock(Lock::SubHolder * holder);
    void unsubscribe(const char * channel);

    //callbacks
    static void assertCallbackError(const redisReply * reply, const char * _msg);
    static void connectCB(const redisAsyncContext *c, int status);
    static void disconnectCB(const redisAsyncContext *c, int status);
    static void subSemCB(redisAsyncContext * context, void * _reply, void * privdata);
    static void subCB(redisAsyncContext * context, void * _reply, void * privdata);
    static void pubCB(redisAsyncContext * context, void * _reply, void * privdata);
    static void setCB(redisAsyncContext * context, void * _reply, void * privdata);
    static void getCB(redisAsyncContext * context, void * _reply, void * privdata);
    static void unsubCB(redisAsyncContext * context, void * _reply, void * privdata);

protected :
    redisAsyncContext * context;
};

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
    if (status != REDIS_OK && status != 2)
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
void Connection::subSemCB(redisAsyncContext * context, void * _reply, void * privdata)
{
    if (_reply == NULL)
        return;

    redisReply * reply = (redisReply*)_reply;
    assertCallbackError(reply, "callback fail");

    if (reply->type == REDIS_REPLY_ARRAY && strcmp("message", reply->element[0]->str) == 0 )
    {
        Lock::SubHolder * holder = (Lock::SubHolder*)privdata;
        holder->setMessage(reply->element[2]->str);
        const char * channel = reply->element[1]->str;
        redisAsyncCommand(context, NULL, NULL, "UNSUBSCRIBE %b", channel, strlen(channel));
        holder->signal();
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
    if (!_reply)
        return;

    redisReply * reply = (redisReply*)_reply;
    assertCallbackError(reply, "get callback fail");

    ((StringAttr*)privdata)->set(reply->str);
    redisLibevDelRead((void*)context->ev.data);
}
void Connection::pubCB(redisAsyncContext * context, void * _reply, void * privdata)
{
    redisReply * reply = (redisReply*)_reply;
    assertCallbackError(reply, "get callback fail");

    redisLibevDelRead((void*)context->ev.data);
}
//----------------------------------GET----------------------------------------
template<class type> void Connection::get(ICodeContext * ctx, const char * key, type & returnValue, Lock::KeyLock * keyPtr, const char * newValue)
{
	//assert keyPtr;
    //Do we double check 1st that the key is locked? NAH!
    //assertBufferWriteError(redisAsyncCommand(connection, callback, (void*)keyPtr, "SUBSCRIBE %b", key, strlen(key)), "subscribe error");
    //what if notify happens before wait?
    //keyPtr->wait();

    /*
    StringBuffer keyMsg = getFailMsg;
    assertOnError(reply->query(), appendIfKeyNotFoundMsg(reply->query(), key, keyMsg));

    size_t returnSize = reply->query()->len;//*sizeof(type);
    if (sizeof(type)!=returnSize)
    {
        VStringBuffer msg("RedisPlugin: ERROR - Requested type of different size (%uB) from that stored (%uB).", (unsigned)sizeof(type), (unsigned)returnSize);

        rtlFail(0, msg.str());
    }
    memcpy(&returnValue, reply->query()->str, returnSize);
    */
}
void Connection::unsubscribe(const char * channel)
{
    //assertRedisErr(redisAsyncCommand(context, NULL, NULL, "UNSUBSCRIBE %b", channel, strlen(channel)), "UNSUBSCRIBE buffer write error");
}
void Connection::subscribe(const char * channel, StringAttr & value)
{
    assertRedisErr(redisAsyncCommand(context, subCB, (void*)&value, "SUBSCRIBE %b", channel, strlen(channel)), "SUBSCRIBE buffer write error");
    ev_loop(EV_DEFAULT_ 0);
}
void Connection::subscribeNonBlock(Lock::SubHolder * holder)
{
    //'NonBlock' refers to this function, not the redis connection.
    const char  * channel = holder->getChannel();
    assertRedisErr(redisAsyncCommand(context, holder->getCB(), (void*)holder, "SUBSCRIBE %b", channel, strlen(channel)), "SUBSCRIBE buffer write error");
}
bool Connection::lock(const char * key, Lock::KeyLock * keyPtr)
{
    StringBuffer cmd("SET %b %b NX EX ");
    cmd.append(Lock::REDIS_LOCK_EXPIRE);

    const char * channel = keyPtr->getChannel();
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

template<class type> void Connection::get(ICodeContext * ctx, const char * key, size_t & returnLength, type * & returnValue, Lock::KeyLock * keyPtr, const char * newValue)
{
    const char * channel = keyPtr->getChannel();
    Lock::SubHolder holder(channel, subSemCB);
    subscribeNonBlock(&holder);

    StringAttr _returnValue;
    assertRedisErr(redisLibevAttach(EV_DEFAULT_ context), "failure to attach to libev");
    assertRedisErr(redisAsyncCommand(context, getCB, (void*)&_returnValue, "GET %b", key, strlen(key)), "GET buffer write error");
    ev_loop(EV_DEFAULT_ 0);

    //This code block is for when setting the retrieved value on an initial cache miss
    if  (strlen(newValue) > 0)
    {
        if (strncmp(_returnValue.str(), Lock::REDIS_LOCK_PREFIX, strlen(Lock::REDIS_LOCK_PREFIX)) == 0 )//double check key is locked
        {
            StringAttr channel(_returnValue);
            assertRedisErr(redisAsyncCommand(context, NULL, NULL, "SET %b %b", key, strlen(key), newValue, strlen(newValue)), "SET buffer write error");
            assertRedisErr(redisAsyncCommand(context, pubCB, NULL, "PUBLISH %b %b", channel.str(), channel.length(), newValue, strlen(newValue)), "PUBLISH buffer write error");
            ev_loop(EV_DEFAULT_ 0);
        }
        unsubscribe(channel);
        returnLength = strlen(newValue);
        size_t returnSize = returnLength*sizeof(type);
        returnValue = reinterpret_cast<type*>(cpy(newValue, returnSize));
        return;
    }

    //Main logic
    //The presence of keyPtr determines whether locks are ignored
    if (_returnValue.isEmpty())
    {
        if (!keyPtr)
        {
            //Do not lock & subscribe, i.e. a normal call to GET
            unsubscribe(channel);
            returnLength = 0;
            size_t returnSize = returnLength*sizeof(type);
            returnValue = reinterpret_cast<type*>(cpy("", returnSize));
            return;
        }
        else
        {
            if (lock(key, keyPtr))
            {
                unsubscribe(channel);
                returnLength = 0;
                size_t returnSize = returnLength*sizeof(type);
                returnValue = reinterpret_cast<type*>(cpy("", returnSize));
                return;
            }
            else
            {
                holder.wait(Lock::REDIS_LOCK_EXPIRE);
            }
        }
    }
    else
    {
        if (strncmp(_returnValue.str(), Lock::REDIS_LOCK_PREFIX, strlen(Lock::REDIS_LOCK_PREFIX)) == 0 )
        {
        	printf("sub B\n");
            holder.wait(Lock::REDIS_LOCK_EXPIRE);
        }
        else
        {
            unsubscribe(channel);
            returnLength = _returnValue.length();
            size_t returnSize = returnLength*sizeof(type);
            returnValue = reinterpret_cast<type*>(cpy(_returnValue.str(), returnSize));
            return;
        }
    }

    //Fall through from a subscription
    returnLength = _returnValue.length();
    size_t returnSize = returnLength*sizeof(type);
    returnValue = reinterpret_cast<type*>(cpy(_returnValue.str(), returnSize));
}
void Connection::getVoidPtrLenPair(ICodeContext * ctx, const char * key, size_t & returnLength, void * & returnValue, Lock::KeyLock * keyPtr, const char * newValue)
{
    /*
	const char * key = keyPtr->getKey();
    OwnedReply reply = RedisPlugin::createReply(redisCommand(context, getCmd, key, strlen(key)));
    StringBuffer keyMsg = getFailMsg;
    assertOnError(reply->query(), appendIfKeyNotFoundMsg(reply->query(), key, keyMsg));

    returnLength = reply->query()->len;
    returnValue = reinterpret_cast<void*>(cpy(reply->query()->str, reply->query()->len*sizeof(char)));
    */
}
//-------------------------------------------GET-----------------------------------------
template<class type> void RGet(ICodeContext * ctx, const char * options, const char * key, type & returnValue, Lock::KeyLock * keyPtr, const char * newValue)
{
    Owned<Async::Connection> master = Async::createConnection(ctx, options);
    master->get(ctx, key, returnValue, keyPtr, newValue);
}
template<class type> void RGet(ICodeContext * ctx, const char * options, const char * key, size_t & returnLength, type * & returnValue, Lock::KeyLock * keyPtr, const char * newValue)
{
    Owned<Async::Connection> master = Async::createConnection(ctx, options);
    master->get(ctx, key, returnLength, returnValue, keyPtr, newValue);
}
void RGetVoidPtrLenPair(ICodeContext * ctx, const char * options, const char * key, size_t & returnLength, void * & returnValue, Lock::KeyLock * keyPtr, const char * newValue)
{
    Owned<Async::Connection> master = Async::createConnection(ctx, options);
    master->getVoidPtrLenPair(ctx, key, returnLength, returnValue, keyPtr, newValue);
}
}//close Async namespace

namespace Lock {
//-------------------------------------GET----------------------------------------
ECL_REDIS_API bool ECL_REDIS_CALL RGetBool(ICodeContext * ctx, unsigned __int64 _keyPtr, const char * newValue)
{
    bool value;
    Lock::KeyLock * keyPtr = (Lock::KeyLock*)_keyPtr;
    Async::RGet(ctx, keyPtr->getOptions(), keyPtr->getKey(), value, keyPtr, newValue);
    return value;
}
ECL_REDIS_API double ECL_REDIS_CALL RGetDouble(ICodeContext * ctx, unsigned __int64 _keyPtr, const char * newValue)
{
    double value;
    Lock::KeyLock * keyPtr = (Lock::KeyLock*)_keyPtr;
    Async::RGet(ctx, keyPtr->getOptions(), keyPtr->getKey(), value, keyPtr, newValue);
    return value;
}
ECL_REDIS_API signed __int64 ECL_REDIS_CALL RGetInt8(ICodeContext * ctx, unsigned __int64 _keyPtr, const char * newValue)
{
    signed __int64 value;
    Lock::KeyLock * keyPtr = (Lock::KeyLock*)_keyPtr;
    Async::RGet(ctx, keyPtr->getOptions(), keyPtr->getKey(), value, keyPtr, newValue);
    return value;
}
ECL_REDIS_API unsigned __int64 ECL_REDIS_CALL RGetUint8(ICodeContext * ctx, unsigned __int64 _keyPtr, const char * newValue)
{
    unsigned __int64 value;
    Lock::KeyLock * keyPtr = (Lock::KeyLock*)_keyPtr;
    Async::RGet(ctx, keyPtr->getOptions(), keyPtr->getKey(), value, keyPtr, newValue);
    return value;
}
ECL_REDIS_API void ECL_REDIS_CALL RGetStr(ICodeContext * ctx, size32_t & returnLength, char * & returnValue, unsigned __int64 _keyPtr, const char * newValue)
{
    size_t _returnLength;
    Lock::KeyLock * keyPtr = (Lock::KeyLock*)_keyPtr;
    Async::RGet(ctx, keyPtr->getOptions(), keyPtr->getKey(), _returnLength, returnValue, keyPtr, newValue);
    returnLength = static_cast<size32_t>(_returnLength);
}
ECL_REDIS_API void ECL_REDIS_CALL RGetUChar(ICodeContext * ctx, size32_t & returnLength, UChar * & returnValue,  unsigned __int64 _keyPtr, const char * newValue)
{
    size_t _returnSize = 0;
    Lock::KeyLock * keyPtr = (Lock::KeyLock*)_keyPtr;
    Async::RGet(ctx, keyPtr->getOptions(), keyPtr->getKey(), returnValue, keyPtr, newValue);
    returnLength = static_cast<size32_t>(_returnSize/sizeof(UChar));
}
ECL_REDIS_API void ECL_REDIS_CALL RGetUtf8(ICodeContext * ctx, size32_t & returnLength, char * & returnValue, unsigned __int64 _keyPtr, const char * newValue)
{
    size_t returnSize = 0;
    Lock::KeyLock * keyPtr = (Lock::KeyLock*)_keyPtr;
    Async::RGet(ctx, keyPtr->getOptions(), keyPtr->getKey(), returnValue, keyPtr, newValue);
    returnLength = static_cast<size32_t>(rtlUtf8Length(returnSize, returnValue));
}
ECL_REDIS_API void ECL_REDIS_CALL RGetData(ICodeContext * ctx, size32_t & returnLength, void * & returnValue, unsigned __int64 _keyPtr, const char * newValue)
{
    size_t _returnLength = 0;
    Lock::KeyLock * keyPtr = (Lock::KeyLock*)_keyPtr;
    Async::RGet(ctx, keyPtr->getOptions(), keyPtr->getKey(), returnValue, keyPtr, newValue);
    returnLength = static_cast<size32_t>(_returnLength);
}

}//close Lock namespace

