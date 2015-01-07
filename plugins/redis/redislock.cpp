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

#include "platform.h"
#include "eclrtl.hpp"
#include "jstring.hpp"
#include "redisplugin.hpp"
#include "redissync.hpp"
#include "redislock.hpp"
#include <hiredis/async.h>
#include <hiredis/adapters/libevent.h>

namespace Lock
{

static const char * REDIS_LOCK_PREFIX = "redis_ecl_lock";// needs to be a large random value uniquely individual per client
static const unsigned REDIS_LOCK_EXPIRE = 60; //(secs)
static const unsigned REDIS_MAX_LOCKS = 9999;
static CriticalSection crit;

class KeyLock : public CInterface
{
public :
    KeyLock(const char * _options, const char * _key, const char * _lock);
    ~KeyLock();

    inline const char * getKey()     const { return key.str(); }
    inline const char * getOptions() const { return options.str(); }
    inline const char * getLockId()  const { return lockId.str(); }

private :
    StringAttr options; //shouldn't be need, tidy isSameConnection to pass 'master' & 'port'
    StringAttr master;
    unsigned port;
    StringAttr key;
    StringAttr lockId;
};

class SyncConnection : public Sync::Connection
{
public :
    SyncConnection(ICodeContext * ctx, const char * _options);
    ~SyncConnection()
    {
        if (connection)
            redisFree(connection);
    }

    void pub(ICodeContext * ctx, const char * channel, const char * msg);
    bool missAndLock(ICodeContext * ctx, const KeyLock * key);

private :
    redisContext * connection;
};

class AsyncConnection : public RedisPlugin::Connection //there is no Async::Connection yet.
{
public :
    AsyncConnection(ICodeContext * ctx, const char * _options);
    ~AsyncConnection()
    {
        if (connection)
            redisAsyncDisconnect(connection);
    }

    void assertIOError(int reply, const char * _msg);

