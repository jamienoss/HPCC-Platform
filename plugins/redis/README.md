sudo apt-get install libhiredis-dev   (to build)
sudo apt-get redis-server

locate redis.conf

copy this to dir for altering and testing
find the line '#requirepass foobared' and uncomment

(note to test the redissynctest.ecl you will need another one of these with
the pass changed to 'youarefoobared' and the port to 6380)

then to run

redis-server redis.conf (may have to sudo or also change the log file dirs in redis.conf)

and that's the server running.

To watch etc
redis-cli
> auth foobared
>monitor

I open another
redis-cli
>auth foobared

So that I can manually flushdb and get etc


-------------------------------------------------
the current locking code is redislockingtest.ecl 

I run this once to access access the GET-miss-SET code and then again to access (test) the GET-already-there-code.
All subsequent runs test the latter
this is where I'm having the issue

A manual 'flushdb' will reset the above behaviour.
