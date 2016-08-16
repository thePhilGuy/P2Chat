#ifndef TRANSCEIVER_HPP
#define TRANSCEIVER_HPP

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>

#include <thread>
#include <atomic>
#include <future>

#include <queue>
#include <iterator>
#include <algorithm>
#include <numeric>

#include <string>
#include <sstream>
#include <iostream>
#include <fstream>

class Transceiver {
	using address = struct sockaddr_in;
public:
	/* Constructor
	* Constructs a Transceiver object listening on given port
	* \param listen_port port transceiver listens on for incoming tcp messages
	*/
	Transceiver(int listen_port);

	/* Constructor
	* Constructs a Transceiver object listening on given port with default destination
	* \param listen_port port transceiver listens on for incoming tcp messages
	* \param destination_port default destination port of transceiver messages
	* \param destination_ip default destination ip of transceiver messages (human readable)
	*/
	Transceiver(int listen_port, int destination_port, std::string destination_ip);

	/* Constructor
	* Constructs a Transceiver object listening on random port with default destination
	* \param destination_port default destination port of transceiver messages
	* \param destination_ip default destination ip of transceiver messages (human readable)
	*/
	Transceiver(int destination_port, std::string destination_ip);

	/// Destructor
	~Transceiver();

	/// Send message to default destination
	void send(std::string message);
	/// Send message to given internet address
	void send(std::string text, address addr);

	/// Remove received message and sender from queue.
	std::tuple<std::string, address> popMessage();

	/// Get the port on which transceiver receives incoming messages
	int getListenPort();

private:
	// Incoming address information
	int listeningSocket;
	int listenPort;
	address listeningAddr;

	// destination address information
	std::string destinationIP;
	int destinationPort;
	int destinationSocket;
	address destinationAddr;

	std::atomic<bool> running;
	std::thread acceptThread;

	std::queue<std::tuple<std::string, address>> incomingQueue;
	std::condition_variable hasIncoming;
	std::mutex incomingMutex;

protected:
	void init_listen();
	void init_send();
	void accept_messages();
	void enqueue_message(int sock, address addr);
};

#endif // TRANSCEIVER_HPP
