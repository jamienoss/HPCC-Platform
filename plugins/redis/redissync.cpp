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
   connection = redisConnect(master.str(), port);//redisConnectWithTimeout(master.str(), port, RedisPlugin::REDIS_TIMEOUT);
   assertConnection();
   redisSetTimeout(connection, RedisPlugin::REDIS_TIMEOUT);
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

void Connection::assertConnection()
{
    if (!connection)
        rtlFail(0, "Redis Plugin: 'redisConnect' failed - no error available.");
    else if (connection->err)
    {
        VStringBuffer msg("Redis Plugin: Connection failed - %s", connection->errstr);
        rtlFail(0, msg.str());
    }
}

void Connection::clear(ICodeContext * ctx, unsigned when)
{
    //NOTE: flush is the actual cache flush/clear/delete and not an io buffer flush.
    OwnedReply reply = RedisPlugin::createReply(redisCommand(connection, "FLUSHDB"));//NOTE: FLUSHDB deletes current database where as FLUSHALL deletes all dbs.
    //NOTE: documented as never failing, but in case
    assertOnError(reply->query(), "'Clear' request failed - ");
}
void Connection::del(ICodeContext * ctx, const char * key, const char * partitionKey)
{
    OwnedReply reply = RedisPlugin::createReply(redisCommand(connection, "DEL %b", key, sizeof(key)));
    assertOnError(reply->query(), "'Del' request failed - ");
}
void Connection::persist(ICodeContext * ctx, const char * key, const char * partitionKey)
{
    OwnedReply reply = RedisPlugin::createReply(redisCommand(connection, "PERSIST %b", key, sizeof(key)));
    assertOnError(reply->query(), "'Persist' request failed - ");
}
void Connection::expire(ICodeContext * ctx, const char * key, const char * partitionKey, unsigned _expire)
{
    OwnedReply reply = RedisPlugin::createReply(redisCommand(connection, "DEL %b %u", key, sizeof(key), _expire));
    assertOnError(reply->query(), "'Expire' request failed - ");
}
//-------------------------------------------SET-----------------------------------------
template<class type> void RSet(ICodeContext * ctx, const char * _options, const char * partitionKey, const char * key, type value, unsigned expire, RedisPlugin::eclDataType eclType)
{
    OwnedConnection master = createConnection(ctx, _options);
    master->set(ctx, partitionKey, key, value, expire, eclType);
}
//Set pointer types
template<class type> void RSet(ICodeContext * ctx, const char * _options, const char * partitionKey, const char * key, size32_t valueLength, const type * value, unsigned expire, RedisPlugin::eclDataType eclType)
{
    OwnedConnection master = createConnection(ctx, _options);
    master->set(ctx, partitionKey, key, valueLength, value, expire, eclType);
}
//-------------------------------------------GET-----------------------------------------
template<class type> void RGet(ICodeContext * ctx, const char * options, const char * partitionKey, const char * key, type & returnValue, RedisPlugin::eclDataType eclType)
{
    OwnedConnection master = createConnection(ctx, options);
    master->get(ctx, partitionKey, key, returnValue, eclType);
}
template<class type> void RGet(ICodeContext * ctx, const char * options, const char * partitionKey, const char * key, size_t & returnLength, type * & returnValue, RedisPlugin::eclDataType eclType)
{
    OwnedConnection master = createConnection(ctx, options);
    master->get(ctx, partitionKey, key, returnLength, returnValue, eclType);
}
void RGetVoidPtrLenPair(ICodeContext * ctx, const char * options, const char * partitionKey, const char * key, size_t & returnLength, void * & returnValue, RedisPlugin::eclDataType eclType)
{
    OwnedConnection master = createConnection(ctx, options);
    master->getVoidPtrLenPair(ctx, partitionKey, key, returnLength, returnValue, eclType);
}
//--------------------------------------------------------------------------------
//                           ECL SERVICE ENTRYPOINTS
//--------------------------------------------------------------------------------
ECL_REDIS_API void ECL_REDIS_CALL RClear(ICodeContext * ctx, const char * options)
{
    OwnedConnection master = createConnection(ctx, options);
    master->clear(ctx, 0);
}
ECL_REDIS_API bool ECL_REDIS_CALL RExist(ICodeContext * ctx, const char * options, const char * key, const char * partitionKey)
{
    OwnedConnection master = createConnection(ctx, options);
    return master->exist(ctx, key, partitionKey);
}
/*ECL_REDIS_API const char * ECL_REDIS_CALL RKeyType(ICodeContext * ctx, const char * options, const char * key, const char * partitionKey)
{
    OwnedConnection master = createConnection(ctx, options);
    const char * keyType = enumToStr(master->getKeyType(key, partitionKey));
    return keyType;
}*/
ECL_REDIS_API void ECL_REDIS_CALL RDel(ICodeContext * ctx, const char * options, const char * key, const char * partitionKey)
{
    OwnedConnection master = createConnection(ctx, options);
    master->del(ctx, key, partitionKey);
}
ECL_REDIS_API void ECL_REDIS_CALL RPersist(ICodeContext * ctx, const char * options, const char * key, const char * partitionKey)
{
    OwnedConnection master = createConnection(ctx, options);
    master->persist(ctx, key, partitionKey);
}
ECL_REDIS_API void ECL_REDIS_CALL RExpire(ICodeContext * ctx, const char * options, const char * key, unsigned _expire, const char * partitionKey)
{
    OwnedConnection master = createConnection(ctx, options);
    master->expire(ctx, key, partitionKey, _expire*RedisPlugin::unitExpire);
}
//-----------------------------------SET------------------------------------------
//NOTE: These were all overloaded by 'value' type, however; this caused problems since ecl implicitly casts and doesn't type check.
ECL_REDIS_API void ECL_REDIS_CALL RSet(ICodeContext * ctx, const char * options, const char * key, size32_t valueLength, const char * value, const char * partitionKey, unsigned expire /* = 0 (ECL default)*/)
{
    RSet(ctx, options, partitionKey, key, valueLength, value, expire, RedisPlugin::ECL_STRING);
}
ECL_REDIS_API void ECL_REDIS_CALL RSet(ICodeContext * ctx, const char * options, const char * key, size32_t valueLength, const UChar * value, const char * partitionKey, unsigned expire /* = 0 (ECL default)*/)
{
    RSet(ctx, options, partitionKey, key, (valueLength)*sizeof(UChar), value, expire, RedisPlugin::ECL_UNICODE);
}
ECL_REDIS_API void ECL_REDIS_CALL RSet(ICodeContext * ctx, const char * options, const char * key, signed __int64 value, const char * partitionKey, unsigned expire /* = 0 (ECL default)*/)
{
    RSet(ctx, options, partitionKey, key, value, expire, RedisPlugin::ECL_INTEGER);
}
ECL_REDIS_API void ECL_REDIS_CALL RSet(ICodeContext * ctx, const char * options, const char * key, unsigned __int64 value, const char * partitionKey, unsigned expire /* = 0 (ECL default)*/)
{
    RSet(ctx, options, partitionKey, key, value, expire, RedisPlugin::ECL_UNSIGNED);
}
ECL_REDIS_API void ECL_REDIS_CALL RSet(ICodeContext * ctx, const char * options, const char * key, double value, const char * partitionKey, unsigned expire /* = 0 (ECL default)*/)
{
    RSet(ctx, options, partitionKey, key, value, expire, RedisPlugin::ECL_REAL);
}
ECL_REDIS_API void ECL_REDIS_CALL RSet(ICodeContext * ctx, const char * options, const char * key, bool value, const char * partitionKey, unsigned expire)
{
    RSet(ctx, options, partitionKey, key, value, expire, RedisPlugin::ECL_BOOLEAN);
}
ECL_REDIS_API void ECL_REDIS_CALL RSetData(ICodeContext * ctx, const char * options, const char * key, size32_t valueLength, const void * value, const char * partitionKey, unsigned expire)
{
    RSet(ctx, options, partitionKey, key, valueLength, value, expire, RedisPlugin::ECL_DATA);
}
ECL_REDIS_API void ECL_REDIS_CALL RSetUtf8(ICodeContext * ctx, const char * options, const char * key, size32_t valueLength, const char * value, const char * partitionKey, unsigned expire /* = 0 (ECL default)*/)
{
    RSet(ctx, options, partitionKey, key, rtlUtf8Size(valueLength, value), value, expire, RedisPlugin::ECL_UTF8);
}
//-------------------------------------GET----------------------------------------
ECL_REDIS_API bool ECL_REDIS_CALL RGetBool(ICodeContext * ctx, const char * options, const char * key, const char * partitionKey)
{
    bool value;
    RGet(ctx, options, partitionKey, key, value, RedisPlugin::ECL_BOOLEAN);
    return value;
}
ECL_REDIS_API double ECL_REDIS_CALL RGetDouble(ICodeContext * ctx, const char * options, const char * key, const char * partitionKey)
{
    double value;
    RGet(ctx, options, partitionKey, key, value, RedisPlugin::ECL_REAL);
    return value;
}
ECL_REDIS_API signed __int64 ECL_REDIS_CALL RGetInt8(ICodeContext * ctx, const char * options, const char * key, const char * partitionKey)
{
    signed __int64 value;
    RGet(ctx, options, partitionKey, key, value, RedisPlugin::ECL_INTEGER);
    return value;
}
ECL_REDIS_API unsigned __int64 ECL_REDIS_CALL RGetUint8(ICodeContext * ctx, const char * options, const char * key, const char * partitionKey)
{
    unsigned __int64 value;
    RGet(ctx, options, partitionKey, key, value, RedisPlugin::ECL_UNSIGNED);
    return value;
}
ECL_REDIS_API void ECL_REDIS_CALL RGetStr(ICodeContext * ctx, size32_t & returnLength, char * & returnValue, const char * options, const char * key, const char * partitionKey)
{
    size_t _returnLength;
    RGet(ctx, options, partitionKey, key, _returnLength, returnValue, RedisPlugin::ECL_STRING);
    returnLength = static_cast<size32_t>(_returnLength);
}
ECL_REDIS_API void ECL_REDIS_CALL RGetUChar(ICodeContext * ctx, size32_t & returnLength, UChar * & returnValue,  const char * options, const char * key, const char * partitionKey)
{
    size_t _returnSize;
    RGet(ctx, options, partitionKey, key, _returnSize, returnValue, RedisPlugin::ECL_UNICODE);
    returnLength = static_cast<size32_t>(_returnSize/sizeof(UChar));
}
ECL_REDIS_API void ECL_REDIS_CALL RGetUtf8(ICodeContext * ctx, size32_t & returnLength, char * & returnValue, const char * options, const char * key, const char * partitionKey)
{
    size_t returnSize;
    RGet(ctx, options, partitionKey, key, returnSize, returnValue, RedisPlugin::ECL_UTF8);
    returnLength = static_cast<size32_t>(rtlUtf8Length(returnSize, returnValue));
}
ECL_REDIS_API void ECL_REDIS_CALL RGetData(ICodeContext * ctx, size32_t & returnLength, void * & returnValue, const char * options, const char * key, const char * partitionKey)
{
    size_t _returnLength;
    RGetVoidPtrLenPair(ctx, options, partitionKey, key, _returnLength, returnValue, RedisPlugin::ECL_DATA);
    returnLength = static_cast<size32_t>(_returnLength);
}

