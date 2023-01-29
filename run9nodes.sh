#!/bin/sh
./kvstore ./config/serversreplicated.txt 1 1 -v & ./sleeper
./kvstore ./config/serversreplicated.txt 2 1 -v & ./sleeper
./kvstore ./config/serversreplicated.txt 3 1 -v & ./sleeper
./kvstore ./config/serversreplicated.txt 1 2 -v & ./sleeper
./kvstore ./config/serversreplicated.txt 2 2 -v & ./sleeper
./kvstore ./config/serversreplicated.txt 3 2 -v & ./sleeper
./kvstore ./config/serversreplicated.txt 1 3 -v & ./sleeper
./kvstore ./config/serversreplicated.txt 2 3 -v & ./sleeper
./kvstore ./config/serversreplicated.txt 3 3 -v
