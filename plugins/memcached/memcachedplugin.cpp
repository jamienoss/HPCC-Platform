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
#include "memcachedplugin.hpp"
#include "eclrtl.hpp"
#include "jexcept.hpp"
#include "jstring.hpp"
#include "workunit.hpp"
#include <libmemcached/util.h>

#define MEMCACHED_VERSION "memcached plugin 1.0.0"

ECL_MEMCACHED_API bool getECLPluginDefinition(ECLPluginDefinitionBlock *pb)
{
    if (pb->size != sizeof(ECLPluginDefinitionBlock))
        return false;

    pb->magicVersion = PLUGIN_VERSION;
    pb->version = MEMCACHED_VERSION;
    pb->moduleName = "memcached";
    pb->ECL = NULL;
    pb->flags = PLUGIN_IMPLICIT_MODULE;
    pb->description = "ECL plugin library for the C API libmemcached (http://libmemcached.org/)\n";
    return true;
}

namespace MemCachedPlugin {
IPluginContext * parentCtx = NULL;
static const unsigned unitExpire = 86400;//1 day (secs)

enum eclDataType {
    ECL_BOOLEAN,
    ECL_DATA,
    ECL_INTEGER,
    ECL_REAL,
    ECL_STRING,
    ECL_UTF8,
    ECL_UNICODE,
    ECL_UNSIGNED,
    ECL_NONE
};

const char * enumToStr(eclDataType type)
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

class MCached : public CInterface
{
public :
    MCached(ICodeContext * ctx, const char * servers);
    ~MCached();

    //set
    template <class type> bool set(ICodeContext * ctx, const char * partitionKey, const char * key, type value, unsigned expire, eclDataType eclType);
    template <class type> bool set(ICodeContext * ctx, const char * partitionKey, const char * key, size32_t valueLength, const type * value, unsigned expire, eclDataType eclType);
    //get
    template <class type> void get(ICodeContext * ctx, const char * partitionKey, const char * key, type & value, eclDataType eclType);
    template <class type> void get(ICodeContext * ctx, const char * partitionKey, const char * key, size_t & valueLength, type * & value, eclDataType eclType);
    void getVoidPtrLenPair(ICodeContext * ctx, const char * partitionKey, const char * key, size_t & valueLength, void * & value, eclDataType eclType);

    bool clear(ICodeContext * ctx, unsigned when);
    bool exist(ICodeContext * ctx, const char * key, const char * partitionKey);
    eclDataType getKeyType(const char * key, const char * partitionKey);

