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
#include "jsem.hpp"
#include "jsocket.hpp"
#include "redisplugin.hpp"
#include "redissync.hpp"
#include "redislock.hpp"

//#include <signal.h>
//#include "hiredis/hiredis.h"
#include "hiredis/async.h"
//#undef loop
//#include "hiredis/adapters/ae.h"
#include "hiredis/adapters/libevent.h"

namespace Async
{
class events : public CInterface
{
public :
    events();
    events(redisAsyncContext * _c);


public :
    bool we;
    bool re;
    redisAsyncContext * c;
};
events::events() { we = false; re = false; c = NULL; }
events::events(redisAsyncContext * _c) {  we = false; re = false; c = _c; }

class Connection : public RedisPlugin::Connection
{
public :
    Connection(ICodeContext * ctx, const char * _options)
        : RedisPlugin::Connection(ctx, _options), ev() { };
    ~Connection()
    {
        if (connection)
            redisAsyncDisconnect(connection);
    }

    void initIOCallbacks();

protected :
    redisAsyncContext * connection;
    events ev;

};

void addRead(void * privdata)
{
    if (privdata == NULL)
         return;
    events  * ev = (events*)privdata;
    if (ev->re)
        return;
    printf("reading...\n");
    ev->re = true;

    ISocket * soc = ISocket::attach(ev->c->c.fd);
    if (soc->wait_read(30000) == 1);
     redisAsyncHandleRead(ev->c);
    if (soc->wait_read(30000) == 1);
          redisAsyncHandleRead(ev->c);
}
void delRead(void * privdata)
{    printf("done reading\n");

    if (privdata == NULL)
        return;
    events * ev = (events*)privdata;
    if (!ev->re)
        return;
    printf("done reading\n");
    ev->re = false;
}
void addWrite(void * privdata)
{
    if (privdata == NULL)
        return;

    events  * ev = (events*)privdata;
    if (ev->we)
        return;
    printf("writing...\n");
    ev->we = true;
    redisAsyncHandleWrite(ev->c);
}
void delWrite(void * privdata)
{
    if (privdata == NULL)
        return;
    events * ev = (events*)privdata;
    if (!ev->we)
        return;
    printf("done writing\n");
    ev->we = false;
}
void cleanup(void * privdata)
{
    if (privdata == NULL)
        return;
    printf("cleaning up\n");
    events * ev = (events*)privdata;
    ev->re = false;
    ev->we = false;
}

void Connection::initIOCallbacks()
{
    if (connection)
    {
        connection->ev.addRead = addRead;
        connection->ev.delRead = delRead;
        connection->ev.addWrite = addWrite;
        connection->ev.delWrite = delWrite;
        connection->ev.cleanup = cleanup;
        ev.c = connection;
        connection->ev.data = &ev;
    }
}

}//close Async namespace

namespace Lock
{

static const char * REDIS_LOCK_PREFIX = "redis_ecl_lock";// needs to be a large random value uniquely individual per client
static const unsigned REDIS_LOCK_EXPIRE = 60; //(secs)
static const unsigned REDIS_MAX_LOCKS = 9999;
static const unsigned WAIT_TIMEOUT = 10000; //(ms)
static CriticalSection crit;

class KeyLock : public CInterface
{
public :
    KeyLock(const char * _options, const char * _key, const char * _lock);
    ~KeyLock();

    inline const char * getKey()     const { return key.str(); }
    inline const char * getOptions() const { return options.str(); }
    inline const char * getLockId()  const { return lockId.str(); }

    inline void waitTO() { sem.wait(WAIT_TIMEOUT); }
    inline void waitFailOnTO() { if (!sem.wait(WAIT_TIMEOUT)) rtlFail(0, "Redis Plugin: callback timeout"); }
    inline void notify() { sem.signal(); sem.reinit(); }

private :
    StringAttr options; //shouldn't be need, tidy isSameConnection to pass 'master' & 'port'
    StringAttr master;
    int port;
    StringAttr key;
    StringAttr lockId;
    Semaphore sem;
};

class SyncConnection : public Sync::Connection
{
public :
    SyncConnection(ICodeContext * ctx, const char * _options);

