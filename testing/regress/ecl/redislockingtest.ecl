/*##############################################################################

    HPCC SYSTEMS software Copyright (C) 2015 HPCC Systems.

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

IMPORT redisServer FROM lib_redis;

STRING server := '--SERVER=127.0.0.1:6379';
STRING password := 'foobared';
myRedis := redisServer(server, password);

key := 'Einstein';
//myRedis.FlushDB();

myFunc() := FUNCTION
 out := output('uh oh!');
 return WHEN('Good boy Einnie!', out);
END;

//myRedis.locking.getString2(key, myFunc, 0/*database*/, 60/*expiration*/)
myRedis.flushdb();

IF (myRedis.locking.Exists(key),
    myRedis.locking.GetString(key),
    myRedis.locking.SetString(key, myFunc())
    );

SEQUENTIAL(
  myRedis.setString('testlock', 'redis_ecl_lock_blah_blah_blah'),
  myRedis.exists('testlock');
  myRedis.locking.unlock('testlock'),
  myRedis.exists('testlock')
  );


//myRedis.FlushDB();