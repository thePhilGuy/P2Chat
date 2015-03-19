Name: Philippe-Guillaume Losembe
UNI: pvl2109

Compilation: Running make compiles everything

Usage: 
	MessageCenter: ./center [port]
	Chat: ./client [center address] [port]

I have made a connection class that is very similar in both server.cpp and client.cpp that I will eventually merge into one usable library. It is very useful in abstracting from low level C socket programming  but still needs work.

When started the client starts an instance of Chat ( the heartbeat delay is defined under headers as a constant) that communicates with the server's instance of MessageCenter.

Everything should be implemented except graceful exit on Ctrl + C and the extra credit portions.