    void pub(ICodeContext * ctx, KeyLock * keyPtr, const char * msg);
    bool missAndLock(ICodeContext * ctx, const KeyLock * key);
};

class AsyncConnection : public Async::Connection
{
public :
    AsyncConnection(ICodeContext * ctx, const char * _options);

    //get
    template <class type> void getLocked(ICodeContext * ctx, KeyLock * keyPtr, type & value, RedisPlugin::eclDataType eclType);
    template <class type> void getLocked(ICodeContext * ctx, KeyLock * keyPtr, size_t & valueLength, type * & value, RedisPlugin::eclDataType eclType);
    void getLockedVoidPtrLenPair(ICodeContext * ctx, KeyLock * keyPtr, size_t & valueLength, void * & value, RedisPlugin::eclDataType eclType);

private :
    virtual void assertOnError(const redisReply * reply, const char * _msg);
    virtual void assertConnection();
    void assertBufferWriteError(int reply, const char * _msg);
    void flush() { redisAsyncHandleWrite(connection); }

private :
    Semaphore conSem;
};

typedef Owned<SyncConnection> OwnedSyncConnection;
static OwnedSyncConnection cachedSyncConnection;

typedef Owned<AsyncConnection> OwnedAsyncConnection;
static OwnedAsyncConnection cachedAsyncConnection;

SyncConnection::SyncConnection(ICodeContext * ctx, const char * _options) : Sync::Connection(ctx, _options)
{
   connection = NULL;
   connection = redisConnectWithTimeout(master.str(), port, RedisPlugin::REDIS_TIMEOUT);
   assertConnection();
}


void connectCallback(const redisAsyncContext * connection, int status)
{
	//((Semaphore*)connection->data)->signal();
    if (status != REDIS_OK)
    {
        VStringBuffer msg("Redis Plugin: async connection fail - %s", connection->errstr);
        //printf("%s retrying...\n", msg.str());
        rtlFail(0, msg.str());
    }
}
AsyncConnection::AsyncConnection(ICodeContext * ctx, const char * _options) : Async::Connection(ctx, _options)
{
   connection = NULL;
   connection = redisAsyncConnect(master.str(), port);
   assertConnection();
   initIOCallbacks();
   //assertBufferWriteError(redisAsyncSetConnectCallback(connection, connectCallback), "set connectCallback");
}
void AsyncConnection::assertConnection()
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
    redisContext * connection = redisConnectWithTimeout(master, port, RedisPlugin::REDIS_TIMEOUT);
    StringAttr lockIdFound;
    CriticalBlock block(crit);
    OwnedReply reply = RedisPlugin::createReply(redisCommand(connection, getCmd, key.str(), strlen(key.str())));
    lockIdFound.setown(reply->query()->str);

    if (strcmp(lockId, lockIdFound) == 0)
        OwnedReply reply = RedisPlugin::createReply(redisCommand(connection, "DEL %b", key.str(), strlen(key.str())));

