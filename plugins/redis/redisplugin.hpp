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

#include "jhash.hpp"
#include "hqlplugins.hpp"
#include "eclhelper.hpp"
#include "jexcept.hpp"
#include "hiredis/hiredis.h"

extern "C"
{
    ECL_REDIS_API bool getECLPluginDefinition(ECLPluginDefinitionBlock *pb);
    ECL_REDIS_API void setPluginContext(IPluginContext * _ctx);
}

class StringBuffer;

namespace RedisPlugin {
static const struct timeval REDIS_TIMEOUT = { 1, 500000 }; // { sec, ms } => 1.5 seconds
static const unsigned unitExpire = 86400;//1 day (secs)

#define setFailMsg "'Set' request failed - "
#define getFailMsg "'Get<type>' request failed - "

StringBuffer & appendExpire(StringBuffer & buffer, unsigned expire);

class RedisServer : public CInterface
{
public :
    RedisServer() : port(0) { }
    RedisServer(ICodeContext * ctx, const char * _options)
    {
        serverPswdHash = hashc((const unsigned char*)_options, strlen(_options), 0);
        options.set(_options);
        parseOptions(ctx, _options);
    }
    RedisServer(ICodeContext * ctx, const char * _options, const char * pswd) : RedisServer(ctx, _options)
    {
        serverPswdHash = hashc((const unsigned char*)_options, strlen(_options), serverPswdHash);
    }
    bool isSame(ICodeContext * ctx, unsigned hash) const
    {
        return (serverPswdHash == hash);
    }
    const char * getIp() { return ip.str(); }
    int getPort() { return port; }
    void parseOptions(ICodeContext * ctx, const char * _options);

private :
    unsigned serverPswdHash;
    StringAttr options;
    StringAttr ip;
    int port;
};
class Connection : public CInterface
{
public :
    Connection(ICodeContext * ctx, const char * _options, const char * pswd);//remove database from these
    Connection(ICodeContext * ctx, RedisServer * _server);

    bool isSameConnection(ICodeContext * ctx, unsigned hash) const;
    const char * ip() const { return server->getIp(); }
    int port() const { return server->getPort(); }

protected :
    virtual void assertOnError(const redisReply * reply, const char * _msg) { }
    virtual void assertConnection() { }
    virtual void logServerStats(ICodeContext * ctx) { }
    virtual bool logErrorOnFail(ICodeContext * ctx, const redisReply * reply, const char * _msg) { return false; }

    const char * appendIfKeyNotFoundMsg(const redisReply * reply, const char * key, StringBuffer & target) const;
    void * cpy(const char * src, size_t size);
    void init(ICodeContext * ctx);

protected :
    Owned<RedisServer> server;
    unsigned __int64 database;
    bool alreadyInitialized;
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

}//close namespace

#endif
