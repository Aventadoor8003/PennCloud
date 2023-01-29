TARGETS = remove_former frontend_server clean

all: $(TARGETS)

remove_former:
	rm -rf frontend_server
	rm -rf mailserver	
frontend_server:
	g++ -c -g main.cpp -pthread
	g++ -c frontend/Http.cpp -pthread
	g++ -c frontend/Handler.cpp -pthread
	g++ -c frontend/utils.cpp -std=c++17
	g++ -c frontend/KVStorage.cpp
	g++ -c frontend/Request.cpp
	g++ -c frontend/Response.cpp
	g++ -c frontend/Message.cpp
	g++ -c -g frontend/MasterNode.cpp
	g++ -c frontend/html/Html.cpp
	g++ -c frontend/FileBody.cpp
	g++ -c frontend/BalanceNode.cpp
	g++ -c frontend/dirFileModel.cpp
	g++ -c frontend/smtp.cpp
	g++ -std=c++17 -o frontend_server main.o Http.o Handler.o utils.o KVStorage.o Request.o Message.o Response.o Html.o MasterNode.o FileBody.o BalanceNode.o dirFileModel.o -lresolv -pthread
	g++ -std=c++17 -o mailserver smtp.o Http.o Handler.o utils.o KVStorage.o Request.o Message.o Response.o Html.o MasterNode.o FileBody.o BalanceNode.o  dirFileModel.o -lresolv -pthread
clean:
	rm -rf main.o
	rm -rf Http.o
	rm -rf Handler.o
	rm -rf utils.o
	rm -rf KVStorage.o
	rm -rf Request.o
	rm -rf Response.o
	rm -rf Html.o
	rm -rf Message.o
	rm -rf MasterNode.o
	rm -rf FileBody.o
	rm -rf BalanceNode.o
	rm -rf dirFileModel.o
	rm -rf smtp.o
run:
	sudo ./frontend_server
