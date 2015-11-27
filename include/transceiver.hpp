#ifndef TRANSCEIVER_HPP
#define TRANSCEIVER_HPP

// Add template for data format
class Transceiver {
public:
	Transceiver(string ip, int port); 
	Transceiver(int port);

	void send(string message);
	void send(string text, struct sockaddr_in addr);

	string popMessage();
	tuple<string, struct sockaddr_in> popMessage;

	string getListeningPort();

private:
	// Server
	int listeningSocket;
	int listeningPort;
	struct sockaddr_in listeningAddr;

	atomic<bool> running;
	thread accept_thread;

	// Solve with templating
	queue<string> incomingQueue;
	queue<tuple<string, struct sockaddr_in>> incomingQueue;
	condition_variable hasIncoming;
	mutex incomingMutex;

	string ip;
	int serverPort;
	int outgoingSocket;
	struct sockaddr_in serverAddr;

protected:
	// Server Methods
	void init_listen();
	void accept_messages();
	void read_message(int sock, struct sockaddr_in addr);	
	// Client Methods
	void read_message(int sock);
	void init_send();
};

#endif // TRANSCEIVER_HPP