    bool isSameConnection(const char * _servers) const;

private :
    void checkServersUp(ICodeContext * ctx);
    void assertOnError(memcached_return_t error, const char * msgSuffix = "");
    StringBuffer keyNotFoundMsg(memcached_return_t error, const char * key) const;
    void connect(ICodeContext * ctx);
    bool reportErrorOnFail(ICodeContext * ctx, memcached_return_t error);
    void reportKeyTypeMismatch(ICodeContext * ctx, const char * key, uint32_t flag, eclDataType eclType);
    void * cpy(const char * src, size_t length);
    void logServerStats(ICodeContext * ctx);
    void init(ICodeContext * ctx);
    void invokeSecurity(ICodeContext * ctx);
    void setSettings();
    void assertPool();//For internal purposes to insure correct order of the above processes and instantiation.

private :
    memcached_st * connection;
    memcached_pool_st * pool;
    StringAttr servers;
    static bool alreadyInitialized;
};

#define OwnedMCached Owned<MemCachedPlugin::MCached>

#define MAX_TYPEMISMATCHCOUNT 10
static unsigned typeMismatchCount = 0;

static CriticalSection crit;
static OwnedMCached cachedConnection;

MCached * createConnection(ICodeContext * ctx, const char * servers)
{
    CriticalBlock block(crit);
    if (!cachedConnection)
    {
        cachedConnection.set(new MemCachedPlugin::MCached(ctx, servers));
        return LINK(cachedConnection);
    }

    if (cachedConnection->isSameConnection(servers))
        return LINK(cachedConnection);

    cachedConnection.set(new MemCachedPlugin::MCached(ctx, servers));
    return LINK(cachedConnection);
}

//-------------------------------------------SET-----------------------------------------
template<class type> bool MSet(ICodeContext * ctx, const char * _servers, const char * partitionKey, const char * key, type value, unsigned expire, eclDataType eclType)
{
    OwnedMCached connection = createConnection(ctx, _servers);
    bool success = connection->set(ctx, partitionKey, key, value, expire, eclType);
    return success;
}
//Set pointer types
template<class type> bool MSet(ICodeContext * ctx, const char * _servers, const char * partitionKey, const char * key, size32_t valueLength, const type * value, unsigned expire, eclDataType eclType)
{
    OwnedMCached connection = createConnection(ctx, _servers);
    bool success = connection->set(ctx, partitionKey, key, valueLength, value, expire, eclType);
    return success;
}
//-------------------------------------------GET-----------------------------------------
template<class type> void MGet(ICodeContext * ctx, const char * servers, const char * partitionKey, const char * key, type & returnValue, eclDataType eclType)
{
    OwnedMCached connection = createConnection(ctx, servers);
    connection->get(ctx, partitionKey, key, returnValue, eclType);
}
template<class type> void MGet(ICodeContext * ctx, const char * servers, const char * partitionKey, const char * key, size_t & returnLength, type * & returnValue, eclDataType eclType)
{
    OwnedMCached connection = createConnection(ctx, servers);
    connection->get(ctx, partitionKey, key, returnLength, returnValue, eclType);
}
void MGetVoidPtrLenPair(ICodeContext * ctx, const char * servers, const char * partitionKey, const char * key, size_t & returnLength, void * & returnValue, eclDataType eclType)
{
    OwnedMCached connection = createConnection(ctx, servers);
    connection->getVoidPtrLenPair(ctx, partitionKey, key, returnLength, returnValue, eclType);
}
}//close namespace

//----------------------------------SET----------------------------------------
template<class type> bool MemCachedPlugin::MCached::set(ICodeContext * ctx, const char * partitionKey, const char * key, type value, unsigned expire, eclDataType eclType)
{
    const char * _value = reinterpret_cast<const char *>(&value);//Do this even for char * to prevent compiler complaining
    return !reportErrorOnFail(ctx, memcached_set_by_key(connection, partitionKey, strlen(partitionKey), key, strlen(key), _value, sizeof(value), (time_t)(expire*unitExpire), (uint32_t)eclType));
}
template<class type> bool MemCachedPlugin::MCached::set(ICodeContext * ctx, const char * partitionKey, const char * key, size32_t valueLength, const type * value, unsigned expire, eclDataType eclType)
{
    const char * _value = reinterpret_cast<const char *>(value);//Do this even for char * to prevent compiler complaining
    return !reportErrorOnFail(ctx, memcached_set_by_key(connection, partitionKey, strlen(partitionKey), key, strlen(key), _value, (size_t)(valueLength), (time_t)(expire*unitExpire), (uint32_t)eclType));
}
//----------------------------------GET----------------------------------------
template<class type> void MemCachedPlugin::MCached::get(ICodeContext * ctx, const char * partitionKey, const char * key, type & returnValue, eclDataType eclType)
{
    uint32_t flag;
    size_t returnLength;
    memcached_return_t error;

    char * _value = memcached_get_by_key(connection, partitionKey, strlen(partitionKey), key, strlen(key), &returnLength, &flag, &error);
    assertOnError(error, keyNotFoundMsg(error, key).str());
    reportKeyTypeMismatch(ctx, key, flag, eclType);

    if (sizeof(type)!=returnLength)
    {
        StringBuffer msg = "MemCachedPlugin: ERROR - Requested type (";
        msg.appendulong(sizeof(type)).append("B) of different size from that stored (").append(returnValue).append("B). Check logs for more information.");
        rtlFail(0, msg.str());
    }
    memcpy(&returnValue, _value, returnLength);

    free(_value);
}
template<class type> void MemCachedPlugin::MCached::get(ICodeContext * ctx, const char * partitionKey, const char * key, size_t & returnLength, type * & returnValue, eclDataType eclType)
{
    uint32_t flag;
    memcached_return_t error;

    char * _value = memcached_get_by_key(connection, partitionKey, strlen(partitionKey), key, strlen(key), &returnLength, &flag, &error);
    assertOnError(error, keyNotFoundMsg(error, key).str());
    reportKeyTypeMismatch(ctx, key, flag, eclType);

    returnValue = reinterpret_cast<type*>(cpy(_value, returnLength));
    free(_value);
}
void MemCachedPlugin::MCached::getVoidPtrLenPair(ICodeContext * ctx, const char * partitionKey, const char * key, size_t & returnLength, void * & returnValue, eclDataType eclType)
{
    size_t returnValueLength = 0;
    uint32_t flag;
    memcached_return_t error;

    char * _value = memcached_get_by_key(connection, partitionKey, strlen(partitionKey), key, strlen(key), &returnValueLength, &flag, &error);
    assertOnError(error, keyNotFoundMsg(error, key).str());
    reportKeyTypeMismatch(ctx, key, flag, eclType);

    returnLength = (size32_t)(returnValueLength);
    returnValue = reinterpret_cast<void*>(cpy(_value, returnLength));
    free(_value);
}

