TARGETS = kvstore loadBalancer masternode adminserver sleeper

FR_DEP = frontend/frontend/Http.cpp frontend/frontend/Handler.cpp frontend/frontend/utils.cpp frontend/frontend/KVStorage.cpp\
 frontend/frontend/Request.cpp frontend/frontend/Response.cpp frontend/frontend/Message.cpp frontend/frontend/MasterNode.cpp\
 frontend/frontend/html/Html.cpp frontend/main.cpp

INC_DIR = include
AUX = lib/StringOperations.cc
DEP = src/loadBalancer.cc src/testLoadBalancer.cc src/baseserver.cc
MN_DEP = src/baseserver.cc src/masternode.cc -lstdc++fs src/masterdriver.cc
SRC_DIR = src

KVSTORE_DEP = src/baseserver.cc src/storageserver.cc src/kvstore.cc lib/StringOperations.cc
ADMIN_DEP = src/baseserver.cc src/adminserver.cc -lstdc++fs src/admindriver.cc

all: $(TARGETS)

%.o: $(SRC_DIR)/%.cc
	g++ $^ -I $(INC_DIR) -c -o $@
	rm kvstore.o

loadBalancer:
	g++ -pthread $(AUX) $(DEP) -I $(INC_DIR) -o $@

kvstore: kvstore.o
	g++ -pthread $(KVSTORE_DEP) -I $(INC_DIR) -o $@
	
masternode:
	g++ -g -pthread $(AUX) $(MN_DEP) -I $(INC_DIR) -o $@

frontend/frontend_server:
	
adminserver:
	g++ -g -pthread $(AUX) $(ADMIN_DEP) -I $(INC_DIR) -o $@
	
sleeper:
	g++ sleeper.cc -o sleeper

clean:
	rm loadBalancer adminserver kvstore masternode