    //get
    template <class type> void getLocked(ICodeContext * ctx, KeyLock * keyPtr, type & value, RedisPlugin::eclDataType eclType);
    template <class type> void getLocked(ICodeContext * ctx, KeyLock * keyPtr, size_t & valueLength, type * & value, RedisPlugin::eclDataType eclType);
    void getLockedVoidPtrLenPair(ICodeContext * ctx, KeyLock * keyPtr, size_t & valueLength, void * & value, RedisPlugin::eclDataType eclType);

private :
    redisAsyncContext * connection;
};

typedef Owned<SyncConnection> OwnedSyncConnection;
static OwnedSyncConnection cachedSyncConnection;

typedef Owned<AsyncConnection> OwnedAsyncConnection;
static OwnedAsyncConnection cachedAsyncConnection;

SyncConnection::SyncConnection(ICodeContext * ctx, const char * _options) : Sync::Connection(ctx, _options)
{
   connection = redisConnect(master, port);
   assertConnection();
}
AsyncConnection::AsyncConnection(ICodeContext * ctx, const char * _options) : RedisPlugin::Connection(ctx, _options)
{
   connection = redisAsyncConnect(master, port);
   assertConnection();
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
AsyncConnection * createAsyncConnection(ICodeContext * ctx, const char * options)
{
    CriticalBlock block(crit);
    if (!cachedAsyncConnection)
    {
        cachedAsyncConnection.setown(new AsyncConnection(ctx, options));
        return LINK(cachedAsyncConnection);
    }

    if (cachedAsyncConnection->isSameConnection(options))
        return LINK(cachedAsyncConnection);

    cachedAsyncConnection.setown(new AsyncConnection(ctx, options));
    return LINK(cachedAsyncConnection);
}
KeyLock::KeyLock(const char * _options, const char * _key, const char * _lockId)
{
    options.setown(_options);
    key.set(_key);
    lockId.set(_lockId);
    RedisPlugin::parseOptions(_options, master, port);
}
KeyLock::~KeyLock()
{
    redisContext * connection = redisConnect(master, port);
    StringAttr lockIdFound;
    CriticalBlock block(crit);
    OwnedReply reply = RedisPlugin::createReply(redisCommand(connection, getCmd, key.str(), strlen(key.str())*sizeof(char)));
    lockIdFound.setown(reply->query()->str);

    if (strcmp(lockId, lockIdFound) == 0)
        OwnedReply reply = RedisPlugin::createReply(redisCommand(connection, "DEL %b", key.str(), strlen(key.str())*sizeof(char)));

    if (connection)
        redisFree(connection);
}
//-----------------------------------------------------------------------------
void AsyncConnection::assertIOError(int reply, const char * _msg)
{
    if (reply == REDIS_ERR)
    {
        VStringBuffer msg("Redis Plugin: Async IO ERROR with - %s", _msg);
        rtlFail(0, msg.str());
    }
}
bool SyncConnection::missAndLock(ICodeContext * ctx, const KeyLock * keyPtr)
{
    //NOTE: at present other calls to SET will ignore lock and overwrite value.
    StringBuffer cmd("SET %b %b NX EX ");
    cmd.append(Lock::REDIS_LOCK_EXPIRE);

    OwnedReply reply = RedisPlugin::createReply(redisCommand(connection, cmd.str(), keyPtr->getKey(), strlen(keyPtr->getKey())*sizeof(char), keyPtr->getLockId(), strlen(keyPtr->getLockId())*sizeof(char)));
    //assertOnError(reply->query(), msg);
    const redisReply * actReply = reply->query();

    if (actReply && (actReply->type == REDIS_REPLY_STATUS && strcmp(actReply->str, "OK")==0))
        return true;
    else
        return false;
}

ECL_REDIS_API bool ECL_REDIS_CALL RMissAndLock(ICodeContext * ctx, unsigned __int64 _keyPtr)
{
    const KeyLock * keyPtr = reinterpret_cast<const KeyLock*>(_keyPtr);
    if (!keyPtr)
    {
        rtlFail(0, "Redis Plugin: no key specified to 'MissAndLock'");
    }
    OwnedSyncConnection master = createSyncConnection(ctx, keyPtr->getOptions());
    return master->missAndLock(ctx, keyPtr);
}

ECL_REDIS_API unsigned __int64 ECL_REDIS_CALL RGetLockObject(ICodeContext * ctx, const char * options, const char * key)
{
    Owned<KeyLock> keyPtr;
    StringBuffer lockId;
    lockId.set(Lock::REDIS_LOCK_PREFIX);
    lockId.append("_").append(key).append("_");
    CriticalBlock block(crit);
    //srand(unsigned int seed);
    lockId.append(rand()%Lock::REDIS_MAX_LOCKS+1);

    keyPtr.set(new KeyLock(options, key, lockId.str()));
    return reinterpret_cast<unsigned long long>(keyPtr.get());
}

void callback(redisAsyncContext * connection, void * reply, void * privdata)
{
    //redisReply *r = (redisReply*)reply;

    printf("callback yes!!!!\n");
    /*if (reply == NULL) return;

    if (r->type == REDIS_REPLY_ARRAY) {
        for (int j = 0; j < r->elements; j++) {
            printf("%u) %s\n", j, r->element[j]->str);
        }
    }*/
}
//----------------------------------GET----------------------------------------
template<class type> void AsyncConnection::getLocked(ICodeContext * ctx, KeyLock * keyPtr, type & returnValue, RedisPlugin::eclDataType eclType)
{
    const char * key = keyPtr->getKey();

    //Do we double check 1st that the key is locked? NAH!
    signal(SIGPIPE, SIG_IGN);
    struct event_base *base = event_base_new();
    redisLibeventAttach(connection, base);
    assertIOError(redisAsyncCommand(connection, callback, NULL, "SUBSCRIBE %b", key, strlen(key)*sizeof(char)), "subscription error");
    event_base_dispatch(base);

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
template<class type> void AsyncConnection::getLocked(ICodeContext * ctx, KeyLock * keyPtr, size_t & returnLength, type * & returnValue, RedisPlugin::eclDataType eclType)
{
	const char * key = keyPtr->getKey();

	    //Do we double check 1st that the key is locked? NAH!
	    signal(SIGPIPE, SIG_IGN);
	    struct event_base *base = event_base_new();
	    redisLibeventAttach(connection, base);
	    assertIOError(redisAsyncCommand(connection, callback, NULL, "SUBSCRIBE %b", key, strlen(key)*sizeof(char)), "subscription error");
	    event_base_dispatch(base);
    /*const char * key = keyPtr->getKey();
    OwnedReply reply = RedisPlugin::createReply(redisCommand(connection, getCmd, key, strlen(key)*sizeof(char)));

    StringBuffer keyMsg = getFailMsg;
    assertOnError(reply->query(), appendIfKeyNotFoundMsg(reply->query(), key, keyMsg));

    returnLength = reply->query()->len;
    size_t returnSize = returnLength*sizeof(type);

    returnValue = reinterpret_cast<type*>(cpy(reply->query()->str, returnSize));
    */
}
void AsyncConnection::getLockedVoidPtrLenPair(ICodeContext * ctx, KeyLock * keyPtr, size_t & returnLength, void * & returnValue, RedisPlugin::eclDataType eclType)
{
    /*
	const char * key = keyPtr->getKey();
    OwnedReply reply = RedisPlugin::createReply(redisCommand(connection, getCmd, key, strlen(key)*sizeof(char)));
    StringBuffer keyMsg = getFailMsg;
    assertOnError(reply->query(), appendIfKeyNotFoundMsg(reply->query(), key, keyMsg));

    returnLength = reply->query()->len;
    returnValue = reinterpret_cast<void*>(cpy(reply->query()->str, reply->query()->len*sizeof(char)));
    */
}
//-------------------------------------------GET-----------------------------------------
template<class type> void RGetLocked(ICodeContext * ctx, unsigned __int64 _keyPtr, type & returnValue, RedisPlugin::eclDataType eclType)
{
    KeyLock * keyPtr = reinterpret_cast<KeyLock*>(_keyPtr);
    OwnedAsyncConnection master = createAsyncConnection(ctx, keyPtr->getOptions());
    master->getLocked(ctx, keyPtr, returnValue, eclType);
}
template<class type> void RGetLocked(ICodeContext * ctx, unsigned __int64 _keyPtr, size_t & returnLength, type * & returnValue, RedisPlugin::eclDataType eclType)
{
    KeyLock * keyPtr = reinterpret_cast<KeyLock*>(_keyPtr);
    OwnedAsyncConnection master = createAsyncConnection(ctx, keyPtr->getOptions());
    master->getLocked(ctx, keyPtr, returnLength, returnValue, eclType);
}
void RGetLockedVoidPtrLenPair(ICodeContext * ctx, unsigned __int64 _keyPtr, size_t & returnLength, void * & returnValue, RedisPlugin::eclDataType eclType)
{
    KeyLock * keyPtr = reinterpret_cast<KeyLock*>(_keyPtr);
    OwnedAsyncConnection master = createAsyncConnection(ctx, keyPtr->getOptions());
    master->getLockedVoidPtrLenPair(ctx, keyPtr, returnLength, returnValue, eclType);
}
//-------------------------------------GET----------------------------------------
ECL_REDIS_API bool ECL_REDIS_CALL RGetLockedBool(ICodeContext * ctx, unsigned __int64 keyPtr)
{
    bool value;
    RGetLocked(ctx, keyPtr, value, RedisPlugin::ECL_BOOLEAN);
    return value;
}
ECL_REDIS_API double ECL_REDIS_CALL RGetLockedDouble(ICodeContext * ctx, unsigned __int64 keyPtr)
{
    double value;
    RGetLocked(ctx, keyPtr, value, RedisPlugin::ECL_REAL);
    return value;
}
ECL_REDIS_API signed __int64 ECL_REDIS_CALL RGetLockedInt8(ICodeContext * ctx, unsigned __int64 keyPtr)
{
    signed __int64 value;
    RGetLocked(ctx, keyPtr, value, RedisPlugin::ECL_INTEGER);
    return value;
}
ECL_REDIS_API unsigned __int64 ECL_REDIS_CALL RGetLockedUint8(ICodeContext * ctx, unsigned __int64 keyPtr)
{
    unsigned __int64 value;
    RGetLocked(ctx, keyPtr, value, RedisPlugin::ECL_UNSIGNED);
    return value;
}
ECL_REDIS_API void ECL_REDIS_CALL RGetLockedStr(ICodeContext * ctx, size32_t & returnLength, char * & returnValue, unsigned __int64 keyPtr)
{
    size_t _returnLength;
    RGetLocked(ctx, keyPtr, _returnLength, returnValue, RedisPlugin::ECL_STRING);
    returnLength = static_cast<size32_t>(_returnLength);
}
ECL_REDIS_API void ECL_REDIS_CALL RGetLockedUChar(ICodeContext * ctx, size32_t & returnLength, UChar * & returnValue,  unsigned __int64 keyPtr)
{
    size_t _returnSize;
    RGetLocked(ctx, keyPtr, _returnSize, returnValue, RedisPlugin::ECL_UNICODE);
    returnLength = static_cast<size32_t>(_returnSize/sizeof(UChar));
}
ECL_REDIS_API void ECL_REDIS_CALL RGetLockedUtf8(ICodeContext * ctx, size32_t & returnLength, char * & returnValue, unsigned __int64 keyPtr)
{
    size_t returnSize;
    RGetLocked(ctx, keyPtr, returnSize, returnValue, RedisPlugin::ECL_UTF8);
    returnLength = static_cast<size32_t>(rtlUtf8Length(returnSize, returnValue));
}
ECL_REDIS_API void ECL_REDIS_CALL RGetLockedData(ICodeContext * ctx, size32_t & returnLength, void * & returnValue, unsigned __int64 keyPtr)
{
    size_t _returnLength;
    RGetLockedVoidPtrLenPair(ctx, keyPtr, _returnLength, returnValue, RedisPlugin::ECL_DATA);
    returnLength = static_cast<size32_t>(_returnLength);
}

ECL_REDIS_API void ECL_REDIS_CALL RPub(ICodeContext * ctx, const char * options, const char * channel, const char * msg)
{
    OwnedSyncConnection master = createSyncConnection(ctx, options);
    master->pub(ctx, channel, msg);
}
void SyncConnection::pub(ICodeContext * ctx, const char * channel, const char * msg)
{
    OwnedReply reply = RedisPlugin::createReply(redisCommand(connection, "PUBLISH %b %s", channel, strlen(channel)*sizeof(char), msg));

}
}//close namespace