bool MemCachedPlugin::MCached::alreadyInitialized = false;

ECL_MEMCACHED_API void setPluginContext(IPluginContext * ctx) { MemCachedPlugin::parentCtx = ctx; }

MemCachedPlugin::MCached::MCached(ICodeContext * ctx, const char * _servers)
{
    connection = NULL;
    pool = NULL;
    servers.set(_servers);

    pool = memcached_pool(_servers, strlen(_servers));
    assertPool();

    setSettings();
    invokeSecurity(ctx);
    connect(ctx);
    checkServersUp(ctx);
}
//-----------------------------------------------------------------------------
MemCachedPlugin::MCached::~MCached()
{
    if (pool)
    {
        //memcached_destroy_sasl_auth_data(connection);//NOTE: not in any libs but present in src, check install logs
        memcached_pool_release(pool, connection);
        connection = NULL;//For safety (from changing this destructor) as not implicit in either the above or below.
        memcached_pool_destroy(pool);
    }
    else if (connection)//This should never be needed but just in case.
    {
        //memcached_destroy_sasl_auth_data(connection);
        memcached_free(connection);
    }
}

bool MemCachedPlugin::MCached::isSameConnection(const char * _servers) const
{
    if (!_servers)
        return false;

    return stricmp(servers.get(), _servers) == 0;
}

void MemCachedPlugin::MCached::assertPool()
{
    if (!pool)
    {
        StringBuffer msg = "Memcached Plugin: Failed to instantiate server pool for:";
        msg.newline().append(servers);
        rtlFail(0, msg.str());
    }
}

void * MemCachedPlugin::MCached::cpy(const char * src, size_t length)
{
    void * value = rtlMalloc(length);
    return memcpy(value, src, length);
}

void MemCachedPlugin::MCached::checkServersUp(ICodeContext * ctx)
{
    memcached_return_t error;
    char * args = NULL;

    memcached_stat_st * stats = memcached_stat(connection, args, &error);
    assertex(stats);

    unsigned int numberOfServers = memcached_server_count(connection);
    unsigned int numberOfServersDown = 0;
    for (unsigned i = 0; i < numberOfServers; ++i)
    {
        if (stats[i].pid == -1)//perhaps not the best test?
        {
            numberOfServersDown++;
            StringBuffer msg = "Memcached Plugin: Failed connecting to entry ";
            msg.append(i+1).newline().append("within the server list: ").append(servers);
            ctx->addWuException(msg.str(), WRN_FROM_PLUGIN, ExceptionSeverityWarning, "");
        }
    }
    if (numberOfServersDown == numberOfServers)
    {
        free(stats);
        rtlFail(0,"Memcached Plugin: Failed connecting to ALL servers. Check memcached on all servers and \"memcached -B ascii\" not used.");
    }

    //check memcached version homogeneity
    for (unsigned i = 0; i < numberOfServers-1; ++i)
    {
        if (strcmp(stats[i].version, stats[i+1].version) != 0)
            ctx->addWuException("Memcached Plugin: Inhomogeneous versions of memcached across servers.", WRN_FROM_PLUGIN, ExceptionSeverityInformation, "");
    }
    free(stats);
}