    if (connection)
        redisFree(connection);
    sem.signal();// could be implicit in ~Semaphore()
}
//-----------------------------------------------------------------------------
void AsyncConnection::assertBufferWriteError(int reply, const char * _msg)
{
    if (reply == REDIS_ERR)
    {
        VStringBuffer msg("Redis Plugin: error failed to write '%s' to async io buffer", _msg);
        rtlFail(0, msg.str());
    }
}
bool SyncConnection::missAndLock(ICodeContext * ctx, const KeyLock * keyPtr)
{
    //NOTE: at present other calls to SET will ignore lock and overwrite value.
    StringBuffer cmd("SET %b %b NX EX ");
    cmd.append(Lock::REDIS_LOCK_EXPIRE);

    OwnedReply reply = RedisPlugin::createReply(redisCommand(connection, cmd.str(), keyPtr->getKey(), strlen(keyPtr->getKey()), keyPtr->getLockId(), strlen(keyPtr->getLockId())));
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
void AsyncConnection::assertOnError(const redisReply * reply, const char * _msg)
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
void subCallback(redisAsyncContext * connection, void * _reply, void * _keyPtr)
{
	printf("in callback\n");
	  if (_keyPtr)
	        ((KeyLock*)_keyPtr)->notify();
    if (_reply == NULL)
        return;

    redisReply * reply = (redisReply*)_reply;
    assertCallbackError(reply, "callback fail");


    if (reply->type == REDIS_REPLY_ARRAY) {
        for (int j = 0; j < reply->elements; j++) {
            printf("%u) %s\n", j, reply->element[j]->str);
        }
    }
    //StringAttr replyStr;
    //replyStr.set(reply->str);
    //printf("reply: %s\n", replyStr.str());
    //if (_keyPtr)
      //  ((KeyLock*)_keyPtr)->notify();
}

//----------------------------------GET----------------------------------------
template<class type> void AsyncConnection::getLocked(ICodeContext * ctx, KeyLock * keyPtr, type & returnValue, RedisPlugin::eclDataType eclType)
{
	//assert keyPtr;
    const char * key = keyPtr->getKey();
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
template<class type> void AsyncConnection::getLocked(ICodeContext * ctx, KeyLock * keyPtr, size_t & returnLength, type * & returnValue, RedisPlugin::eclDataType eclType)
{
    const char * channel = keyPtr->getLockId();


    channel = "str";

    printf("setting cmd...\n");
    assertBufferWriteError(redisAsyncCommand(connection, subCallback, (void*)keyPtr, "SUBSCRIBE %b", channel, strlen(channel)*sizeof(char)), "subscription error");
    printf("waiting...\n");
    keyPtr->waitTO();




    /*
    OwnedReply reply = RedisPlugin::createReply(redisCommand(connection, getCmd, key, strlen(key)));

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
    OwnedReply reply = RedisPlugin::createReply(redisCommand(connection, getCmd, key, strlen(key)));
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
    OwnedAsyncConnection master = new AsyncConnection(ctx, keyPtr->getOptions());//createAsyncConnection(ctx, keyPtr->getOptions());
    master->getLocked(ctx, keyPtr, returnValue, eclType);
}
template<class type> void RGetLocked(ICodeContext * ctx, unsigned __int64 _keyPtr, size_t & returnLength, type * & returnValue, RedisPlugin::eclDataType eclType)
{
    KeyLock * keyPtr = reinterpret_cast<KeyLock*>(_keyPtr);
    OwnedAsyncConnection master = new AsyncConnection(ctx, keyPtr->getOptions());//createAsyncConnection(ctx, keyPtr->getOptions());
    master->getLocked(ctx, keyPtr, returnLength, returnValue, eclType);
}
void RGetLockedVoidPtrLenPair(ICodeContext * ctx, unsigned __int64 _keyPtr, size_t & returnLength, void * & returnValue, RedisPlugin::eclDataType eclType)
{
    KeyLock * keyPtr = reinterpret_cast<KeyLock*>(_keyPtr);
    OwnedAsyncConnection master = new AsyncConnection(ctx, keyPtr->getOptions());//createAsyncConnection(ctx, keyPtr->getOptions());
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

ECL_REDIS_API void ECL_REDIS_CALL RPub(ICodeContext * ctx, unsigned __int64 _keyPtr, const char * msg)
{
	KeyLock * keyPtr = reinterpret_cast<KeyLock*>(_keyPtr);
    OwnedSyncConnection master = createSyncConnection(ctx, keyPtr->getOptions());
    master->pub(ctx, keyPtr, msg);
}
void SyncConnection::pub(ICodeContext * ctx, KeyLock * keyPtr, const char * msg)
{
	const char * channel = keyPtr->getKey();
    OwnedReply reply = RedisPlugin::createReply(redisCommand(connection, "PUBLISH %b %s", channel, strlen(channel), msg));

}
}//close namespace