//----------------------------------SET----------------------------------------
template<class type> void Connection::set(ICodeContext * ctx, const char * partitionKey, const char * key, type value, unsigned expire, RedisPlugin::eclDataType eclType)
{
    const char * _value = reinterpret_cast<const char *>(&value);//Do this even for char * to prevent compiler complaining
    const char * msg = setFailMsg;

    StringBuffer cmd(setCmd);
    RedisPlugin::appendExpire(cmd, expire);

    OwnedReply reply = RedisPlugin::createReply(redisCommand(connection, cmd.str(), key, strlen(key)*sizeof(char), _value, sizeof(type)));
    assertOnError(reply->query(), msg);
}
template<class type> void Connection::set(ICodeContext * ctx, const char * partitionKey, const char * key, size32_t valueLength, const type * value, unsigned expire, RedisPlugin::eclDataType eclType)
{
    const char * _value = reinterpret_cast<const char *>(value);//Do this even for char * to prevent compiler complaining
    const char * msg = setFailMsg;

    StringBuffer cmd(setCmd);
    RedisPlugin::appendExpire(cmd, expire);
    OwnedReply reply = RedisPlugin::createReply(redisCommand(connection, cmd.str(), key, strlen(key)*sizeof(char), _value, (size_t)valueLength));
    assertOnError(reply->query(), msg);
}
//----------------------------------GET----------------------------------------
template<class type> void Connection::get(ICodeContext * ctx, const char * partitionKey, const char * key, type & returnValue, RedisPlugin::eclDataType eclType)
{
    OwnedReply reply = RedisPlugin::createReply(redisCommand(connection, getCmd, key, strlen(key)*sizeof(char)));

    StringBuffer keyMsg = getFailMsg;
    assertOnError(reply->query(), appendIfKeyNotFoundMsg(reply->query(), key, keyMsg));

    size_t returnSize = reply->query()->len;//*sizeof(type);
    if (sizeof(type)!=returnSize)
    {
        VStringBuffer msg("RedisPlugin: ERROR - Requested type of different size (%uB) from that stored (%uB).", (unsigned)sizeof(type), (unsigned)returnSize);

        rtlFail(0, msg.str());
    }
    memcpy(&returnValue, reply->query()->str, returnSize);
}
template<class type> void Connection::get(ICodeContext * ctx, const char * partitionKey, const char * key, size_t & returnLength, type * & returnValue, RedisPlugin::eclDataType eclType)
{
    OwnedReply reply = RedisPlugin::createReply(redisCommand(connection, getCmd, key, strlen(key)*sizeof(char)));
    ifLockedSub(ctx, key, reply->query());

    StringBuffer keyMsg = getFailMsg;
    assertOnError(reply->query(), appendIfKeyNotFoundMsg(reply->query(), key, keyMsg));

    returnLength = reply->query()->len;
    size_t returnSize = returnLength*sizeof(type);

    returnValue = reinterpret_cast<type*>(cpy(reply->query()->str, returnSize));
}
void Connection::getVoidPtrLenPair(ICodeContext * ctx, const char * partitionKey, const char * key, size_t & returnLength, void * & returnValue, RedisPlugin::eclDataType eclType)
{
    OwnedReply reply = RedisPlugin::createReply(redisCommand(connection, getCmd, key, strlen(key)*sizeof(char)));
    StringBuffer keyMsg = getFailMsg;
    assertOnError(reply->query(), appendIfKeyNotFoundMsg(reply->query(), key, keyMsg));

    returnLength = reply->query()->len;
    returnValue = reinterpret_cast<void*>(cpy(reply->query()->str, reply->query()->len*sizeof(char)));
}