bool MemCachedPlugin::MCached::reportErrorOnFail(ICodeContext * ctx, memcached_return_t error)
{
    if (error == MEMCACHED_SUCCESS)
        return false;

    StringBuffer msg = "Memcached Plugin: ";
    ctx->addWuException(msg.append(memcached_strerror(connection, error)).str(), ERR_FROM_PLUGIN, ExceptionSeverityInformation, "");
    return true;
}

void MemCachedPlugin::MCached::assertOnError(memcached_return_t error, const char * msgSuffix)
{
    if (error != MEMCACHED_SUCCESS)
    {
        StringBuffer msg = "Memcached Plugin: ";
        msg.append(memcached_strerror(connection, error)).append(msgSuffix);
        rtlFail(0, msg.str());
    }
}

StringBuffer MemCachedPlugin::MCached::keyNotFoundMsg(memcached_return_t error, const char * key) const
{
    StringBuffer msg;
    if (error == MEMCACHED_NOTFOUND)
    {
        msg = " (key: '";
        msg.append(key).append("') ");
    }
    return msg;
}

bool MemCachedPlugin::MCached::clear(ICodeContext * ctx, unsigned when)
{
    //NOTE: memcached_flush is the actual cache flush and not a io buffer flush.
    return !reportErrorOnFail(ctx, memcached_flush(connection, (time_t)(when)));
}

bool MemCachedPlugin::MCached::exist(ICodeContext * ctx, const char * key, const char * partitionKey)
{
    memcached_return_t error = memcached_exist_by_key(connection, partitionKey, strlen(partitionKey), key, strlen(key));

    if (error == MEMCACHED_SUCCESS)
        return true;
    else if (error == MEMCACHED_NOTFOUND)
        return false;

    reportErrorOnFail(ctx, error);
    return false;
}

MemCachedPlugin::eclDataType MemCachedPlugin::MCached::getKeyType(const char * key, const char * partitionKey)
{
    size_t returnValueLength;
    uint32_t flag;
    memcached_return_t error;

    memcached_get_by_key(connection, partitionKey, strlen(partitionKey), key, strlen(key), &returnValueLength, &flag, &error);
    if (error == MEMCACHED_SUCCESS)
        return (MemCachedPlugin::eclDataType)(flag);
    else if (error == MEMCACHED_NOTFOUND)
        return ECL_NONE;
    else
    {
        StringBuffer msg = "Memcached Plugin: ";
        rtlFail(0, msg.append(memcached_strerror(connection, error)).str());
    }
}

void MemCachedPlugin::MCached::reportKeyTypeMismatch(ICodeContext * ctx, const char * key, uint32_t flag, eclDataType eclType)
{
    if (flag && eclType != ECL_DATA && flag != eclType)
    {
        StringBuffer msg = "Memcached Plugin: The requested key '";
        msg.append(key).append("' is of type ").append(enumToStr((eclDataType)(flag))).append(", not ").append(enumToStr(eclType)).append(" as requested.\n");

        if (++typeMismatchCount <= MAX_TYPEMISMATCHCOUNT)
            ctx->addWuException(msg.str(), WRN_FROM_PLUGIN, ExceptionSeverityInformation, "");
        else
            ctx->addWuException(msg.str(), ERR_FROM_PLUGIN, ExceptionSeverityError, "");
    }
}

