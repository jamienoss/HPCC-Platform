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

#ifndef ECL_REDIS_INCL
#define ECL_REDIS_INCL

#ifdef _WIN32
#define ECL_REDIS_CALL _cdecl
#ifdef ECL_REDIS_EXPORTS
#define ECL_REDIS_API __declspec(dllexport)
#else
#define ECL_REDIS_API __declspec(dllimport)
#endif
#else
#define ECL_REDIS_CALL
#define ECL_REDIS_API
#endif

#include "hqlplugins.hpp"
#include "eclhelper.hpp"
#include "jexcept.hpp"
#include <hiredis/hiredis.h>

extern "C"
{
    ECL_REDIS_API bool getECLPluginDefinition(ECLPluginDefinitionBlock *pb);
    ECL_REDIS_API void setPluginContext(IPluginContext * _ctx);
}

class StringBuffer;

namespace RedisPlugin {

static const unsigned unitExpire = 86400;//1 day (secs)
#define MAX_TYPEMISMATCHCOUNT 10;

//redis base commands
#define setCmd "SET %b %b"
#define getCmd "GET %b"

#define setFailMsg "'Set' request failed - "
#define getFailMsg "'Get<type>' request failed - "

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
const char * enumToStr(eclDataType type);
StringBuffer & appendExpire(StringBuffer & buffer, unsigned expire);

class Connection : public CInterface
{
public :
    Connection(ICodeContext * ctx, const char * _options);
    virtual void clear(ICodeContext * ctx, unsigned when) { };
    //eclDataType getKeyType(const char * key, const char * partitionKey);
    bool isSameConnection(const char * _options) const;

protected :
    virtual void assertOnError(const redisReply * reply, const char * _msg) { };
    virtual bool logErrorOnFail(ICodeContext * ctx, const redisReply * reply, const char * _msg) { return FALSE; };
    virtual void assertConnection() { };

    void checkServersUp(ICodeContext * ctx);
    const char * appendIfKeyNotFoundMsg(const redisReply * reply, const char * key, StringBuffer & target) const;
    void * cpy(const char * src, size_t length);
    void logServerStats(ICodeContext * ctx);
    void init(ICodeContext * ctx);
    void invokePoolSecurity(ICodeContext * ctx);
    void invokeConnectionSecurity(ICodeContext * ctx);
    void setPoolSettings();

protected :
    StringAttr options;
    StringAttr master;
    unsigned port;
    bool alreadyInitialized;
    unsigned typeMismatchCount;
};

class Reply : public CInterface
{
public :
    inline Reply() { reply = NULL; };
    inline Reply(void * _reply){ reply = (redisReply*)_reply; }
    inline Reply(redisReply * _reply){ reply = _reply; }
    inline ~Reply()
    {
        if (reply)
            freeReplyObject(reply);
    }

    inline const redisReply * query() const { return reply; }
    const char * typeToStr() const;

private :
    redisReply * reply;
};
Reply * createReply(void * _reply);
#define OwnedReply Owned<RedisPlugin::Reply>

void parseOptions(const char * options, StringAttr & master, unsigned & port);

}//close namespace

#endif