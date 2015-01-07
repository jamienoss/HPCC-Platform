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
    pb->description = "ECL plugin library for the C API hiredis\n";
    return true;
}

namespace RedisPlugin {
IPluginContext * parentCtx = NULL;

const char * enumToStr(RedisPlugin::eclDataType type)
{
    switch(type)
    {
    case ECL_BOOLEAN:
        return "BOOLEAN";
    case ECL_INTEGER:
        return "INTEGER";
    case ECL_UNSIGNED:
        return "UNSIGNED";
    case ECL_REAL:
        return "REAL";
    case ECL_STRING:
        return "STRING";
    case ECL_UTF8:
        return "UTF8";
    case ECL_UNICODE:
        return "UNICODE";
    case ECL_DATA:
        return "DATA";
    case ECL_NONE:
        return "Nonexistent";
    default:
        return "UNKNOWN";
    }
}

StringBuffer & appendExpire(StringBuffer & buffer, unsigned expire)
{
    if (expire > 0)
        buffer.append(" EX ").append(expire*RedisPlugin::unitExpire);
    return buffer;
}

Reply * createReply(void * _reply) { return new Reply(_reply); }

const char * Reply::typeToStr() const
{
    switch (reply->type)
    {
    case REDIS_REPLY_STATUS :
        return "REDIS_REPLY_STATUS";
    case REDIS_REPLY_INTEGER :
        return "REDIS_REPLY_INTEGER";
    case REDIS_REPLY_NIL :
            return "REDIS_REPLY_NIL";
    case REDIS_REPLY_STRING :
            return "REDIS_REPLY_STRING";
    case REDIS_REPLY_ERROR :
            return "REDIS_REPLY_ERROR";
    case REDIS_REPLY_ARRAY :
            return "REDIS_REPLY_ARRAY";
    default :
        return "UKNOWN";
    }
}

static CriticalSection crit;
typedef Owned<RedisPlugin::Connection> OwnedConnection;
static OwnedConnection cachedConnection;

Connection * createConnection(ICodeContext * ctx, const char * options)
{
    CriticalBlock block(crit);
    if (!cachedConnection)
    {
        cachedConnection.setown(new RedisPlugin::Connection(ctx, options));
        return LINK(cachedConnection);
    }

    if (cachedConnection->isSameConnection(options))
        return LINK(cachedConnection);

    cachedConnection.setown(new RedisPlugin::Connection(ctx, options));
    return LINK(cachedConnection);
}
}//close namespace

ECL_REDIS_API void setPluginContext(IPluginContext * ctx) { RedisPlugin::parentCtx = ctx; }

void RedisPlugin::parseOptions(const char * _options, StringAttr & master, unsigned & port)
{
    StringArray optionStrings;
    optionStrings.appendList(_options, " ");
    ForEachItemIn(idx, optionStrings)
    {
        const char *opt = optionStrings.item(idx);
        if (strncmp(opt, "--SERVER=", 9) ==0)
        {
            opt += 9;
            StringArray splitPort;
            splitPort.appendList(opt, ":");
            if (splitPort.ordinality()==2)
            {
                master.set(splitPort.item(0));
                port = atoi(splitPort.item(1));
            }
            else
            {
                master.set("localhost");//MORE: May need to explicitly be 127.0.0.1
                port = 6379;
            }
         }
        else
        {
            VStringBuffer err("RedisPlugin: unsupported option string %s", opt);
            rtlFail(0, err.str());
        }
    }
}

RedisPlugin::Connection::Connection(ICodeContext * ctx, const char * _options)
{
    alreadyInitialized = false;
    options.set(_options);
    typeMismatchCount = 0;
    RedisPlugin::parseOptions(_options, master, port);

    /*if (!alreadyInitialized)
    {
        init(ctx);//doesn't necessarily initialize anything, instead outputs specs etc for debugging
        alreadyInitialized = true;
    }*/
}
//-----------------------------------------------------------------------------

bool RedisPlugin::Connection::isSameConnection(const char * _options) const
{
    if (!_options)
        return false;

    StringAttr newMaster;
    unsigned newPort = 0;
    RedisPlugin::parseOptions(_options, newMaster, newPort);

    return stricmp(master.get(), newMaster.get()) == 0 && port == newPort;
}

void * RedisPlugin::Connection::cpy(const char * src, size_t length)
{
    void * value = rtlMalloc(length);
    return memcpy(value, src, length);
}