void MemCachedPlugin::MCached::logServerStats(ICodeContext * ctx)
{
    memcached_return_t * error = NULL;
    char * args = NULL;

    memcached_stat_st * stats = memcached_stat(connection, args, error);
    char **keys = memcached_stat_get_keys(connection, stats, error);

    StringBuffer statsStr;
    unsigned i = 0;
    do
    {
        char * value = memcached_stat_get_value(connection, stats, keys[i], error);
        statsStr.append("libmemcached server stat - ").append(keys[i]).append(":").append(value).newline();
    } while (keys[++i]);
    ctx->logString(statsStr.str());
    free(stats);
    free(keys);
}

void MemCachedPlugin::MCached::init(ICodeContext * ctx)
{
    logServerStats(ctx);
}

void MemCachedPlugin::MCached::setSettings()
{
    assertPool();
    assertOnError(memcached_pool_behavior_set(pool, MEMCACHED_BEHAVIOR_USE_UDP, 0));
    assertOnError(memcached_pool_behavior_set(pool, MEMCACHED_BEHAVIOR_SERVER_FAILURE_LIMIT, 1));
    assertOnError(memcached_pool_behavior_set(pool, MEMCACHED_BEHAVIOR_REMOVE_FAILED_SERVERS, 1));
    assertOnError(memcached_pool_behavior_set(pool, MEMCACHED_BEHAVIOR_NO_BLOCK, 0));
    assertOnError(memcached_pool_behavior_set(pool, MEMCACHED_BEHAVIOR_CONNECT_TIMEOUT, 100));//units of ms MORE: What should I set this to or get from?
    assertOnError(memcached_pool_behavior_set(pool, MEMCACHED_BEHAVIOR_BUFFER_REQUESTS, 0));// Buffering does not work with the ecl runtime paradigm
}

void MemCachedPlugin::MCached::invokeSecurity(ICodeContext * ctx)
{
    assertPool();
    //have to disconnect to toggle MEMCACHED_BEHAVIOR_BINARY_PROTOCOL
    //this routine is called in the constructor BEFORE the connection is fetched from the pool, the following check is for when/if this is called from elsewhere.
    bool disconnected = false;
    if (connection)//disconnect
    {
        memcached_pool_release(pool, connection);
        disconnected = true;
    }

    assertOnError(memcached_pool_behavior_set(pool, MEMCACHED_BEHAVIOR_BINARY_PROTOCOL, 1));

    if (disconnected)//reconnect
        connect(ctx);

    //Turn off verbosity for added security, in case someone's watching. Doesn't work with forced binary server though.
    //Requires a connection.
    bool tempConnect = false;
    if (!connection)//connect
    {
        connect(ctx);
        tempConnect = true;
    }
    reportErrorOnFail(ctx, memcached_verbosity(connection, (uint32_t)(0)));

/*
    Couldn't get either to work
    const char * user = "user1";
    const char * pswd = "123456789";
    assertOnError(memcached_set_sasl_auth_data(connection, user, pswd));
    assertOnError(memcached_set_encoding_key(connection, pswd, strlen(pswd)));
*/

    if (tempConnect)//disconnect
        memcached_pool_release(pool, connection);
}

void MemCachedPlugin::MCached::connect(ICodeContext * ctx)
{
    memcached_return_t error;
    connection = memcached_pool_fetch(pool, (struct timespec *)0 , &error);  //is used to fetch a connection structure from the connection pool.
                                                                                //The relative_time argument specifies if the function should block and
                                                                                //wait for a connection structure to be available if we try to exceed the
                                                                                //maximum size. You need to specify time in relative time.

    if (!alreadyInitialized)//Do this now rather than after assert. Better to have something even if it could be jiberish.
    {
        init(ctx);//doesn't necessarily initialize anything, instead outputs specs etc for debugging
        alreadyInitialized = true;
    }
    assertOnError(error);
}

