#include "transceiver.hpp"

using namespace std;
using address = struct sockaddr_in;

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
	close(listeningSocket);
	acceptThread.join();
}

void Transceiver::send(string message) {
	int outgoing_socket;
	/* Initialize connecting socket */
	if ((outgoing_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		// BETTER ERROR HANDLING
		cerr << "socket() failed";

	/* Connect to server */
	if (connect(outgoing_socket, (struct sockaddr *)&destinationAddr, sizeof(destinationAddr)) < 0)
		// BETTER ERROR HANDLING
		cerr << "connect() failed " << errno;

	/* Send message and close connection */
	if (write(outgoing_socket, message.c_str(), message.length()) < 0)
		//BETTER ERROR HANDLING
		cerr << "write() failed";
	::close(outgoing_socket);
}

void Transceiver::send(string text, address addr) {
	int outgoing_socket;

	/* Initialize connecting socket */
	if ((outgoing_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		// BETTER ERROR HANDLING
		cerr << "socket() failed";

	/* Connect to server */
	if (connect(outgoing_socket, (struct sockaddr *)&addr, sizeof(addr)) < 0)
		// BETTER ERROR HANDLING
		cerr << "connect() failed " << errno;

	/* Send message and close connection */
	if (write(outgoing_socket, text.c_str(), text.length()) < 0)
		//BETTER ERROR HANDLING
		cerr << "write() failed";
	::close(outgoing_socket);
}

tuple<string, address> Transceiver::popMessage() {
	unique_lock<mutex> lck{incomingMutex};
	hasIncoming.wait(lck);
	auto m = incomingQueue.front();
	incomingQueue.pop();
	lck.unlock();
	return m;
}

string Transceiver::getListenPort() {
	return to_string(listenPort);
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
		async(launch::async, &Transceiver::enqueue_message, this, incoming_socket, incoming_addr);
	}
}

void Transceiver::enqueue_message(int sock, address addr) {
	FILE *fp = fdopen(sock, "r");
	if (fp == NULL) cerr << "fdopen() failed"; // switch this with a try

	char buffer [256];
	memset(buffer, 0, sizeof buffer);

	if (fgets(buffer, sizeof buffer, fp) == NULL) cerr << "fgets() failed"; // include in first try
	fclose(fp);

	unique_lock<mutex> lck {incomingMutex};
	incomingQueue.push(tuple<string, address> {string(buffer), addr});
	hasIncoming.notify_one();
}
