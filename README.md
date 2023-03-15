# ClientServer_CustomUDP - Client using customized protocol on top of UDP protocol for sending information to the server.


Pre-requisite:
Install gcc for C program compiler

How to compile and run in Linux:

1. Copy the files 'client.c', 'server.c' and 'packet_payload.txt' to the desired location.
2. Run the below commands to compile the C programs:
	gcc server.c -o ServerExe
	gcc client.c -o ClientExe
3. First the server should be started. To start the server, run:
	./ServerExe
4. In a new terminal window, run below for running the client program:
	./ClientExe
5. Packets would start transmitting and output would be visible
