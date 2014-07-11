/*##############################################################################

    HPCC SYSTEMS software Copyright (C) 2012 HPCC Systems.

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
#include <libmemcached-1.0/memcached.h>
#include <libmemcachedutil-1.0/pool.h>

#define MEMCACHED_VERSION "memcached plugin 1.0.0"

static const char * EclDefinition =
"export MemCached := SERVICE\n"
"  BOOLEAN MSetVarunicode(CONST VARSTRING servers, CONST VARSTRING key, VARUNICODE value, CONST VARSTRING partitionKey = 'root', UNSIGNED4 expire = 0) : cpp,action,context,entrypoint='MSet'; \n"
"  BOOLEAN MSetUnicode(CONST VARSTRING servers, CONST VARSTRING key, UNICODE value, CONST VARSTRING partitionKey = 'root', UNSIGNED4 expire = 0) : cpp,action,context,entrypoint='MSet'; \n"
"  BOOLEAN MSetVarstring(CONST VARSTRING servers, CONST VARSTRING key, VARSTRING value, CONST VARSTRING partitionKey = 'root', UNSIGNED4 expire = 0) : cpp,action,context,entrypoint='MSet'; \n"
"  BOOLEAN MSetString(CONST VARSTRING servers, CONST VARSTRING key, STRING value, CONST VARSTRING partitionKey = 'root', UNSIGNED4 expire = 0) : cpp,action,context,entrypoint='MSet'; \n"
"  BOOLEAN MSetBoolean(CONST VARSTRING servers, CONST VARSTRING key, BOOLEAN value, CONST VARSTRING partitionKey = 'root', UNSIGNED4 expire = 0) : cpp,action,context,entrypoint='MSet'; \n"
"  BOOLEAN MSetReal4(CONST VARSTRING servers, CONST VARSTRING key, REAL4 value, CONST VARSTRING partitionKey = 'root', UNSIGNED4 expire = 0) : cpp,action,context,entrypoint='MSet'; \n"
"  BOOLEAN MSetReal8(CONST VARSTRING servers, CONST VARSTRING key, REAL8 value, CONST VARSTRING partitionKey = 'root', UNSIGNED4 expire = 0) : cpp,action,context,entrypoint='MSet'; \n"
"  BOOLEAN MSetReal(CONST VARSTRING servers, CONST VARSTRING key, REAL value, CONST VARSTRING partitionKey = 'root', UNSIGNED4 expire = 0) : cpp,action,context,entrypoint='MSet'; \n"
"  BOOLEAN MSetInteger1(CONST VARSTRING servers, CONST VARSTRING key, INTEGER1 value, CONST VARSTRING partitionKey = 'root', UNSIGNED4 expire = 0) : cpp,action,context,entrypoint='MSet'; \n"
"  BOOLEAN MSetInteger2(CONST VARSTRING servers, CONST VARSTRING key, INTEGER2 value, CONST VARSTRING partitionKey = 'root', UNSIGNED4 expire = 0) : cpp,action,context,entrypoint='MSet'; \n"
"  BOOLEAN MSetInteger4(CONST VARSTRING servers, CONST VARSTRING key, INTEGER4 value, CONST VARSTRING partitionKey = 'root', UNSIGNED4 expire = 0) : cpp,action,context,entrypoint='MSet'; \n"
"  BOOLEAN MSetInteger8(CONST VARSTRING servers, CONST VARSTRING key, INTEGER8 value, CONST VARSTRING partitionKey = 'root', UNSIGNED4 expire = 0) : cpp,action,context,entrypoint='MSet'; \n"
"  BOOLEAN MSetInteger(CONST VARSTRING servers, CONST VARSTRING key, INTEGER value, CONST VARSTRING partitionKey = 'root', UNSIGNED4 expire = 0) : cpp,action,context,entrypoint='MSet'; \n"
"  BOOLEAN MSetUnsigned1(CONST VARSTRING servers, CONST VARSTRING key, UNSIGNED1 value, CONST VARSTRING partitionKey = 'root', UNSIGNED4 expire = 0) : cpp,action,context,entrypoint='MSet'; \n"
"  BOOLEAN MSetUnsigned2(CONST VARSTRING servers, CONST VARSTRING key, UNSIGNED2 value, CONST VARSTRING partitionKey = 'root', UNSIGNED4 expire = 0) : cpp,action,context,entrypoint='MSet'; \n"
"  BOOLEAN MSetUnsigned4(CONST VARSTRING servers, CONST VARSTRING key, UNSIGNED4 value, CONST VARSTRING partitionKey = 'root', UNSIGNED4 expire = 0) : cpp,action,context,entrypoint='MSet'; \n"
"  BOOLEAN MSetUnsigned8(CONST VARSTRING servers, CONST VARSTRING key, UNSIGNED8 value, CONST VARSTRING partitionKey = 'root', UNSIGNED4 expire = 0) : cpp,action,context,entrypoint='MSet'; \n"
"  BOOLEAN MSetUnsigned(CONST VARSTRING servers, CONST VARSTRING key, UNSIGNED value,CONST VARSTRING partitionKey = 'root',  UNSIGNED4 expire = 0) : cpp,action,context,entrypoint='MSet'; \n"
"  BOOLEAN MSetData(CONST VARSTRING servers, CONST VARSTRING key, DATA value, CONST VARSTRING partitionKey = 'root', UNSIGNED4 expire = 0) : cpp,action,context,entrypoint='MSetVoidPtr'; \n"
"  BOOLEAN MSetDataset(CONST VARSTRING servers, CONST VARSTRING key, DATASET value, CONST VARSTRING partitionKey = 'root', UNSIGNED4 expire = 0) : cpp,action,context,entrypoint='MSetVoidPtr'; \n"

"  INTEGER1 MGetInteger1(CONST VARSTRING servers, CONST VARSTRING key, CONST VARSTRING partitionKey = 'root') : cpp,action,context,entrypoint='MGetInt1'; \n"
"  INTEGER2 MGetInteger2(CONST VARSTRING servers, CONST VARSTRING key, CONST VARSTRING partitionKey = 'root') : cpp,action,context,entrypoint='MGetInt2'; \n"
"  INTEGER4 MGetInteger4(CONST VARSTRING servers, CONST VARSTRING key, CONST VARSTRING partitionKey = 'root') : cpp,action,context,entrypoint='MGetInt4'; \n"
"  INTEGER8 MGetInteger8(CONST VARSTRING servers, CONST VARSTRING key, CONST VARSTRING partitionKey = 'root') : cpp,action,context,entrypoint='MGetInt8'; \n"
"  INTEGER8 MGetInteger(CONST VARSTRING servers, CONST VARSTRING key, CONST VARSTRING partitionKey = 'root') : cpp,action,context,entrypoint='MGetInt8'; \n"
"  UNSIGNED1 MGetUnsigned1(CONST VARSTRING servers, CONST VARSTRING key, CONST VARSTRING partitionKey = 'root') : cpp,action,context,entrypoint='MGetUint1'; \n"
"  UNSIGNED2 MGetUnsigned2(CONST VARSTRING servers, CONST VARSTRING key, CONST VARSTRING partitionKey = 'root') : cpp,action,context,entrypoint='MGetUint2'; \n"
"  UNSIGNED4 MGetUnsigned4(CONST VARSTRING servers, CONST VARSTRING key, CONST VARSTRING partitionKey = 'root') : cpp,action,context,entrypoint='MGetUint4'; \n"
"  UNSIGNED8 MGetUnsigned8(CONST VARSTRING servers, CONST VARSTRING key, CONST VARSTRING partitionKey = 'root') : cpp,action,context,entrypoint='MGetUint8'; \n"
"  UNSIGNED8 MGetUnsigned(CONST VARSTRING servers, CONST VARSTRING key, CONST VARSTRING partitionKey = 'root') : cpp,action,context,entrypoint='MGetUint8'; \n"
"  STRING MGetString(CONST VARSTRING servers, CONST VARSTRING key, CONST VARSTRING partitionKey = 'root') : cpp,action,context,entrypoint='MGetStr'; \n"
"  VARSTRING MGetVarstring(CONST VARSTRING servers, CONST VARSTRING key, CONST VARSTRING partitionKey = 'root') : cpp,action,context,entrypoint='MGetVarStr'; \n"
"  UNICODE MGetUnicode(CONST VARSTRING servers, CONST VARSTRING key, CONST VARSTRING partitionKey = 'root') : cpp,action,context,entrypoint='MGetUChar'; \n"
"  VARUNICODE MGetVarunicode(CONST VARSTRING servers, CONST VARSTRING key, CONST VARSTRING partitionKey = 'root') : cpp,action,context,entrypoint='MGetVarUChar'; \n"
"  BOOLEAN MGetBoolean(CONST VARSTRING servers, CONST VARSTRING key, CONST VARSTRING partitionKey = 'root') : cpp,action,context,entrypoint='MGetBool'; \n"
"  REAL MGetReal(CONST VARSTRING servers, CONST VARSTRING key, CONST VARSTRING partitionKey = 'root') : cpp,action,context,entrypoint='MGetDouble'; \n"
"  REAL4 MGetReal4(CONST VARSTRING servers, CONST VARSTRING key, CONST VARSTRING partitionKey = 'root') : cpp,action,context,entrypoint='MGetFloat'; \n"
"  REAL8 MGetReal8(CONST VARSTRING servers, CONST VARSTRING key, CONST VARSTRING partitionKey = 'root') : cpp,action,context,entrypoint='MGetDouble'; \n"
"  DATA MGetData(CONST VARSTRING servers, CONST VARSTRING key, CONST VARSTRING partitionKey = 'root') : cpp,action,context,entrypoint='MGetData'; \n"
"  DATASET MGetDataset(CONST VARSTRING servers, CONST VARSTRING key, CONST VARSTRING partitionKey = 'root') : cpp,action,context,entrypoint='MGetData'; \n"

"  BOOLEAN MExist(CONST VARSTRING servers, CONST VARSTRING key, CONST VARSTRING partitionKey = 'root') : cpp,action,context,entrypoint='MExist'; \n"
"  CONST VARSTRING MKeyType(CONST VARSTRING servers, CONST VARSTRING key, CONST VARSTRING partitionKey = 'root') : cpp,action,context,entrypoint='MKeyType'; \n"//NOTE: calls get
"  BOOLEAN MClear(CONST VARSTRING servers) : cpp,action,context,entrypoint='MClear'; \n"
"END;";

ECL_MEMCACHED_API bool getECLPluginDefinition(ECLPluginDefinitionBlock *pb)
{
    if (pb->size != sizeof(ECLPluginDefinitionBlock))
        return false;

    pb->magicVersion = PLUGIN_VERSION;
    pb->version = MEMCACHED_VERSION;
    pb->moduleName = "lib_eclmemcached";
    pb->ECL = EclDefinition;
    pb->flags = PLUGIN_IMPLICIT_MODULE;
    pb->description = "ECL plugin library for the C++ API libmemcached (http://libmemcached.org/)\n";
    return true;
}

namespace MemCachedPlugin {
    IPluginContext * parentCtx = NULL;
    unsigned unitExpire = 86400;//1 day (secs)

    enum eclDataType {
        ECL_BOOLEAN,
        ECL_DATA,
        ECL_INTEGER1,
        ECL_INTEGER2,
        ECL_INTEGER4,
        ECL_INTEGER8,
        ECL_REAL4,
        ECL_REAL8,
        ECL_STRING,
        ECL_UNICODE,
        ECL_UNSIGNED1,
        ECL_UNSIGNED2,
        ECL_UNSIGNED4,
        ECL_UNSIGNED8,
        ECL_VARSTRING,
        ECL_VARUNICODE,
        ECL_NONE
    };

    const char * enumToStr(eclDataType type)
    {
        switch(type)
        {
        case ECL_BOOLEAN:
            return "BOOLEAN";
        case ECL_INTEGER1:
            return "INTEGER1";
        case ECL_INTEGER2:
            return "INTEGER2";
        case ECL_INTEGER4:
            return "INTEGER4";
        case ECL_INTEGER8:
            return "INTEGER8";
        case ECL_UNSIGNED1:
            return "UNSIGNED1";
        case ECL_UNSIGNED2:
            return "UNSIGNED1";
        case ECL_UNSIGNED4:
            return "UNSIGNED1";
        case ECL_UNSIGNED8:
            return "UNSIGNED1";
        case ECL_REAL4:
            return "REAL4";
        case ECL_REAL8:
               return "REAL8";
        case ECL_STRING:
            return "STRING";
        case ECL_UNICODE:
            return "UNICODE";
        case ECL_VARSTRING:
            return "VARSTRING";
        case ECL_VARUNICODE:
            return "VARUNICODE";
        case ECL_DATA:
            return "DATA";
        case ECL_NONE:
            return "Nonexistent";
        default:
            return "UNKNOWN";
        }
    }

    class MCached
    {
    public :
        MCached();
        MCached(const char * servers, ICodeContext * _tcx);
        ~MCached();

        //set
        template <class type> bool set(const char * partitionKey, const char * key, size32_t valueLength, type value, unsigned expire, eclDataType eclType);
        template <class type> bool set(const char * partitionKey, const char * key, size32_t valueLength, type * value, unsigned expire, eclDataType eclType);
        bool setVoidPtr(const char * partitionKey, const char * key, size32_t valueLength, void * value, unsigned expire, eclDataType eclType);
        //get
        template <class type> void get(const char * partitionKey, const char * key, size_t & valueLength, type * & value, eclDataType eclType);
        void getVoidPtrLenPair(const char * partitionKey, const char * key, size32_t & valueLength, void * & value, eclDataType eclType);

        bool clear(unsigned when);
        bool exist(const char * key, const char * partitionKey);
        eclDataType getKeyType(const char * key, const char * partitionKey);

    private :
        void assertServerVersionHomogeneity();
        void assertAtLeastSingleServerUp();
        void assertAllServersUp();
        inline void assertOnError(memcached_return_t error);
        inline bool logErrorOnFail(memcached_return_t error);
        void assertKeyTypeMismatch(const char * key, uint32_t flag, eclDataType type);
        void logKeyTypeMismatch(const char * key, uint32_t flag, eclDataType eclType);
        void * cpy(char * src, size_t length, eclDataType t);
        void logServerStats();
        void init();
        void invokeSecurity();
        void setSettings();
        inline void assertPool();//For internal purposes to insure correct order of the above processes and instantiation.

    private :
        ICodeContext * ctx;
        memcached_st * connection;
        memcached_pool_st * pool;
        static bool alreadyInitialized;
    };

    //-------------------------------------------SET-----------------------------------------
    template<class type> bool MSet(ICodeContext * _ctx, const char * servers, const char * partitionKey, const char * key, size32_t valueLength, type value, unsigned expire, eclDataType eclType)
    {
        MemCachedPlugin::MCached * connection = new MemCachedPlugin::MCached(servers, _ctx);
        bool success = connection->set(partitionKey, key, valueLength, value, expire, eclType);
        delete connection;
        return success;
    }
    //Set pointer types
    template<class type> bool MSet(ICodeContext * _ctx, const char * servers, const char * partitionKey, const char * key, size32_t valueLength, type * value, unsigned expire, eclDataType eclType)
    {
        MemCachedPlugin::MCached * connection = new MemCachedPlugin::MCached(servers, _ctx);
        bool success = connection->set(partitionKey, key, valueLength, value, expire, eclType);
        delete connection;
        return success;
    }
    //Set void pointer
    bool MSetVoidPtr(ICodeContext * _ctx, const char * servers, const char * partitionKey, const char * key, size32_t valueLength, void * value, unsigned expire, eclDataType eclType)
    {
        MemCachedPlugin::MCached * connection = new MemCachedPlugin::MCached(servers, _ctx);
        bool success = connection->setVoidPtr(partitionKey, key, valueLength, value, expire, eclType);
        delete connection;
        return success;
    }
    //-------------------------------------------GET-----------------------------------------
    template<class type> void MGet(ICodeContext * _ctx, const char * servers, const char * partitionKey, const char * key, size_t & returnLength, type * & returnValue, eclDataType eclType)
    {
        MemCachedPlugin::MCached * connection = new MemCachedPlugin::MCached(servers, _ctx);
        connection->get(partitionKey, key, returnLength, returnValue, eclType);
        delete connection;
    }
    //---------------------------------------------------------------------------------------
    void MGetVoidPtrLenPair(ICodeContext * _ctx, const char * servers, const char * partitionKey, const char * key, size32_t & returnLength, void * & returnValue, eclDataType eclType)
    {
        MemCachedPlugin::MCached * connection = new MemCachedPlugin::MCached(servers, _ctx);
        connection->getVoidPtrLenPair(partitionKey, key, returnLength, returnValue, eclType);
        delete connection;
    }
}//namespace
//----------------------------------SET----------------------------------------
template<class type> bool MemCachedPlugin::MCached::set(const char * partitionKey, const char * key, size32_t valueLength, type value, unsigned expire, eclDataType eclType)
{
    char * _value = reinterpret_cast<char *>(&value);//Do this even for char * to prevent compiler complaining
    return !logErrorOnFail(memcached_set_by_key(connection, partitionKey, strlen(partitionKey), key, strlen(key), _value, (size_t)(valueLength), (time_t)(expire*unitExpire), (uint32_t)eclType));
}
template<class type> bool MemCachedPlugin::MCached::set(const char * partitionKey, const char * key, size32_t valueLength, type * value, unsigned expire, eclDataType eclType)
{
    char * _value = reinterpret_cast<char *>(value);//Do this even for char * to prevent compiler complaining
    return !logErrorOnFail(memcached_set_by_key(connection, partitionKey, strlen(partitionKey), key, strlen(key), _value, (size_t)(valueLength), (time_t)(expire*unitExpire), (uint32_t)eclType));
}
bool MemCachedPlugin::MCached::setVoidPtr(const char * partitionKey, const char * key, size32_t valueLength, void * value, unsigned expire, eclDataType eclType)
{
    char * _value = reinterpret_cast<char *>(value);//Do this even for char * to prevent compiler complaining
    return !logErrorOnFail(memcached_set_by_key(connection, partitionKey, strlen(partitionKey), key, strlen(key), _value, (size_t)(valueLength), (time_t)(expire*unitExpire), (uint32_t)eclType));
}
//----------------------------------GET----------------------------------------
template<class type> void MemCachedPlugin::MCached::get(const char * partitionKey, const char * key, size_t & returnLength, type * & returnValue, eclDataType eclType)
{
    uint32_t flag;
    memcached_return_t error;

    char * _value = memcached_get_by_key(connection, partitionKey, strlen(partitionKey), key, strlen(key), &returnLength, &flag, &error);
    assertOnError(error);
    logKeyTypeMismatch(key, flag, eclType);

    returnValue = reinterpret_cast<type*>(cpy(_value, returnLength, eclType));
    delete _value;
}
void MemCachedPlugin::MCached::getVoidPtrLenPair(const char * partitionKey, const char * key, size32_t & returnLength, void * & returnValue, eclDataType eclType)
{
    size_t returnValueLength;
    uint32_t flag;
    memcached_return_t error;

    char * _value = memcached_get_by_key(connection, partitionKey, strlen(partitionKey), key, strlen(key), &returnValueLength, &flag, &error);
    assertOnError(error);
    logKeyTypeMismatch(key, flag, eclType);

    returnLength = (size32_t)(returnValueLength);
    returnValue = reinterpret_cast<void*>(cpy(_value, returnLength, eclType));
    delete _value;
}

bool MemCachedPlugin::MCached::alreadyInitialized = false;

ECL_MEMCACHED_API void setPluginContext(IPluginContext * _ctx) { MemCachedPlugin::parentCtx = _ctx; }

MemCachedPlugin::MCached::MCached() { throwUnexpected(); }

MemCachedPlugin::MCached::MCached(const char * servers, ICodeContext * _ctx) : ctx(_ctx)
{
    connection = NULL;
    pool = NULL;

    pool = memcached_pool(servers, strlen(servers));
    assertPool();
    invokeSecurity();

    memcached_return_t error;
    connection = memcached_pool_fetch(pool, (struct timespec *)0 , &error); //is used to fetch a connection structure from the connection pool.
                                                                               //The relative_time argument specifies if the function should block and
                                                                               //wait for a connection structure to be available if we try to exceed the
                                                                               //maximum size. You need to specify time in relative time.

    if(!alreadyInitialized)//Do this now rather than after assert. Better to have something even if it could be jiberish.
    {
        init();//doesn't necessarily initialize anything, instead outputs specs etc for debugging
        alreadyInitialized = true;
    }
    assertOnError(error);
    //assertAtLeastSingleServerUp();
    assertAllServersUp();//MORE: Would we want to limp on if a server drops? How to differentiate a server dropping and incorrectly
                         //requesting wrong IP, or just forgetting to start all daemons?

#ifndef _DEBUG // default?
    assertServerVersionHomogeneity(); //Not sure if we want this, or whether it should live in assertServersUp()?
#endif

    setSettings();
}
//-----------------------------------------------------------------------------
inline MemCachedPlugin::MCached::~MCached()
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
        //memcached_quit(connection);//NOTE: Implicit in memcached_free
        memcached_free(connection);
    }
}

inline void MemCachedPlugin::MCached::assertPool()
{
    if(!pool)
        rtlFail(0, "MemCachedPlugin:INTERNAL ERROR - Failed to instantiate server pool.");
}

void * MemCachedPlugin::MCached::cpy(char * src, size_t length, eclDataType eclType)
{
    void * value = rtlMalloc(length);
    return memcpy(value, src, length);
}

void MemCachedPlugin::MCached::assertAllServersUp()
{
    memcached_return_t error;
    char * args = NULL;

    memcached_stat_st * stats = memcached_stat(connection, args, &error);
    //assertOnError(error);//Asserting this here will cause a fail if a server is not up and with only a SOME ERRORS WERE REPORTED message.

    unsigned int numberOfServers = memcached_server_count(connection);
    for (unsigned i = 0; i < numberOfServers; ++i)
    {
        if (stats[i].pid == -1)//not sure if this is the  best test?
        {
            delete[] stats;
#ifdef _DEBUG
            rtlFail(0, "MemCachedPlugin: Failed to connect to ALL server(s). Check memcached daemons on all requested servers?");
#else
            rtlFail(0, "MemCachedPlugin: Failed to connect to ALL server(s). Check memcached daemons on all requested servers and that \"memcached -B ascii\" was not used.");
#endif
        }
    }
    delete[] stats;
}
//-----------------------------------------------------------------------------
void MemCachedPlugin::MCached::assertAtLeastSingleServerUp()
{
    memcached_return_t error;
    char * args = NULL;

    if (!connection)
        rtlFail(0, "MemCachedPlugin:INTERNAL ERROR - connection referenced without being instantiated.");

    memcached_stat_st * stats = memcached_stat(connection, args, &error);
    //assertOnError(error);//Asserting this here will cause a fail if a server is not up and with only a SOME ERRORS WERE REPORTED message.

    unsigned int numberOfServers = memcached_server_count(connection);
    for (unsigned i = 0; i < numberOfServers; ++i)
    {
        if (stats[i].pid > -1)//not sure if this is the  best test?
        {
            delete[] stats;
            return;
        }
    }
    delete[] stats;

#ifdef _DEBUG
    rtlFail(0, "MemCachedPlugin: Failed to connect to at least a single server. Check memcached daemons on all requested servers?");
#else
    rtlFail(0, "MemCachedPlugin: Failed to connect to at least a single servers. Check memcached daemons on all requested servers and that \"memcached -B ascii\" was not used.");
#endif
}

bool MemCachedPlugin::MCached::logErrorOnFail(memcached_return_t error)
{
    if (error == MEMCACHED_SUCCESS)
        return false;

    ctx->addWuException(memcached_strerror(connection, error), 0, 0, "");
    return true;
}

void MemCachedPlugin::MCached::assertOnError(memcached_return_t error)
{
    if (error != MEMCACHED_SUCCESS)
        rtlFail(0, memcached_strerror(connection, error));
}

bool MemCachedPlugin::MCached::clear(unsigned when)
{
    //NOTE: This is the actual cache flush and not a buffer flush.
    return !logErrorOnFail(memcached_flush(connection, (time_t)(when)));
}

bool MemCachedPlugin::MCached::exist(const char * key, const char * partitionKey)
{
    return !logErrorOnFail(memcached_exist_by_key(connection, partitionKey, strlen(partitionKey), key, strlen(key)));
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
        rtlFail(0, memcached_strerror(connection, error));
}

void MemCachedPlugin::MCached::logKeyTypeMismatch(const char * key, uint32_t flag, eclDataType eclType)
{
    if (flag !=  eclType)
    {
        StringBuffer msg = "MemCachedPlugin: The key (";
        msg.append(key).append(") to fetch is of type ").append(enumToStr((eclDataType)(flag))).append(", not ").append(enumToStr(eclType)).append(" as requested.\n");
        ctx->addWuException(msg.str(), 0, 0, "");
    }
}

void MemCachedPlugin::MCached::assertKeyTypeMismatch(const char * key, uint32_t flag, eclDataType eclType)
{
    if (flag !=  eclType)
    {
        StringBuffer msg = "MemCachedPlugin: The key (";
        msg.append(key).append(") to fetch is of type ").append(enumToStr((eclDataType)(flag))).append(", not ").append(enumToStr(eclType)).append(" as requested.\n");
        rtlFail(0, msg.str());
    }
}

void MemCachedPlugin::MCached::logServerStats()
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
    delete[] stats;
    delete[] keys;
}

void MemCachedPlugin::MCached::init()
{
    logServerStats();
    //MORE: log more.
}

void MemCachedPlugin::MCached::setSettings()
{
    assertPool();
    assertOnError(memcached_pool_behavior_set(pool, MEMCACHED_BEHAVIOR_USE_UDP, 0));
    assertOnError(memcached_pool_behavior_set(pool, MEMCACHED_BEHAVIOR_REMOVE_FAILED_SERVERS, 1));//conflicts with assertAllServersUp()
    assertOnError(memcached_pool_behavior_set(pool, MEMCACHED_BEHAVIOR_NO_BLOCK, 0));
    assertOnError(memcached_pool_behavior_set(pool, MEMCACHED_BEHAVIOR_CONNECT_TIMEOUT, 100));//units of ms MORE: What should I set this to or get from?
    assertOnError(memcached_pool_behavior_set(pool, MEMCACHED_BEHAVIOR_BUFFER_REQUESTS, 0));
}

void MemCachedPlugin::MCached::invokeSecurity() //Might be better to move all this to constructor such that temp disconnects and connects aren't needed.
{
    assertPool();
    //have to disconnect to toggle MEMCACHED_BEHAVIOR_BINARY_PROTOCOL
    //this routine is called in the constructor BEFORE the connection is fetched from the pool, the following check is for when this is called from elsewhere.
    bool disconnected = false;
    if (connection)//disconnect
    {
        memcached_pool_release(pool, connection);
        disconnected = true;
    }

    assertOnError(memcached_pool_behavior_set(pool, MEMCACHED_BEHAVIOR_BINARY_PROTOCOL, 1));

    if (disconnected)//reconnect
    {
        memcached_return_t error;
        connection = memcached_pool_fetch(pool, (struct timespec *)0 , &error);
        assertOnError(error);
    }

    //Turn off verbosity for added security, in case someone's watching. Doesn't work with forced binary server though.
    //Requires a connection.
    bool tempConnect = false;
    if (!connection)//connect
    {
        memcached_return_t error;
        connection = memcached_pool_fetch(pool, (struct timespec *)0 , &error);
        assertOnError(error);
        tempConnect = true;
    }

#ifdef _DEBUG
    logErrorOnFail(memcached_verbosity(connection, (uint32_t)(2)));
#else
    //assertOnError(memcached_verbosity(connection, (uint32_t)(0)));
    logErrorOnFail(memcached_verbosity(connection, (uint32_t)(0))); //Doesn't work with forced binary servers.
#endif                                                                //This will cause everything to hang until it times out with
                                                                      //memcached_return_t = SOME_ERRORS_REPORTED, so should perhaps assert?
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

void MemCachedPlugin::MCached::assertServerVersionHomogeneity()
{
    unsigned int numberOfServers = memcached_server_count(connection);
    if (numberOfServers < 2)
        return;

    memcached_return_t error;
    char * args = NULL;

    memcached_stat_st * stats = memcached_stat(connection, args, &error);
    //assertOnError(error);//Asserting this here will cause a fail if a server is not up and with only a SOME ERRORS WERE REPORTED message.

    for (unsigned i = 0; i < numberOfServers-1; ++i)
    {
        if (strcmp(stats[i].version, stats[i++].version) != 0)
        {
            delete[] stats;
            rtlFail(0, "MemCachedPlugin: Inhomogeneous versions of memcached across servers.");
        }
    }
    delete[] stats;
}
//--------------------------------------------------------------------------------
//                           ECL SERVICE ENTRYPOINTS
//--------------------------------------------------------------------------------
ECL_MEMCACHED_API bool ECL_MEMCACHED_CALL MClear(ICodeContext * _ctx, const char * servers)
{
    MemCachedPlugin::MCached * connection = new MemCachedPlugin::MCached(servers, _ctx);
    bool returnValue = connection->clear(0);
    delete connection;
    return returnValue;
}

ECL_MEMCACHED_API bool ECL_MEMCACHED_CALL MExist(ICodeContext * _ctx, const char * servers, const char * key, const char * partitionKey)
{
    MemCachedPlugin::MCached * connection = new MemCachedPlugin::MCached(servers, _ctx);
    bool returnValue = connection->exist(key, partitionKey);
    delete connection;
    return returnValue;
}

ECL_MEMCACHED_API const char * ECL_MEMCACHED_CALL MKeyType(ICodeContext * _ctx, const char * servers, const char * key, const char * partitionKey)
{
    MemCachedPlugin::MCached * connection = new MemCachedPlugin::MCached(servers, _ctx);
    const char * keyType = enumToStr(connection->getKeyType(key, partitionKey));
    delete connection;
    return keyType;
}
//-----------------------------------SET------------------------------------------
//NOTE: This were all overloaded by 'value' type, however; this caused problems since ecl implicitly casts and doesn't type check.
ECL_MEMCACHED_API bool ECL_MEMCACHED_CALL MSet(ICodeContext * _ctx, const char * servers, const char * key, size32_t valueLength, char * value, const char * partitionKey, unsigned expire /* = 0 (ECL default)*/)
{
    return MemCachedPlugin::MSet(_ctx, servers, partitionKey, key, valueLength, value, expire, MemCachedPlugin::ECL_STRING);
}
ECL_MEMCACHED_API bool ECL_MEMCACHED_CALL MSet(ICodeContext * _ctx, const char * servers, const char * key, size32_t valueLength, UChar * value, const char * partitionKey, unsigned expire /* = 0 (ECL default)*/)
{
    return MemCachedPlugin::MSet(_ctx, servers, partitionKey, key, valueLength*sizeof(*value), value, expire, MemCachedPlugin::ECL_UNICODE);
}
ECL_MEMCACHED_API bool ECL_MEMCACHED_CALL MSet(ICodeContext * _ctx, const char * servers, const char * key, char * value, const char * partitionKey, unsigned expire /* = 0 (ECL default)*/)
{
    return MemCachedPlugin::MSet(_ctx, servers, partitionKey, key, strlen(value)+1, value, expire, MemCachedPlugin::ECL_VARSTRING);
}
ECL_MEMCACHED_API bool ECL_MEMCACHED_CALL MSet(ICodeContext * _ctx, const char * servers, const char * key, UChar * value, const char * partitionKey, unsigned expire /* = 0 (ECL default)*/)
{
    return MemCachedPlugin::MSet(_ctx, servers, partitionKey, key, sizeof(*value), value, expire, MemCachedPlugin::ECL_VARUNICODE);//valueLength should be (length+1)*2
}
ECL_MEMCACHED_API bool ECL_MEMCACHED_CALL MSet(ICodeContext * _ctx, const char * servers, const char * key, signed char value, const char * partitionKey, unsigned expire /* = 0 (ECL default)*/)
{
    return MemCachedPlugin::MSet(_ctx, servers, partitionKey, key, sizeof(value), value, expire, MemCachedPlugin::ECL_INTEGER1);
}
ECL_MEMCACHED_API bool ECL_MEMCACHED_CALL MSet(ICodeContext * _ctx, const char * servers, const char * key, signed short value, const char * partitionKey, unsigned expire /* = 0 (ECL default)*/)
{
    return MemCachedPlugin::MSet(_ctx, servers, partitionKey, key, sizeof(value), value, expire, MemCachedPlugin::ECL_INTEGER2);
}
ECL_MEMCACHED_API bool ECL_MEMCACHED_CALL MSet(ICodeContext * _ctx, const char * servers, const char * key, signed int value, const char * partitionKey, unsigned expire /* = 0 (ECL default)*/)
{
    return MemCachedPlugin::MSet(_ctx, servers, partitionKey, key, sizeof(value), value, expire, MemCachedPlugin::ECL_INTEGER4);
}
ECL_MEMCACHED_API bool ECL_MEMCACHED_CALL MSet(ICodeContext * _ctx, const char * servers, const char * key, signed __int64 value, const char * partitionKey, unsigned expire /* = 0 (ECL default)*/)
{
    return MemCachedPlugin::MSet(_ctx, servers, partitionKey, key, sizeof(value), value, expire, MemCachedPlugin::ECL_INTEGER8);
}
ECL_MEMCACHED_API bool ECL_MEMCACHED_CALL MSet(ICodeContext * _ctx, const char * servers, const char * key, unsigned char value, const char * partitionKey, unsigned expire /* = 0 (ECL default)*/)
{
    return MemCachedPlugin::MSet(_ctx, servers, partitionKey, key, sizeof(value), value, expire, MemCachedPlugin::ECL_UNSIGNED1);
}
ECL_MEMCACHED_API bool ECL_MEMCACHED_CALL MSet(ICodeContext * _ctx, const char * servers, const char * key, unsigned short value, const char * partitionKey, unsigned expire /* = 0 (ECL default)*/)
{
    return MemCachedPlugin::MSet(_ctx, servers, partitionKey, key, sizeof(value), value, expire, MemCachedPlugin::ECL_UNSIGNED2);
}
ECL_MEMCACHED_API bool ECL_MEMCACHED_CALL MSet(ICodeContext * _ctx, const char * servers, const char * key, unsigned int value, const char * partitionKey, unsigned expire /* = 0 (ECL default)*/)
{
    return MemCachedPlugin::MSet(_ctx, servers, partitionKey, key, sizeof(value), value, expire, MemCachedPlugin::ECL_UNSIGNED4);
}
ECL_MEMCACHED_API bool ECL_MEMCACHED_CALL MSet(ICodeContext * _ctx, const char * servers, const char * key, unsigned __int64 value, const char * partitionKey, unsigned expire /* = 0 (ECL default)*/)
{
    return MemCachedPlugin::MSet(_ctx, servers, partitionKey, key, sizeof(value), value, expire, MemCachedPlugin::ECL_UNSIGNED8);
}
ECL_MEMCACHED_API bool ECL_MEMCACHED_CALL MSet(ICodeContext * _ctx, const char * servers, const char * key, float value, const char * partitionKey, unsigned expire /* = 0 (ECL default)*/)
{
    return MemCachedPlugin::MSet(_ctx, servers, partitionKey, key, sizeof(value), value, expire, MemCachedPlugin::ECL_REAL4);
}
ECL_MEMCACHED_API bool ECL_MEMCACHED_CALL MSet(ICodeContext * _ctx, const char * servers, const char * key, double value, const char * partitionKey, unsigned expire /* = 0 (ECL default)*/)
{
    return MemCachedPlugin::MSet(_ctx, servers, partitionKey, key, sizeof(value), value, expire, MemCachedPlugin::ECL_REAL8);
}
ECL_MEMCACHED_API bool ECL_MEMCACHED_CALL MSet(ICodeContext * _ctx, const char * servers, const char * key, bool value, const char * partitionKey, unsigned expire)
{
    return MemCachedPlugin::MSet(_ctx, servers, partitionKey, key, sizeof(value), value, expire, MemCachedPlugin::ECL_BOOLEAN);
}
ECL_MEMCACHED_API bool ECL_MEMCACHED_CALL MSetVoidPtr(ICodeContext * _ctx, const char * servers, const char * key, size32_t valueLength, void * value, const char * partitionKey, unsigned expire)
{
    return MemCachedPlugin::MSetVoidPtr(_ctx, servers, partitionKey, key, valueLength, value, expire, MemCachedPlugin::ECL_DATA);
}
//-------------------------------------GET----------------------------------------
ECL_MEMCACHED_API bool ECL_MEMCACHED_CALL MGetBool(ICodeContext * _ctx, const char * servers, const char * key, const char * partitionKey)
{
    bool * value = NULL;
    size_t valueLength;
    MemCachedPlugin::MGet(_ctx, servers, partitionKey, key, valueLength, value, MemCachedPlugin::ECL_BOOLEAN);
    return *value;
}
ECL_MEMCACHED_API float ECL_MEMCACHED_CALL MGetFloat(ICodeContext * _ctx, const char * servers, const char * key, const char * partitionKey)
{
    float * value = NULL;
    size_t valueLength;
    MemCachedPlugin::MGet(_ctx, servers, partitionKey, key, valueLength, value, MemCachedPlugin::ECL_REAL4);
    return *value;
}
ECL_MEMCACHED_API double ECL_MEMCACHED_CALL MGetDouble(ICodeContext * _ctx, const char * servers, const char * key, const char * partitionKey)
{
    double * value = NULL;
    size_t valueLength;
    MemCachedPlugin::MGet(_ctx, servers, partitionKey, key, valueLength, value, MemCachedPlugin::ECL_REAL8);
    return *value;
}
ECL_MEMCACHED_API signed char ECL_MEMCACHED_CALL MGetInt1(ICodeContext * _ctx, const char * servers, const char * key, const char * partitionKey)
{
    signed char * value = NULL;
    size_t valueLength;
    MemCachedPlugin::MGet(_ctx, servers, partitionKey, key, valueLength, value, MemCachedPlugin::ECL_INTEGER1);
    return *value;
}
ECL_MEMCACHED_API signed short ECL_MEMCACHED_CALL MGetInt2(ICodeContext * _ctx, const char * servers, const char * key, const char * partitionKey)
{
    signed short * value = NULL;
    size_t valueLength;
    MemCachedPlugin::MGet(_ctx, servers, partitionKey, key, valueLength, value, MemCachedPlugin::ECL_INTEGER2);
    return *value;
}
ECL_MEMCACHED_API signed int ECL_MEMCACHED_CALL MGetInt4(ICodeContext * _ctx, const char * servers, const char * key, const char * partitionKey)
{
    signed int * value = NULL;
    size_t valueLength;
    MemCachedPlugin::MGet(_ctx, servers, partitionKey, key, valueLength, value, MemCachedPlugin::ECL_INTEGER4);
    return *value;
}
ECL_MEMCACHED_API signed __int64 ECL_MEMCACHED_CALL MGetInt8(ICodeContext * _ctx, const char * servers, const char * key, const char * partitionKey)
{
    signed __int64 * value = NULL;
    size_t valueLength;
    MemCachedPlugin::MGet(_ctx, servers, partitionKey, key, valueLength, value, MemCachedPlugin::ECL_INTEGER8);
    return *value;
}
ECL_MEMCACHED_API unsigned char ECL_MEMCACHED_CALL MGetUint1(ICodeContext * _ctx, const char * servers, const char * key, const char * partitionKey)
{
    unsigned char * value = NULL;
    size_t valueLength;
    MemCachedPlugin::MGet(_ctx, servers, partitionKey, key, valueLength, value, MemCachedPlugin::ECL_UNSIGNED1);
    return *value;
}
ECL_MEMCACHED_API unsigned short ECL_MEMCACHED_CALL MGetUint2(ICodeContext * _ctx, const char * servers, const char * key, const char * partitionKey)
{
    unsigned short * value = NULL;
    size_t valueLength;
    MemCachedPlugin::MGet(_ctx, servers, partitionKey, key, valueLength, value, MemCachedPlugin::ECL_UNSIGNED2);
    return *value;
}
ECL_MEMCACHED_API unsigned int ECL_MEMCACHED_CALL MGetUint4(ICodeContext * _ctx, const char * servers, const char * key, const char * partitionKey)
{
    unsigned int * value = NULL;
    size_t valueLength;
    MemCachedPlugin::MGet(_ctx, servers, partitionKey, key, valueLength, value, MemCachedPlugin::ECL_UNSIGNED4);
    return *value;
}
ECL_MEMCACHED_API unsigned __int64 ECL_MEMCACHED_CALL MGetUint8(ICodeContext * _ctx, const char * servers, const char * key, const char * partitionKey)
{
    unsigned __int64 * value = NULL;
    size_t valueLength;
    MemCachedPlugin::MGet(_ctx, servers, partitionKey, key, valueLength, value, MemCachedPlugin::ECL_UNSIGNED8);
    return *value;
}
ECL_MEMCACHED_API char * ECL_MEMCACHED_CALL MGetVarStr(ICodeContext * _ctx, const char * servers, const char * key, const char * partitionKey)
{
    char * value = NULL;
    size_t valueLength;
    MemCachedPlugin::MGet(_ctx, servers, partitionKey, key, valueLength, value, MemCachedPlugin::ECL_VARSTRING);
    return value;
}
ECL_MEMCACHED_API UChar * ECL_MEMCACHED_CALL MGetVarUChar(ICodeContext * _ctx, const char * servers, const char * key, const char * partitionKey)
{
    UChar * value = NULL;
    size_t valueLength;
    MemCachedPlugin::MGet(_ctx, servers, partitionKey, key, valueLength, value, MemCachedPlugin::ECL_VARUNICODE);
    return value;
}
ECL_MEMCACHED_API void ECL_MEMCACHED_CALL MGetStr(ICodeContext * _ctx, size32_t & returnLength, char * & returnValue, const char * servers, const char * key, const char * partitionKey)
{
    size_t _returnLength;
    MemCachedPlugin::MGet(_ctx, servers, partitionKey, key, _returnLength, returnValue, MemCachedPlugin::ECL_STRING);
    returnLength = static_cast<size32_t>(_returnLength);
}
ECL_MEMCACHED_API void ECL_MEMCACHED_CALL MGetUChar(ICodeContext * _ctx, size32_t & returnLength, UChar * & returnValue,  const char * servers, const char * key, const char * partitionKey)
{
    size_t _returnLength;
    MemCachedPlugin::MGet(_ctx, servers, partitionKey, key, _returnLength, returnValue, MemCachedPlugin::ECL_UNICODE);
    returnLength = static_cast<size32_t>(_returnLength);
}
ECL_MEMCACHED_API void ECL_MEMCACHED_CALL MGetData(ICodeContext * _ctx, size32_t & returnLength, void * & returnValue, const char * servers, const char * key, const char * partitionKey)
{
    //MemCachedPlugin::MGetVoidPtrLenPair(_ctx, servers, partitionKey, key, returnLength, returnValue, MemCachedPlugin::ECL_DATA);
}
