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
#include "redissync.hpp"

namespace Sync {
static CriticalSection crit;

typedef Owned<Connection> OwnedConnection;
static OwnedConnection cachedConnection;

Connection::Connection(ICodeContext * ctx, const char * _options) : RedisPlugin::Connection(ctx, _options)
{
   context = redisConnectWithTimeout(master.str(), port, RedisPlugin::REDIS_TIMEOUT);
   assertConnection();
   redisSetTimeout(context, RedisPlugin::REDIS_TIMEOUT);
   init(ctx);
}

Connection * createConnection(ICodeContext * ctx, const char * options)
{
    CriticalBlock block(crit);
    if (!cachedConnection)
    {
        cachedConnection.setown(new Connection(ctx, options));
        return LINK(cachedConnection);
    }

    if (cachedConnection->isSameConnection(ctx, options))
        return LINK(cachedConnection);

    cachedConnection.setown(new Connection(ctx, options));
    return LINK(cachedConnection);
}
void Connection::logServerStats(ICodeContext * ctx)
{
    OwnedReply reply = RedisPlugin::createReply(redisCommand(context, "INFO ALL"));
    assertOnError(reply->query(), "'INFO ALL' request failed");
    StringBuffer stats("Redis Plugin : Server stats - ");
    stats.newline().append(reply->query()->str).newline();
    ctx->logString(stats.str());
    alreadyInitialized = true;
}
bool Connection::logErrorOnFail(ICodeContext * ctx, const redisReply * reply, const char * _msg)
{
    if (!reply)
    {
        assertConnection();
        assertOnError(NULL, _msg);
    }
    else if (reply->type == REDIS_REPLY_STRING)
        return false;

    VStringBuffer msg("Redis Plugin: %s%s", _msg, reply->str);
    ctx->logString(msg.str());
    return true;
}

void Connection::assertOnError(const redisReply * reply, const char * _msg)
{
    if (!reply)
    {
        //There should always be a context error if no reply error
        assertConnection();
        VStringBuffer msg("Redis Plugin: %s%s", _msg, "no 'reply' nor connection error");
        rtlFail(0, msg.str());
    }
    else if (reply->type == REDIS_REPLY_ERROR)
    {
        VStringBuffer msg("Redis Plugin: %s%s", _msg, reply->str);
        rtlFail(0, msg.str());
    }
}
void Connection::assertConnection()
{
    if (!context)
        rtlFail(0, "Redis Plugin: 'redisConnect' failed - no error available.");
    else if (context->err)
    {
        VStringBuffer msg("Redis Plugin: Connection failed - %s for %s:%u", context->errstr, master.str(), port);
        rtlFail(0, msg.str());
    }
}

void Connection::clear(ICodeContext * ctx, unsigned when)
{
    //NOTE: flush is the actual cache flush/clear/delete and not an io buffer flush.
    OwnedReply reply = RedisPlugin::createReply(redisCommand(context, "FLUSHDB"));//NOTE: FLUSHDB deletes current database where as FLUSHALL deletes all dbs.
    //NOTE: documented as never failing, but in case
    assertOnError(reply->query(), "'Clear' request failed - ");
}
void Connection::del(ICodeContext * ctx, const char * key)
{
    OwnedReply reply = RedisPlugin::createReply(redisCommand(context, "DEL %b", key, strlen(key)*sizeof(char)));
    assertOnError(reply->query(), "'Del' request failed - ");
}
void Connection::persist(ICodeContext * ctx, const char * key)
{
    OwnedReply reply = RedisPlugin::createReply(redisCommand(context, "PERSIST %b", key, strlen(key)*sizeof(char)));
    assertOnError(reply->query(), "'Persist' request failed - ");
}
void Connection::expire(ICodeContext * ctx, const char * key, unsigned _expire)
{
    OwnedReply reply = RedisPlugin::createReply(redisCommand(context, "DEL %b %u", key, strlen(key)*sizeof(char), _expire));
    assertOnError(reply->query(), "'Expire' request failed - ");
}
//-------------------------------------------SET-----------------------------------------
//--OUTER--
template<class type> void RSet(ICodeContext * ctx, const char * _options, const char * key, type value, unsigned expire)
{
    OwnedConnection master = createConnection(ctx, _options);
    master->set(ctx, key, value, expire);
}
//Set pointer types
template<class type> void RSet(ICodeContext * ctx, const char * _options, const char * key, size32_t valueLength, const type * value, unsigned expire)
{
    OwnedConnection master = createConnection(ctx, _options);
    master->set(ctx, key, valueLength, value, expire);
}
//--INNER--
template<class type> void Connection::set(ICodeContext * ctx, const char * key, type value, unsigned expire)
{
    const char * _value = reinterpret_cast<const char *>(&value);//Do this even for char * to prevent compiler complaining
    const char * msg = setFailMsg;

    StringBuffer cmd(setCmd);
    RedisPlugin::appendExpire(cmd, expire);

    OwnedReply reply = RedisPlugin::createReply(redisCommand(context, cmd.str(), key, strlen(key)*sizeof(char), _value, sizeof(type)));
    assertOnError(reply->query(), msg);
}
template<class type> void Connection::set(ICodeContext * ctx, const char * key, size32_t valueLength, const type * value, unsigned expire)
{
    const char * _value = reinterpret_cast<const char *>(value);//Do this even for char * to prevent compiler complaining
    const char * msg = setFailMsg;

    StringBuffer cmd(setCmd);
    RedisPlugin::appendExpire(cmd, expire);
    OwnedReply reply = RedisPlugin::createReply(redisCommand(context, cmd.str(), key, strlen(key)*sizeof(char), _value, (size_t)valueLength));
    assertOnError(reply->query(), msg);
}
//-------------------------------------------GET-----------------------------------------
//--OUTER--
template<class type> void RGet(ICodeContext * ctx, const char * options, const char * key, type & returnValue)
{
    OwnedConnection master = createConnection(ctx, options);
    master->get(ctx, key, returnValue);
}
template<class type> void RGet(ICodeContext * ctx, const char * options, const char * key, size_t & returnLength, type * & returnValue)
{
    OwnedConnection master = createConnection(ctx, options);
    master->get(ctx, key, returnLength, returnValue);
}
void RGetVoidPtrLenPair(ICodeContext * ctx, const char * options, const char * key, size_t & returnLength, void * & returnValue)
{
    OwnedConnection master = createConnection(ctx, options);
    master->getVoidPtrLenPair(ctx, key, returnLength, returnValue);
}
//--INNER--
template<class type> void Connection::get(ICodeContext * ctx, const char * key, type & returnValue)
{
    OwnedReply reply = RedisPlugin::createReply(redisCommand(context, getCmd, key, strlen(key)*sizeof(char)));

    StringBuffer keyMsg = getFailMsg;
    assertOnError(reply->query(), appendIfKeyNotFoundMsg(reply->query(), key, keyMsg));

    size_t returnSize = reply->query()->len;
    if (sizeof(type)!=returnSize)
    {
        VStringBuffer msg("RedisPlugin: ERROR - Requested type of different size (%uB) from that stored (%uB).", (unsigned)sizeof(type), (unsigned)returnSize);

        rtlFail(0, msg.str());
    }
    memcpy(&returnValue, reply->query()->str, returnSize);
}
template<class type> void Connection::get(ICodeContext * ctx, const char * key, size_t & returnLength, type * & returnValue)
{
    OwnedReply reply = RedisPlugin::createReply(redisCommand(context, getCmd, key, strlen(key)*sizeof(char)));

    StringBuffer keyMsg = getFailMsg;
    assertOnError(reply->query(), appendIfKeyNotFoundMsg(reply->query(), key, keyMsg));

    returnLength = reply->query()->len;
    size_t returnSize = returnLength*sizeof(type);

    returnValue = reinterpret_cast<type*>(cpy(reply->query()->str, returnSize));
}
void Connection::getVoidPtrLenPair(ICodeContext * ctx, const char * key, size_t & returnLength, void * & returnValue)
{
    OwnedReply reply = RedisPlugin::createReply(redisCommand(context, getCmd, key, strlen(key)*sizeof(char)));
    StringBuffer keyMsg = getFailMsg;
    assertOnError(reply->query(), appendIfKeyNotFoundMsg(reply->query(), key, keyMsg));

    returnLength = reply->query()->len;
    returnValue = reinterpret_cast<void*>(cpy(reply->query()->str, reply->query()->len*sizeof(char)));
}

bool Connection::exist(ICodeContext * ctx, const char * key)
{
    OwnedReply reply = RedisPlugin::createReply(redisCommand(context, "EXISTS %b", key, strlen(key)*sizeof(char)));
    assertOnError(reply->query(), "'Exist' request failed - ");
    return (reply->query()->integer != 0);
}
//--------------------------------------------------------------------------------
//                           ECL SERVICE ENTRYPOINTS
//--------------------------------------------------------------------------------
ECL_REDIS_API void ECL_REDIS_CALL RClear(ICodeContext * ctx, const char * options)
{
    OwnedConnection master = createConnection(ctx, options);
    master->clear(ctx, 0);
}
ECL_REDIS_API bool ECL_REDIS_CALL RExist(ICodeContext * ctx, const char * options, const char * key)
{
    OwnedConnection master = createConnection(ctx, options);
    return master->exist(ctx, key);
}
ECL_REDIS_API void ECL_REDIS_CALL RDel(ICodeContext * ctx, const char * options, const char * key)
{
    OwnedConnection master = createConnection(ctx, options);
    master->del(ctx, key);
}
ECL_REDIS_API void ECL_REDIS_CALL RPersist(ICodeContext * ctx, const char * options, const char * key)
{
    OwnedConnection master = createConnection(ctx, options);
    master->persist(ctx, key);
}
ECL_REDIS_API void ECL_REDIS_CALL RExpire(ICodeContext * ctx, const char * options, const char * key, unsigned _expire)
{
    OwnedConnection master = createConnection(ctx, options);
    master->expire(ctx, key, _expire*RedisPlugin::unitExpire);
}
//-----------------------------------SET------------------------------------------
ECL_REDIS_API void ECL_REDIS_CALL RSetStr(ICodeContext * ctx, const char * options, const char * key, size32_t valueLength, const char * value, unsigned expire /* = 0 (ECL default)*/)
{
    RSet(ctx, options, key, valueLength, value, expire);
}
ECL_REDIS_API void ECL_REDIS_CALL RSetUChar(ICodeContext * ctx, const char * options, const char * key, size32_t valueLength, const UChar * value, unsigned expire /* = 0 (ECL default)*/)
{
    RSet(ctx, options, key, (valueLength)*sizeof(UChar), value, expire);
}
ECL_REDIS_API void ECL_REDIS_CALL RSetInt(ICodeContext * ctx, const char * options, const char * key, signed __int64 value, unsigned expire /* = 0 (ECL default)*/)
{
    RSet(ctx, options, key, value, expire);
}
ECL_REDIS_API void ECL_REDIS_CALL RSetUInt(ICodeContext * ctx, const char * options, const char * key, unsigned __int64 value, unsigned expire /* = 0 (ECL default)*/)
{
    RSet(ctx, options, key, value, expire);
}
ECL_REDIS_API void ECL_REDIS_CALL RSetReal(ICodeContext * ctx, const char * options, const char * key, double value, unsigned expire /* = 0 (ECL default)*/)
{
    RSet(ctx, options, key, value, expire);
}
ECL_REDIS_API void ECL_REDIS_CALL RSetBool(ICodeContext * ctx, const char * options, const char * key, bool value, unsigned expire)
{
    RSet(ctx, options, key, value, expire);
}
ECL_REDIS_API void ECL_REDIS_CALL RSetData(ICodeContext * ctx, const char * options, const char * key, size32_t valueLength, const void * value, unsigned expire)
{
    RSet(ctx, options, key, valueLength, value, expire);
}
ECL_REDIS_API void ECL_REDIS_CALL RSetUtf8(ICodeContext * ctx, const char * options, const char * key, size32_t valueLength, const char * value, unsigned expire /* = 0 (ECL default)*/)
{
    RSet(ctx, options, key, rtlUtf8Size(valueLength, value), value, expire);
}
//-------------------------------------GET----------------------------------------
ECL_REDIS_API bool ECL_REDIS_CALL RGetBool(ICodeContext * ctx, const char * options, const char * key)
{
    bool value;
    RGet(ctx, options, key, value);
    return value;
}
ECL_REDIS_API double ECL_REDIS_CALL RGetDouble(ICodeContext * ctx, const char * options, const char * key)
{
    double value;
    RGet(ctx, options, key, value);
    return value;
}
ECL_REDIS_API signed __int64 ECL_REDIS_CALL RGetInt8(ICodeContext * ctx, const char * options, const char * key)
{
    signed __int64 value;
    RGet(ctx, options, key, value);
    return value;
}
ECL_REDIS_API unsigned __int64 ECL_REDIS_CALL RGetUint8(ICodeContext * ctx, const char * options, const char * key)
{
    unsigned __int64 value;
    RGet(ctx, options, key, value);
    return value;
}
ECL_REDIS_API void ECL_REDIS_CALL RGetStr(ICodeContext * ctx, size32_t & returnLength, char * & returnValue, const char * options, const char * key)
{
    size_t _returnLength;
    RGet(ctx, options, key, _returnLength, returnValue);
    returnLength = static_cast<size32_t>(_returnLength);
}
ECL_REDIS_API void ECL_REDIS_CALL RGetUChar(ICodeContext * ctx, size32_t & returnLength, UChar * & returnValue,  const char * options, const char * key)
{
    size_t _returnSize;
    RGet(ctx, options, key, _returnSize, returnValue);
    returnLength = static_cast<size32_t>(_returnSize/sizeof(UChar));
}
ECL_REDIS_API void ECL_REDIS_CALL RGetUtf8(ICodeContext * ctx, size32_t & returnLength, char * & returnValue, const char * options, const char * key)
{
    size_t returnSize;
    RGet(ctx, options, key, returnSize, returnValue);
    returnLength = static_cast<size32_t>(rtlUtf8Length(returnSize, returnValue));
}
ECL_REDIS_API void ECL_REDIS_CALL RGetData(ICodeContext * ctx, size32_t & returnLength, void * & returnValue, const char * options, const char * key)
{
    size_t _returnLength;
    RGetVoidPtrLenPair(ctx, options, key, _returnLength, returnValue);
    returnLength = static_cast<size32_t>(_returnLength);
}
}//close Sync namespace