void RedisPlugin::Connection::checkServersUp(ICodeContext * ctx)
{
   /* redisReply * reply;
    char * args = NULL;

    OwnedMalloc<memcached_stat_st> stats;
    stats.setown(memcached_stat(connection, args, &reply));
    assertex(stats);

    unsigned int numberOfServers = memcached_server_count(connection);
    unsigned int numberOfServersDown = 0;
    for (unsigned i = 0; i < numberOfServers; ++i)
    {
        if (stats[i].pid == -1)//perhaps not the best test?
        {
            numberOfServersDown++;
            VStringBuffer msg("Memcached Plugin: Failed connecting to entry %u\nwithin the server list: %s", i+1, options.str());
            ctx->logString(msg.str());
        }
    }
    if (numberOfServersDown == numberOfServers)
        rtlFail(0,"Memcached Plugin: Failed connecting to ALL servers. Check memcached on all servers and \"memcached -B ascii\" not used.");

    //check memcached version homogeneity
    for (unsigned i = 0; i < numberOfServers-1; ++i)
    {
        if (strcmp(stats[i].version, stats[i+1].version) != 0)
            ctx->logString("Memcached Plugin: Inhomogeneous versions of memcached across servers.");
    }
    */
}

const char * RedisPlugin::Connection::appendIfKeyNotFoundMsg(const redisReply * reply, const char * key, StringBuffer & target) const
{
    if (reply && reply->type == REDIS_REPLY_NIL)
        target.append("(key: '").append(key).append("') ");
    return target.str();
}

/*RedisPlugin::RedisPlugin::eclDataType RedisPlugin::Connection::getKeyType(const char * key, const char * partitionKey)
{
    size_t returnValueLength;
    uint32_t flag;
    OwnedReply reply;

    size_t partitionKeyLength = strlen(partitionKey);
    if (partitionKeyLength)
        memcached_get_by_key(connection, partitionKey, partitionKeyLength, key, strlen(key), &returnValueLength, &flag, &reply);
    else
        memcached_get(connection, key, strlen(key), &returnValueLength, &flag, &reply);

    if (reply == MEMCACHED_SUCCESS)
        return (RedisPlugin::eclDataType)(flag);
    else if (reply == MEMCACHED_NOTFOUND)
        return ECL_NONE;
    else
    {
        VStringBuffer msg("Memcached Plugin: 'KeyType' request failed - %s", memcached_strerror(connection, reply));
        rtlFail(0, msg.str());
    }

}*/

/*void RedisPlugin::Connection::reportKeyTypeMismatch(ICodeContext * ctx, const char * key, uint32_t flag, RedisPlugin::eclDataType eclType)
{
    if (flag && eclType != ECL_DATA && flag != eclType)
    {
        VStringBuffer msg("Memcached Plugin: The requested key '%s' is of type %s, not %s as requested.", key, enumToStr((RedisPlugin::eclDataType)(flag)), enumToStr(eclType));
        if (++typeMismatchCount <= MAX_TYPEMISMATCHCOUNT)
            ctx->logString(msg.str());
    }
}*/

void RedisPlugin::Connection::logServerStats(ICodeContext * ctx)
{
   /* //NOTE: errors are ignored here so that at least some info is reported, such as non-connection related libmemcached version numbers
    OwnedReply reply;
    char * args = NULL;

    OwnedMalloc<memcached_stat_st> stats;
    stats.setown(memcached_stat(connection, args, &reply));

    OwnedMalloc<char*> keys;
    keys.setown(memcached_stat_get_keys(connection, stats, &reply));

    unsigned int numberOfServers = memcached_server_count(connection);
    for (unsigned int i = 0; i < numberOfServers; ++i)
    {
        StringBuffer statsStr;
        unsigned j = 0;
        do
        {
            OwnedMalloc<char> value;
            value.setown(memcached_stat_get_value(connection, &stats[i], keys[j], &reply));
            statsStr.newline().append("libmemcached server stat - ").append(keys[j]).append(":").append(value);
        } while (keys[++j]);
        statsStr.newline().append("libmemcached client stat - libmemcached version:").append(memcached_lib_version());
        ctx->logString(statsStr.str());
    }
    */
}

void RedisPlugin::Connection::init(ICodeContext * ctx)
{
    logServerStats(ctx);
}



