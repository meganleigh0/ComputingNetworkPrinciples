This project is a client/server application that allows multiple clients/players to play connect four. 
Client-to-Server communication is done via UDP datagrams. Client-to-Client communication is done via TCP socket streams.

Compile files:
Server:
cd Server
gcc GameLogic.c server.c -o server
./server

Client:
cd Server
gcc display.c InputManage.c main.c -o client
./client

This program was tested on a GCP e2 VM instance. 
