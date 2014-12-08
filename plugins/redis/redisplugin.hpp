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

extern "C"
{
    ECL_REDIS_API bool getECLPluginDefinition(ECLPluginDefinitionBlock *pb);
    ECL_REDIS_API void setPluginContext(IPluginContext * _ctx);
}

extern "C++"
{
    //--------------------------SET----------------------------------------
    /*ECL_REDIS_API void ECL_REDIS_CALL RSet    (ICodeContext * _ctx, const char * servers, const char * key, bool value, const char * partitionKey, unsigned expire);
    ECL_REDIS_API void ECL_REDIS_CALL RSet    (ICodeContext * _ctx, const char * servers, const char * key, signed __int64 value, const char * partitionKey, unsigned expire);
    ECL_REDIS_API void ECL_REDIS_CALL RSet    (ICodeContext * _ctx, const char * servers, const char * key, unsigned __int64 value, const char * partitionKey, unsigned expire);
    ECL_REDIS_API void ECL_REDIS_CALL RSet    (ICodeContext * _ctx, const char * servers, const char * key, double value, const char * partitionKey, unsigned expire);
    ECL_REDIS_API void ECL_REDIS_CALL RSetUtf8(ICodeContext * _ctx, const char * servers, const char * key, size32_t valueLength, const char * value, const char * partitionKey, unsigned expire);
    */ECL_REDIS_API void ECL_REDIS_CALL RSet    (ICodeContext * _ctx, const char * servers, const char * key, size32_t valueLength, const char * value, const char * partitionKey, unsigned expire);
    /*ECL_REDIS_API void ECL_REDIS_CALL RSet    (ICodeContext * _ctx, const char * servers, const char * key, size32_t valueLength, const UChar * value, const char * partitionKey, unsigned expire);
    ECL_REDIS_API void ECL_REDIS_CALL RSetData(ICodeContext * _ctx, const char * servers, const char * key, size32_t valueLength, const void * value, const char * partitionKey, unsigned expire);
    //--------------------------GET----------------------------------------
    ECL_REDIS_API bool             ECL_REDIS_CALL RGetBool  (ICodeContext * _ctx, const char * servers, const char * key, const char * partitionKey);
    ECL_REDIS_API signed __int64   ECL_REDIS_CALL RGetInt8  (ICodeContext * _ctx, const char * servers, const char * key, const char * partitionKey);
    ECL_REDIS_API unsigned __int64 ECL_REDIS_CALL RGetUint8 (ICodeContext * _ctx, const char * servers, const char * key, const char * partitionKey);
    ECL_REDIS_API double           ECL_REDIS_CALL RGetDouble(ICodeContext * _ctx, const char * servers, const char * key, const char * partitionKey);
    ECL_REDIS_API void             ECL_REDIS_CALL RGetUtf8  (ICodeContext * _ctx, size32_t & valueLength, char * & returnValue, const char * servers, const char * key, const char * partitionKey);
    */ECL_REDIS_API void             ECL_REDIS_CALL RGetStr   (ICodeContext * _ctx, size32_t & valueLength, UChar * & returnValue, const char * servers, const char * key, const char * partitionKey);
    /*ECL_REDIS_API void             ECL_REDIS_CALL RGetUChar (ICodeContext * _ctx, size32_t & valueLength, char * & returnValue, const char * servers, const char * key, const char * partitionKey);
    ECL_REDIS_API void             ECL_REDIS_CALL RGetData  (ICodeContext * _ctx,size32_t & returnLength, void * & returnValue, const char * servers, const char * key, const char * partitionKey);
    //--------------------------------AUXILLARIES---------------------------
    ECL_REDIS_API bool             ECL_REDIS_CALL RExist  (ICodeContext * _ctx, const char * servers, const char * key, const char * partitionKey);
    ECL_REDIS_API const char *     ECL_REDIS_CALL RKeyType(ICodeContext * _ctx, const char * servers, const char * key, const char * partitionKey);
    ECL_REDIS_API void             ECL_REDIS_CALL RClear  (ICodeContext * _ctx, const char * servers);
    */
}
#endif
