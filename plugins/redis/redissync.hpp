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

#ifndef ECL_REDIS_SYNC_INCL
#define ECL_REDIS_SYNC_INCL

#include "redisplugin.hpp"

namespace Sync
{
class Connection : public RedisPlugin::Connection
{
public :
    Connection(ICodeContext * ctx, const char * _options);
    ~Connection()
    {
        if (connection)
            redisFree(connection);
    }

    //set
    template <class type> void set(ICodeContext * ctx, const char * partitionKey, const char * key, type value, unsigned expire, RedisPlugin::eclDataType eclType);
    template <class type> void set(ICodeContext * ctx, const char * partitionKey, const char * key, size32_t valueLength, const type * value, unsigned expire, RedisPlugin::eclDataType eclType);
    //get
    template <class type> void get(ICodeContext * ctx, const char * partitionKey, const char * key, type & value, RedisPlugin::eclDataType eclType);
    template <class type> void get(ICodeContext * ctx, const char * partitionKey, const char * key, size_t & valueLength, type * & value, RedisPlugin::eclDataType eclType);
    void getVoidPtrLenPair(ICodeContext * ctx, const char * partitionKey, const char * key, size_t & valueLength, void * & value, RedisPlugin::eclDataType eclType);
    bool exist(ICodeContext * ctx, const char * key, const char * partitionKey);
    void persist(ICodeContext * ctx, const char * key, const char * partitionKey);
    void expire(ICodeContext * ctx, const char * key, const char * partitionKey, unsigned _expire);
    virtual void del(ICodeContext * ctx, const char * key, const char * partitionKey);
    virtual void clear(ICodeContext * ctx, unsigned when);

    bool ifLockedSub(ICodeContext * ctx, const char * key, const redisReply * reply);

protected :
    virtual void assertOnError(const redisReply * reply, const char * _msg);
    virtual bool logErrorOnFail(ICodeContext * ctx, const redisReply * reply, const char * _msg);
    virtual void assertConnection();
    const char * getErrStr() { return connection->errstr; }

protected :
    redisContext * connection;
};
}//close namespace

extern "C++"
{
    //--------------------------SET----------------------------------------
    ECL_REDIS_API void ECL_REDIS_CALL RSet    (ICodeContext * _ctx, const char * options, const char * key, bool value, const char * partitionKey, unsigned expire);
    ECL_REDIS_API void ECL_REDIS_CALL RSet    (ICodeContext * _ctx, const char * options, const char * key, signed __int64 value, const char * partitionKey, unsigned expire);
    ECL_REDIS_API void ECL_REDIS_CALL RSet    (ICodeContext * _ctx, const char * options, const char * key, unsigned __int64 value, const char * partitionKey, unsigned expire);
    ECL_REDIS_API void ECL_REDIS_CALL RSet    (ICodeContext * _ctx, const char * options, const char * key, double value, const char * partitionKey, unsigned expire);
    ECL_REDIS_API void ECL_REDIS_CALL RSetUtf8(ICodeContext * _ctx, const char * options, const char * key, size32_t valueLength, const char * value, const char * partitionKey, unsigned expire);
    ECL_REDIS_API void ECL_REDIS_CALL RSet    (ICodeContext * _ctx, const char * options, const char * key, size32_t valueLength, const char * value, const char * partitionKey, unsigned expire);
    ECL_REDIS_API void ECL_REDIS_CALL RSet    (ICodeContext * _ctx, const char * options, const char * key, size32_t valueLength, const UChar * value, const char * partitionKey, unsigned expire);
    ECL_REDIS_API void ECL_REDIS_CALL RSetData(ICodeContext * _ctx, const char * options, const char * key, size32_t valueLength, const void * value, const char * partitionKey, unsigned expire);
    //--------------------------GET----------------------------------------
    ECL_REDIS_API bool             ECL_REDIS_CALL RGetBool  (ICodeContext * _ctx, const char * options, const char * key, const char * partitionKey);
    ECL_REDIS_API signed __int64   ECL_REDIS_CALL RGetInt8  (ICodeContext * _ctx, const char * options, const char * key, const char * partitionKey);
    ECL_REDIS_API unsigned __int64 ECL_REDIS_CALL RGetUint8 (ICodeContext * _ctx, const char * options, const char * key, const char * partitionKey);
    ECL_REDIS_API double           ECL_REDIS_CALL RGetDouble(ICodeContext * _ctx, const char * options, const char * key, const char * partitionKey);
    ECL_REDIS_API void             ECL_REDIS_CALL RGetUtf8  (ICodeContext * _ctx, size32_t & valueLength, char * & returnValue, const char * options, const char * key, const char * partitionKey);
    ECL_REDIS_API void             ECL_REDIS_CALL RGetStr   (ICodeContext * _ctx, size32_t & valueLength, UChar * & returnValue, const char * options, const char * key, const char * partitionKey);
    ECL_REDIS_API void             ECL_REDIS_CALL RGetUChar (ICodeContext * _ctx, size32_t & valueLength, char * & returnValue, const char * options, const char * key, const char * partitionKey);
    ECL_REDIS_API void             ECL_REDIS_CALL RGetData  (ICodeContext * _ctx,size32_t & returnLength, void * & returnValue, const char * options, const char * key, const char * partitionKey);

    //--------------------------------AUXILLARIES---------------------------
    ECL_REDIS_API bool             ECL_REDIS_CALL RExist  (ICodeContext * _ctx, const char * options, const char * key, const char * partitionKey);
    //ECL_REDIS_API const char *     ECL_REDIS_CALL RKeyType(ICodeContext * _ctx, const char * options, const char * key, const char * partitionKey);
    ECL_REDIS_API void             ECL_REDIS_CALL RClear  (ICodeContext * _ctx, const char * options);
    ECL_REDIS_API void             ECL_REDIS_CALL RDel  (ICodeContext * _ctx, const char * options, const char * key, const char * partitionKey);
    ECL_REDIS_API void             ECL_REDIS_CALL RPersist  (ICodeContext * _ctx, const char * options, const char * key, const char * partitionKey);
    ECL_REDIS_API void             ECL_REDIS_CALL RExpire  (ICodeContext * _ctx, const char * options, const char * key, unsigned expire, const char * partitionKey);

    ECL_REDIS_API void ECL_REDIS_CALL RCmdStr(ICodeContext * _ctx, size32_t & returnLength, void * & value, const char * options, const char * key);
}

#endif
