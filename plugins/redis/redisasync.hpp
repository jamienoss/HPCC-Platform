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

#ifndef ECL_REDIS_AAsync_INCL
#define ECL_REDIS_AAsync_INCL

#include "redisplugin.hpp"

extern "C++"
{
namespace RedisPlugin {
   //--------------------------SET----------------------------------------
   ECL_REDIS_API bool             ECL_REDIS_CALL AsyncRSetBool (ICodeContext * _ctx, const char * key, bool value, const char * options, unsigned __int64 database, unsigned expire, const char * pswd, unsigned timeout);
   ECL_REDIS_API signed __int64   ECL_REDIS_CALL AsyncRSetInt  (ICodeContext * _ctx, const char * key, signed __int64 value, const char * options, unsigned __int64 database, unsigned expire, const char * pswd, unsigned timeout);
   ECL_REDIS_API unsigned __int64 ECL_REDIS_CALL AsyncRSetUInt (ICodeContext * _ctx, const char * key, unsigned __int64 value, const char * options, unsigned __int64 database, unsigned expire, const char * pswd, unsigned timeout);
   ECL_REDIS_API double           ECL_REDIS_CALL AsyncRSetReal (ICodeContext * _ctx, const char * key, double value, const char * options, unsigned __int64 database, unsigned expire, const char * pswd, unsigned timeout);
   ECL_REDIS_API void             ECL_REDIS_CALL AsyncRSetUtf8 (ICodeContext * _ctx, size32_t & returnLength, char * & returnValue, const char * key, size32_t valueLength, const char * value, const char * options, unsigned __int64 database, unsigned expire, const char * pswd, unsigned timeout);
   ECL_REDIS_API void             ECL_REDIS_CALL AsyncRSetStr  (ICodeContext * _ctx, size32_t & returnLength, char * & returnValue, const char * key, size32_t valueLength, const char * value, const char * options, unsigned __int64 database, unsigned expire, const char * pswd, unsigned timeout);
   ECL_REDIS_API void             ECL_REDIS_CALL AsyncRSetUChar(ICodeContext * _ctx, size32_t & returnLength, UChar * & returnValue, const char * key, size32_t valueLength, const UChar * value, const char * options, unsigned __int64 database, unsigned expire, const char * pswd, unsigned timeout);
   ECL_REDIS_API void             ECL_REDIS_CALL AsyncRSetData (ICodeContext * _ctx, size32_t & returnLength, void * & returnValue, const char * key, size32_t valueLength, const void * value, const char * options, unsigned __int64 database, unsigned expire, const char * pswd, unsigned timeout);
   //--------------------------GET----------------------------------------
   ECL_REDIS_API bool             ECL_REDIS_CALL AsyncRGetBool  (ICodeContext * _ctx, const char * key, const char * options, unsigned __int64 database, const char * pswd, unsigned timeout);
   ECL_REDIS_API signed __int64   ECL_REDIS_CALL AsyncRGetInt8  (ICodeContext * _ctx, const char * key, const char * options, unsigned __int64 database, const char * pswd, unsigned timeout);
   ECL_REDIS_API unsigned __int64 ECL_REDIS_CALL AsyncRGetUint8 (ICodeContext * _ctx, const char * key, const char * options, unsigned __int64 database, const char * pswd, unsigned timeout);
   ECL_REDIS_API double           ECL_REDIS_CALL AsyncRGetDouble(ICodeContext * _ctx, const char * key, const char * options, unsigned __int64 database, const char * pswd, unsigned timeout);
   ECL_REDIS_API void             ECL_REDIS_CALL AsyncRGetUtf8  (ICodeContext * _ctx, size32_t & returnLength, char * & returnValue, const char * key, const char * options, unsigned __int64 database, const char * pswd, unsigned timeout);
   ECL_REDIS_API void             ECL_REDIS_CALL AsyncRGetStr   (ICodeContext * _ctx, size32_t & returnLength, char * & returnValue, const char * key, const char * options, unsigned __int64 database, const char * pswd, unsigned timeout);
   ECL_REDIS_API void             ECL_REDIS_CALL AsyncRGetUChar (ICodeContext * _ctx, size32_t & returnLength, UChar * & returnValue, const char * key, const char * options, unsigned __int64 database, const char * pswd, unsigned timeout);
   ECL_REDIS_API void             ECL_REDIS_CALL AsyncRGetData  (ICodeContext * _ctx,size32_t & returnLength, void * & returnValue, const char * key, const char * options, unsigned __int64 database, const char * pswd, unsigned timeout);

    ECL_REDIS_API bool ECL_REDIS_CALL AsyncRMissThenLock(ICodeContext * _ctx, const char * key, const char * options, unsigned __int64 database, const char * password, unsigned __int64 timeout);
}
}
#endif
