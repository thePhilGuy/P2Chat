#include "transceiver.hpp"

using namespace std;

Transceiver::Transceiver(int listen_port) :
	listenPort{listen_port},
	running{true},
	incomingQueue{}
{
	init_listen();
}

Transceiver::Transceiver(int listen_port, int destination_port, string destination_ip) :
	listenPort{listen_port},
	destinationPort{destination_port},
	destinationIP{destination_ip},
	running{true},
	incomingQueue{}
{
	init_listen();
	init_send();
}

Transceiver::Transceiver(int destination_port, string destination_ip) :
	listenPort{0},
	destinationPort{destination_port},
	destinationIP{destination_ip},
	running{true},
	incomingQueue{}
{
	init_listen();
	init_send();
}

void Transceiver::init_listen() {
	/* Initialize listening socket */
	if ((listeningSocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
		cerr << "socket() failed";

	/* Build listening address structure */
	listeningAddr = {};     	
	listeningAddr.sin_family = AF_INET;                
	listeningAddr.sin_addr.s_addr = htonl(INADDR_ANY); 
	listeningAddr.sin_port = htons(listenPort);

	/* Bind */
	if (::bind(listeningSocket, (struct sockaddr *)&listeningAddr, sizeof(listeningAddr)) < 0)
		cerr << "bind() failed";

	/* Listen to incoming connections */
	if (listen(listeningSocket, MAXPENDING) < 0)
		cerr << "listen() failed";

	accept_thread = thread(&Connection::accept_messages, this);
}