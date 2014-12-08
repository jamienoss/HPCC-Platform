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
#include "redisplugin.hpp"
#include "eclrtl.hpp"
#include "jexcept.hpp"
#include "jstring.hpp"

#include <hiredis/hiredis.h>
#include <hiredis/async.h>
//#include "hiredis/adapters/libevent.h"

#define REDIS_VERSION "redis plugin 1.0.0"

ECL_REDIS_API bool getECLPluginDefinition(ECLPluginDefinitionBlock *pb)
{
    if (pb->size != sizeof(ECLPluginDefinitionBlock))
        return false;

    pb->magicVersion = PLUGIN_VERSION;
    pb->version = REDIS_VERSION;
    pb->moduleName = "lib_redis";
    pb->ECL = NULL;
    pb->flags = PLUGIN_IMPLICIT_MODULE;
    pb->description = "ECL plugin library for redis\n";
    return true;
}

namespace RedisPlugin {
IPluginContext * parentCtx = NULL;
static const unsigned unitExpire = 86400;//1 day (secs)

}//namespace

//-----------------------------------SET------------------------------------------
ECL_REDIS_API void ECL_REDIS_CALL RSet(ICodeContext * ctx, const char * servers, const char * key, size32_t valueLength, const char * value, const char * partitionKey, unsigned expire /* = 0 (ECL default)*/)
{
    redisContext * connection = redisConnect("127.0.0.1", 6379);
    if (!connection && connection->err)
        printf("Redis error: %s\n", connection->errstr);

    expire = 10;
    StringBuffer cmd = "SET %s %s EX ";
    void * reply = redisCommand(connection, cmd.append(expire).str(), key, value);

    if (!reply)
    {
        printf("redis SET error: %s\n", connection->errstr);
        freeReplyObject(reply);
        redisFree(connection);
    }

    freeReplyObject(reply);
    redisFree(connection);
}
//-------------------------------------GET----------------------------------------
ECL_REDIS_API void ECL_REDIS_CALL RGetStr(ICodeContext * ctx, size32_t & returnLength, char * & returnValue, const char * servers, const char * key, const char * partitionKey)
{
    returnLength = 0;
    returnValue = NULL;

    redisContext * connection = redisConnect("127.0.0.1", 6379);
    if (!connection && connection->err)
        printf("Redis error: %s\n", connection->errstr);

    redisReply * reply = (redisReply*)redisCommand(connection, "GET %s", key);
    if (!reply && connection->err)
    {
        printf("redis GET error: %s\n", connection->errstr);
        //on errors conection canot be reused
        freeReplyObject(reply);
        redisFree(connection);
        return;
    }

    if (reply->type == REDIS_REPLY_STRING)
    {
        returnLength = reply->len;
    	size_t size = returnLength*sizeof(char);
    	void * value = rtlMalloc(size);
        memcpy(value, reply->str, size);
        returnValue = reinterpret_cast<char*>(value);
    }
    freeReplyObject(reply);
    redisFree(connection);
}







