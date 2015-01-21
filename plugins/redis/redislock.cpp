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
static const unsigned WAIT_TIMEOUT = 10000; //(ms)

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
    redisContext * connection = redisConnectWithTimeout(master, port, RedisPlugin::REDIS_TIMEOUT);
    StringAttr lockIdFound;
    CriticalBlock block(crit);
    OwnedReply reply = RedisPlugin::createReply(redisCommand(connection, getCmd, key.str(), strlen(key.str())));
    lockIdFound.setown(reply->query()->str);

    if (strcmp(channel, lockIdFound) == 0)
        OwnedReply reply = RedisPlugin::createReply(redisCommand(connection, "DEL %b", key.str(), strlen(key.str())));

    if (connection)
        redisFree(connection);
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
class Connection : public RedisPlugin::Connection
{
public :
    Connection(ICodeContext * ctx, const char * _options);
    ~Connection()
    {
        if (connection)
            redisAsyncDisconnect(connection);
    }
    //get
    template <class type> void get(ICodeContext * ctx, const char * key, type & value, Lock::KeyLock * keyPtr, const char * newValue);
    template <class type> void get(ICodeContext * ctx, const char * key, size_t & valueLength, type * & value, Lock::KeyLock * keyPtr, const char * newValue);
    void getVoidPtrLenPair(ICodeContext * ctx, const char * key, size_t & valueLength, void * & value, Lock::KeyLock * keyPtr, const char * newValue);

   private :
       virtual void assertOnError(const redisReply * reply, const char * _msg);
       virtual void assertConnection();
       void assertRedisErr(int reply, const char * _msg);
       redisReply * handleLock(const char * key);
       bool lock(const char * key, Lock::KeyLock * keyPtr);
       void subscribe(const char * channel, StringAttr & value);

protected :
    redisAsyncContext * connection;
};

class SyncConnection : public Sync::Connection
{
public :
    SyncConnection(ICodeContext * ctx, const char * _options);

    void pub(ICodeContext * ctx, Lock::KeyLock * keyPtr, const char * msg);
    bool missAndLock(ICodeContext * ctx, const Lock::KeyLock * key);
};


typedef Owned<SyncConnection> OwnedSyncConnection;
static OwnedSyncConnection cachedSyncConnection;

SyncConnection::SyncConnection(ICodeContext * ctx, const char * _options) : Sync::Connection(ctx, _options)
{
   connection = NULL;
   connection = redisConnectWithTimeout(master.str(), port, RedisPlugin::REDIS_TIMEOUT);
   assertConnection();
}

Connection::Connection(ICodeContext * ctx, const char * _options) : RedisPlugin::Connection(ctx, _options)
{
   connection = NULL;
   connection = redisAsyncConnect(master.str(), port);
   assertConnection();
}
void Connection::assertConnection()
{
    if (!connection)
        rtlFail(0, "Redis Plugin: async context mem alloc fail.");
    //connection->data = (void*)&conSem;
    //assertBufferWriteError(redisAsyncSetConnectCallback(connection, connectCallback), "set connectCallback");
    //conSem.wait(10000);
    //assertBufferWriteError(redisAsyncSetConnectCallback(connection, connectCallback), "set connectCallback");

}
SyncConnection * createSyncConnection(ICodeContext * ctx, const char * options)//could be collapsed with interface to Connection
{
    CriticalBlock block(crit);
    if (!cachedSyncConnection)
    {
        cachedSyncConnection.setown(new SyncConnection(ctx, options));
        return LINK(cachedSyncConnection);
    }

    if (cachedSyncConnection->isSameConnection(options))
        return LINK(cachedSyncConnection);

    cachedSyncConnection.setown(new SyncConnection(ctx, options));
    return LINK(cachedSyncConnection);
}
Connection * createConnection(ICodeContext * ctx, const char * options)
{
    CriticalBlock block(crit);
    return new Connection(ctx, options);
}

//-----------------------------------------------------------------------------
void Connection::assertRedisErr(int reply, const char * _msg)
{
    if (reply == REDIS_ERR)
    {
        VStringBuffer msg("Redis Plugin: %s", _msg);
        rtlFail(0, msg.str());
    }
}
bool SyncConnection::missAndLock(ICodeContext * ctx, const Lock::KeyLock * keyPtr)
{
    //NOTE: at present other calls to SET will ignore lock and overwrite value.
    StringBuffer cmd("SET %b %b NX EX ");
    cmd.append(Lock::REDIS_LOCK_EXPIRE);

    OwnedReply reply = RedisPlugin::createReply(redisCommand(connection, cmd.str(), keyPtr->getKey(), strlen(keyPtr->getKey()), keyPtr->getChannel(), strlen(keyPtr->getChannel())));
    //assertOnError(reply->query(), msg);
    const redisReply * actReply = reply->query();

    if (actReply && (actReply->type == REDIS_REPLY_STATUS && strcmp(actReply->str, "OK")==0))
        return true;
    else
        return false;
}

