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

namespace Lock
{

static const char * REDIS_LOCK_PREFIX = "ajbdfoiuadgf9pqw7t3r973tq";// needs to be a large random value uniquely individual per client
static const unsigned lockExpire = 60; //(secs)
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

class Connection : public Sync::Connection
{
public :
    Connection(ICodeContext * ctx, const char * _options);
    ~Connection()
    {
        if (connection)
            redisFree(connection);
    }

    bool missAndLock(ICodeContext * ctx, const KeyLock * key);

private :
    redisContext * connection;
};

typedef Owned<Connection> OwnedConnection;
static OwnedConnection cachedConnection;

Connection::Connection(ICodeContext * ctx, const char * _options) : Sync::Connection(ctx, _options)
{
   connection = redisConnect(master, port);
   assertConnection();
}

Connection * createConnection(ICodeContext * ctx, const char * options)
{
    CriticalBlock block(crit);
    if (!cachedConnection)
    {
        cachedConnection.setown(new Connection(ctx, options));
        return LINK(cachedConnection);
    }

    if (cachedConnection->isSameConnection(options))
        return LINK(cachedConnection);

    cachedConnection.setown(new Connection(ctx, options));
    return LINK(cachedConnection);
}

KeyLock::KeyLock(const char * _options, const char * _key, const char * _lockId)
{
    options.setown(_options);
    key.set(_key);
    lockId.setown(_lockId);
    RedisPlugin::parseOptions(_options, master, port);
}
KeyLock::~KeyLock()
{
    redisContext * connection = redisConnect(master, port);
    StringAttr lockIdFound;
    OwnedReply reply = RedisPlugin::createReply(redisCommand(connection, getCmd, key.str(), strlen(key.str())*sizeof(char)));
    lockIdFound.setown(reply->query()->str);

    if (strcmp(lockId, lockIdFound) == 0)
        OwnedReply reply = RedisPlugin::createReply(redisCommand(connection, "DEL %b", key.str(), strlen(key.str())*sizeof(char)));

    if (connection)
        redisFree(connection);
}
//-----------------------------------------------------------------------------

bool Connection::missAndLock(ICodeContext * ctx, const KeyLock * keyPtr)
{
    StringBuffer cmd("SET %b %b NX EX ");
    cmd.append(Lock::lockExpire);
    OwnedReply reply = RedisPlugin::createReply(redisCommand(connection, cmd.str(), keyPtr->getKey(), strlen(keyPtr->getKey())*sizeof(char), keyPtr->getLockId(), strlen(keyPtr->getLockId())*sizeof(char)));
    //assertOnError(reply->query(), msg);
    const redisReply * actReply = reply->query();
    if (!actReply)
        return false;
    else if (actReply->type == REDIS_REPLY_STATUS && strcmp(actReply->str, "OK"))
        return true;

    return false;
}

ECL_REDIS_API bool ECL_REDIS_CALL RMissAndLock(ICodeContext * ctx, char * _keyPtr)
{
	printf("inhere\n");
    const KeyLock * keyPtr = reinterpret_cast<const KeyLock*>(_keyPtr);
    if (!keyPtr)
    {
        rtlFail(0, "Redis Plugin: no key specified to 'MissAndLock'");
    }
    OwnedConnection master = createConnection(ctx, keyPtr->getOptions());
    return master->missAndLock(ctx, keyPtr);
}

ECL_REDIS_API char * ECL_REDIS_CALL RGetLockObject(ICodeContext * ctx, const char * options, const char * key)
{
    Owned<KeyLock> keyPtr;
    StringBuffer lockId;
    lockId.set(Lock::REDIS_LOCK_PREFIX);
    lockId.append("iviyfau8sifsdiuf");

    keyPtr.setown(new KeyLock(options, key, lockId.str()));
    return reinterpret_cast<char*>(keyPtr.get());
}

}//close namespace
