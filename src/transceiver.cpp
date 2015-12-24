#include "transceiver.hpp"

using namespace std;

// =============================================================================
// Public Functions
// =============================================================================

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

Transceiver::~Transceiver() {
	running = false;
	close(listenSocket);
	acceptThread.join();
}

void send(string message) {

}

void send(string text, address addr) {

}

auto popMessage() {

}

int getListenPort() {

}

// =============================================================================
// Protected Functions
// =============================================================================

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

	acceptThread = thread(&Transceiver::accept_messages, this);
}

void Transceiver::init_send() {
	/* Get network address from default destination hostname */
	struct hostent *destination_host;
	destination_host = gethostbyname(destinationIP.c_str());

	/* Build default destination address structure */
	destinationAddr = {};
	destinationAddr.sin_family = AF_INET;
	memcpy((char *) destination_host->h_addr, // Copy given server hostname to server address
		   (char *)&destinationAddr.sin_addr.s_addr, destination_host->h_length);
	destinationAddr.sin_port = htons(destinationPort);
}

void Transceiver::accept_messages() {
	address incoming_addr;
	uint inc_addr_len = sizeof(incoming_addr); // might have to correct to unsigned int
	int incoming_socket;

	cout << "Accepting messages on port: " << listenPort << "\n";

	while (running) {
		if ((incoming_socket = accept(listeningSocket, (struct sockaddr *)&incoming_addr, &inc_addr_len)) < 0)
			// BETTER ERROR HANDLING
			cerr << "accept() failed";
			async(launch::async, &Transceiver::read_message, this, incoming_socket, incoming_addr);
	}
}

void Transceiver::enqueue_message(int sock, address addr) {

}