ECL_REDIS_API bool ECL_REDIS_CALL RMissAndLock(ICodeContext * ctx, unsigned __int64 _keyPtr)
{
    const Lock::KeyLock * keyPtr = reinterpret_cast<const Lock::KeyLock*>(_keyPtr);
    if (!keyPtr)
    {
        rtlFail(0, "Redis Plugin: no key specified to 'MissAndLock'");
    }
    OwnedSyncConnection master = createSyncConnection(ctx, keyPtr->getOptions());
    return master->missAndLock(ctx, keyPtr);
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
void assertCallbackError(const redisReply * reply, const char * _msg)
{
    if (reply->type == REDIS_REPLY_ERROR)
    {
        VStringBuffer msg("Redis Plugin: %s%s", _msg, reply->str);
        rtlFail(0, msg.str());
    }
}
void subCB(redisAsyncContext * connection, void * _reply, void * privdata)
{
    if (_reply == NULL)
        return;

    redisReply * reply = (redisReply*)_reply;
    assertCallbackError(reply, "callback fail");

    if (reply->type == REDIS_REPLY_ARRAY && strcmp("message", reply->element[0]->str) == 0 )// && strcmp(reply->element[1]->str, channel) == 0 )
    {
        ((StringAttr*)privdata)->set(reply->element[2]->str);
        redisLibevDelRead((void*)connection->ev.data);
    }

}
void unsubCB(redisAsyncContext * connection, void * _reply, void * privdata)
{
    redisReply * reply = (redisReply*)_reply;
    assertCallbackError(reply, "get callback fail");

    redisLibevDelRead((void*)connection->ev.data);
}
void getCB(redisAsyncContext * connection, void * _reply, void * privdata)
{
    redisReply * reply = (redisReply*)_reply;
    assertCallbackError(reply, "get callback fail");

    ((StringAttr*)privdata)->set(reply->str);
    redisLibevDelRead((void*)connection->ev.data);
}
void pubCB(redisAsyncContext * connection, void * _reply, void * privdata)
{
    redisReply * reply = (redisReply*)_reply;
    assertCallbackError(reply, "get callback fail");

    redisLibevDelRead((void*)connection->ev.data);
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
void Connection::subscribe(const char * channel, StringAttr & value)
{
    assertRedisErr(redisAsyncCommand(connection, subCB, (void*)&value, "SUBSCRIBE %b", channel, strlen(channel)), "buffer write error");
    ev_loop(EV_DEFAULT_ 0);
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
    StringAttr _returnValue;
    assertRedisErr(redisLibevAttach(EV_DEFAULT_ connection), "failure to attach to libev");
    assertRedisErr(redisAsyncCommand(connection, getCB, (void*)&_returnValue, "GET %b", key, strlen(key)), "buffer write error");
    ev_loop(EV_DEFAULT_ 0);

    if  (strcmp(newValue, "\0") != 0 )
    {
        if (strncmp(_returnValue.str(), Lock::REDIS_LOCK_PREFIX, strlen(Lock::REDIS_LOCK_PREFIX)) == 0 )//double check key is locked
        {
            StringAttr channel(_returnValue);
            assertRedisErr(redisAsyncCommand(connection, NULL, NULL, "SET %b %b", key, strlen(key), newValue, strlen(newValue)), "buffer write error");
            assertRedisErr(redisAsyncCommand(connection, pubCB, NULL, "PUBLISH %b %b", channel.str(), channel.length(), newValue, strlen(newValue)), "buffer write error");
            ev_loop(EV_DEFAULT_ 0);
        }
        returnLength = strlen(newValue);
        size_t returnSize = returnLength*sizeof(type);
        returnValue = reinterpret_cast<type*>(cpy(newValue, returnSize));
        return;
    }

    if (_returnValue.isEmpty())
    {
        if (!keyPtr)
        {
            returnLength = 0;
            size_t returnSize = returnLength*sizeof(type);
            returnValue = reinterpret_cast<type*>(cpy("", returnSize));
            return;
        }
        else
        {
            if (lock(key, keyPtr))
            {
            	printf("locked\n");
                returnLength = 0;
                size_t returnSize = returnLength*sizeof(type);
                returnValue = reinterpret_cast<type*>(cpy("", returnSize));
                return;
            }
            else
            {
            	printf("sub A\n");
                const char * channel = keyPtr->getChannel();
                subscribe(channel, _returnValue);
                assertRedisErr(redisAsyncCommand(connection, unsubCB, NULL, "UNSUBSCRIBE %b", channel, strlen(channel)), "buffer write error");
                ev_loop(EV_DEFAULT_ 0);
            }
        }
    }
    else
    {
        if (strncmp(_returnValue.str(), Lock::REDIS_LOCK_PREFIX, strlen(Lock::REDIS_LOCK_PREFIX)) == 0 )
        {
        	printf("sub B\n");

            StringAttr channel(_returnValue);
            subscribe(channel.str(), _returnValue);
            assertRedisErr(redisAsyncCommand(connection, unsubCB, NULL, "UNSUBSCRIBE %b", channel.str(), channel.length()), "buffer write error");
            ev_loop(EV_DEFAULT_ 0);
        }
        else
        {
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
    OwnedReply reply = RedisPlugin::createReply(redisCommand(connection, getCmd, key, strlen(key)));
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

