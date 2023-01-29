#!/bin/sh
./adminserver -a 127.0.0.1:23333 -c config/serversreplicated.txt -f frontend/frontend/config.txt & 
./loadBalancer -p 5000 frontend/frontend/config.txt &
./masternode -a 127.0.0.1:2334 -c config/serversreplicated.txt & ./sleeper &
./run9nodes.sh