bool Connection::ifLockedSub(ICodeContext * ctx, const char * key, const redisReply * replyorig)
{
	printf("in here\n");
    const char * value = replyorig->str;
    if (strncmp(value, "redis_ecl_lock", 14) == 0)
    {
    	void * reply = redisCommand(connection, "SUBSCRIBE str");//%b", value, strlen(value)*sizeof(char)));
        freeReplyObject(reply);


        while(redisGetReply(connection,&reply) == REDIS_OK)
        {
        	printf("in loop\n");
        redisReply * _reply = (redisReply*)reply;
        if (_reply->type == REDIS_REPLY_ARRAY) {
               for (int j = 0; j < _reply->elements; j++) {
                   printf("%u) %s\n", j, _reply->element[j]->str);
               }
           }
        freeReplyObject(reply);
        }
               //ISocket * soc = ISocket::attach(connection.fd);
        //    if (soc->wait_read(30000) == 1);

        printf("connection err: %s\n", getErrStr());

        return true;
    }
    return false;
}


bool Connection::exist(ICodeContext * ctx, const char * key, const char * partitionKey)
{
    OwnedReply reply = RedisPlugin::createReply(redisCommand(connection, "GET %s", key));

    if (reply->query()->type == REDIS_REPLY_NIL)
        return false;
    else
    {
        assertOnError(reply->query(), "'Exist' request failed - ");
        return true;
    }
}

ECL_REDIS_API void ECL_REDIS_CALL RCmdStr(ICodeContext * ctx, size32_t & returnLength, void * & returnValue, const char * options, const char * cmd)
{
    /*OwnedConnection master = createConnection(ctx, options);
    OwnedReply reply = RedisPlugin::createReply(redisCommand(connection, "GET %s", key));

    RGetVoidPtrLenPair(ctx, options, partitionKey, key, _returnLength, returnValue, RedisPlugin::ECL_DATA);
    returnLength = static_cast<size32_t>(_returnLength);
    */
}
}//close namespace
