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

#ifndef ECL_REDIS_LOCK_INCL
#define ECL_REDIS_LOCK_INCL

#include "redisplugin.hpp"

extern "C++"
{
    ECL_REDIS_API char * ECL_REDIS_CALL RGetLockObject(ICodeContext * ctx, const char * options, const char * key);
    ECL_REDIS_API bool ECL_REDIS_CALL RMissAndLock(ICodeContext * _ctx, char * keyPtr);
}

#endif