//--------------------------------------------------------------------------------
//                           ECL SERVICE ENTRYPOINTS
//--------------------------------------------------------------------------------
ECL_MEMCACHED_API bool ECL_MEMCACHED_CALL MClear(ICodeContext * ctx, const char * servers)
{
    OwnedMCached connection = MemCachedPlugin::createConnection(ctx, servers);
    bool returnValue = connection->clear(ctx, 0);
    return returnValue;
}
ECL_MEMCACHED_API bool ECL_MEMCACHED_CALL MExist(ICodeContext * ctx, const char * servers, const char * key, const char * partitionKey)
{
    OwnedMCached connection = MemCachedPlugin::createConnection(ctx, servers);
    bool returnValue = connection->exist(ctx, key, partitionKey);
    return returnValue;
}
ECL_MEMCACHED_API const char * ECL_MEMCACHED_CALL MKeyType(ICodeContext * ctx, const char * servers, const char * key, const char * partitionKey)
{
    OwnedMCached connection = MemCachedPlugin::createConnection(ctx, servers);
    const char * keyType = enumToStr(connection->getKeyType(key, partitionKey));
    return keyType;
}
//-----------------------------------SET------------------------------------------
//NOTE: These were all overloaded by 'value' type, however; this caused problems since ecl implicitly casts and doesn't type check.
ECL_MEMCACHED_API bool ECL_MEMCACHED_CALL MSet(ICodeContext * ctx, const char * servers, const char * key, size32_t valueLength, const char * value, const char * partitionKey, unsigned expire /* = 0 (ECL default)*/)
{
    return MemCachedPlugin::MSet(ctx, servers, partitionKey, key, valueLength, value, expire, MemCachedPlugin::ECL_STRING);
}
ECL_MEMCACHED_API bool ECL_MEMCACHED_CALL MSet(ICodeContext * ctx, const char * servers, const char * key, size32_t valueLength, const UChar * value, const char * partitionKey, unsigned expire /* = 0 (ECL default)*/)
{
    return MemCachedPlugin::MSet(ctx, servers, partitionKey, key, (valueLength)*sizeof(UChar), value, expire, MemCachedPlugin::ECL_UNICODE);
}
ECL_MEMCACHED_API bool ECL_MEMCACHED_CALL MSet(ICodeContext * ctx, const char * servers, const char * key, signed __int64 value, const char * partitionKey, unsigned expire /* = 0 (ECL default)*/)
{
    return MemCachedPlugin::MSet(ctx, servers, partitionKey, key, value, expire, MemCachedPlugin::ECL_INTEGER);
}
ECL_MEMCACHED_API bool ECL_MEMCACHED_CALL MSet(ICodeContext * ctx, const char * servers, const char * key, unsigned __int64 value, const char * partitionKey, unsigned expire /* = 0 (ECL default)*/)
{
    return MemCachedPlugin::MSet(ctx, servers, partitionKey, key, value, expire, MemCachedPlugin::ECL_UNSIGNED);
}
ECL_MEMCACHED_API bool ECL_MEMCACHED_CALL MSet(ICodeContext * ctx, const char * servers, const char * key, double value, const char * partitionKey, unsigned expire /* = 0 (ECL default)*/)
{
    return MemCachedPlugin::MSet(ctx, servers, partitionKey, key, value, expire, MemCachedPlugin::ECL_REAL);
}
ECL_MEMCACHED_API bool ECL_MEMCACHED_CALL MSet(ICodeContext * ctx, const char * servers, const char * key, bool value, const char * partitionKey, unsigned expire)
{
    return MemCachedPlugin::MSet(ctx, servers, partitionKey, key, value, expire, MemCachedPlugin::ECL_BOOLEAN);
}
ECL_MEMCACHED_API bool ECL_MEMCACHED_CALL MSetData(ICodeContext * ctx, const char * servers, const char * key, size32_t valueLength, const void * value, const char * partitionKey, unsigned expire)
{
    return MemCachedPlugin::MSet(ctx, servers, partitionKey, key, valueLength, value, expire, MemCachedPlugin::ECL_DATA);
}
ECL_MEMCACHED_API bool ECL_MEMCACHED_CALL MSetUtf8(ICodeContext * ctx, const char * servers, const char * key, size32_t valueLength, const char * value, const char * partitionKey, unsigned expire /* = 0 (ECL default)*/)
{
    return MemCachedPlugin::MSet(ctx, servers, partitionKey, key, rtlUtf8Size(valueLength, value), value, expire, MemCachedPlugin::ECL_UTF8);
}
//-------------------------------------GET----------------------------------------
ECL_MEMCACHED_API bool ECL_MEMCACHED_CALL MGetBool(ICodeContext * ctx, const char * servers, const char * key, const char * partitionKey)
{
    bool value;
    MemCachedPlugin::MGet(ctx, servers, partitionKey, key, value, MemCachedPlugin::ECL_BOOLEAN);
    return value;
}
ECL_MEMCACHED_API double ECL_MEMCACHED_CALL MGetDouble(ICodeContext * ctx, const char * servers, const char * key, const char * partitionKey)
{
    double value;
    MemCachedPlugin::MGet(ctx, servers, partitionKey, key, value, MemCachedPlugin::ECL_REAL);
    return value;
}
ECL_MEMCACHED_API signed __int64 ECL_MEMCACHED_CALL MGetInt8(ICodeContext * ctx, const char * servers, const char * key, const char * partitionKey)
{
    signed __int64 value;
    MemCachedPlugin::MGet(ctx, servers, partitionKey, key, value, MemCachedPlugin::ECL_INTEGER);
    return value;
}
ECL_MEMCACHED_API unsigned __int64 ECL_MEMCACHED_CALL MGetUint8(ICodeContext * ctx, const char * servers, const char * key, const char * partitionKey)
{
    unsigned __int64 value;
    MemCachedPlugin::MGet(ctx, servers, partitionKey, key, value, MemCachedPlugin::ECL_UNSIGNED);
    return value;
}
ECL_MEMCACHED_API void ECL_MEMCACHED_CALL MGetStr(ICodeContext * ctx, size32_t & returnLength, char * & returnValue, const char * servers, const char * key, const char * partitionKey)
{
    size_t _returnLength;
    MemCachedPlugin::MGet(ctx, servers, partitionKey, key, _returnLength, returnValue, MemCachedPlugin::ECL_STRING);
    returnLength = static_cast<size32_t>(_returnLength);
}
ECL_MEMCACHED_API void ECL_MEMCACHED_CALL MGetUChar(ICodeContext * ctx, size32_t & returnLength, UChar * & returnValue,  const char * servers, const char * key, const char * partitionKey)
{
    size_t _returnSize;
    MemCachedPlugin::MGet(ctx, servers, partitionKey, key, _returnSize, returnValue, MemCachedPlugin::ECL_UNICODE);
    returnLength = static_cast<size32_t>(_returnSize/sizeof(UChar));
}
ECL_MEMCACHED_API void ECL_MEMCACHED_CALL MGetUtf8(ICodeContext * ctx, size32_t & returnLength, char * & returnValue, const char * servers, const char * key, const char * partitionKey)
{
    size_t returnSize;
    MemCachedPlugin::MGet(ctx, servers, partitionKey, key, returnSize, returnValue, MemCachedPlugin::ECL_UTF8);
    returnLength = static_cast<size32_t>(rtlUtf8Length(returnSize, returnValue));
}
ECL_MEMCACHED_API void ECL_MEMCACHED_CALL MGetData(ICodeContext * ctx, size32_t & returnLength, void * & returnValue, const char * servers, const char * key, const char * partitionKey)
{
    size_t _returnLength;
    MemCachedPlugin::MGetVoidPtrLenPair(ctx, servers, partitionKey, key, _returnLength, returnValue, MemCachedPlugin::ECL_DATA);
    returnLength = static_cast<size32_t>(_returnLength);
}
