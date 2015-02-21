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

#ifndef ECL_REDIS_ASYNC_INCL
#define ECL_REDIS_ASYNC_INCL

#include "redisplugin.hpp"

extern "C++"
{
namespace RedisPlugin {
    //------------------------ASYNC--GET----------------------------------------
    ECL_REDIS_API bool             ECL_REDIS_CALL AsncRGetBool  (ICodeContext * _ctx, const char * options, const char * key, unsigned __int64 database, const char * password, unsigned __int64 timeout);
    ECL_REDIS_API signed __int64   ECL_REDIS_CALL AsncRGetInt8  (ICodeContext * _ctx, const char * options, const char * key, unsigned __int64 database, const char * password, unsigned __int64 timeout);
    ECL_REDIS_API unsigned __int64 ECL_REDIS_CALL AsncRGetUint8 (ICodeContext * _ctx, const char * options, const char * key, unsigned __int64 database, const char * password, unsigned __int64 timeout);
    ECL_REDIS_API double           ECL_REDIS_CALL AsncRGetDouble(ICodeContext * _ctx, const char * options, const char * key, unsigned __int64 database, const char * password, unsigned __int64 timeout);
    ECL_REDIS_API void             ECL_REDIS_CALL AsncRGetUtf8  (ICodeContext * _ctx, size32_t & returnLength, char * & returnValue, const char * options, const char * key, unsigned __int64 database, const char * password, unsigned __int64 timeout);
    ECL_REDIS_API void             ECL_REDIS_CALL AsncRGetStr   (ICodeContext * _ctx, size32_t & returnLength, char * & returnValue, const char * options, const char * key, unsigned __int64 database, const char * password, unsigned __int64 timeout);
    ECL_REDIS_API void             ECL_REDIS_CALL AsncRGetUChar (ICodeContext * _ctx, size32_t & returnLength, UChar * & returnValue, const char * options, const char * key, unsigned __int64 database, const char * password, unsigned __int64 timeout);
    ECL_REDIS_API void             ECL_REDIS_CALL AsncRGetData  (ICodeContext * _ctx,size32_t & returnLength, void * & returnValue, const char * options, const char * key, unsigned __int64 database, const char * password, unsigned __int64 timeout);
    //------------------------ASYNC--SET----------------------------------------
    ECL_REDIS_API bool             ECL_REDIS_CALL AsncRSetBool (ICodeContext * _ctx, const char * options, const char * key, bool value, unsigned __int64 database, unsigned expire, const char * password, unsigned __int64 timeout);
    ECL_REDIS_API signed __int64   ECL_REDIS_CALL AsncRSetInt  (ICodeContext * _ctx, const char * options, const char * key, signed __int64 value, unsigned __int64 database, unsigned expire, const char * password, unsigned __int64 timeout);
    ECL_REDIS_API unsigned __int64 ECL_REDIS_CALL AsncRSetUInt (ICodeContext * _ctx, const char * options, const char * key, unsigned __int64 value, unsigned __int64 database, unsigned expire, const char * password, unsigned __int64 timeout);
    ECL_REDIS_API double           ECL_REDIS_CALL AsncRSetReal (ICodeContext * _ctx, const char * options, const char * key, double value, unsigned expire, const char * password, unsigned __int64 timeout);
    ECL_REDIS_API void             ECL_REDIS_CALL AsncRSetUtf8 (ICodeContext * _ctx, size32_t & returnLength, char * & returnValue, const char * options, const char * key, size32_t valueLength, const char * value, unsigned __int64 database, unsigned expire, const char * password, unsigned __int64 timeout);
    ECL_REDIS_API void             ECL_REDIS_CALL AsncRSetStr  (ICodeContext * _ctx, size32_t & returnLength, char * & returnValue, const char * options, const char * key, size32_t valueLength, const char * value, unsigned __int64 database, unsigned expire, const char * password, unsigned __int64 timeout);
    ECL_REDIS_API void             ECL_REDIS_CALL AsncRSetUChar(ICodeContext * _ctx, size32_t & returnLength, UChar * & returnValue, const char * options, const char * key, size32_t valueLength, const UChar * value, unsigned __int64 database, unsigned expire, const char * password, unsigned __int64 timeout);
    ECL_REDIS_API void             ECL_REDIS_CALL AsncRSetData (ICodeContext * _ctx, size32_t & returnLength, void * & returnValue, const char * options, const char * key, size32_t valueLength, const void * value, unsigned __int64 database, unsigned expire, const char * password, unsigned __int64 timeout);

    ECL_REDIS_API bool ECL_REDIS_CALL RMissThenLock(ICodeContext * _ctx, const char * options, const char * key, unsigned __int64 database, const char * password, unsigned __int64 timeout);
}
}
#endif
