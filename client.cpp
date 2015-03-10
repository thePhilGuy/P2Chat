#include <iostream>
#include <cstdlib>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <thread>
#include <atomic>
#include <chrono>
#include <future>
#include <queue>

#define MAXPENDING 5

using namespace std;



class Connection {
	public:
		Connection(string ip, int port) : ip{ip}, serverPort{port}, running{true}, incomingQueue{} {
			init_listen();
			init_send();
			cout << "Incoming port: " << listeningPort << "\n";
		}

		~Connection() { // Close socket
			running = false;
			close(listeningSocket);
		 }

		// void receive();
		void send(string message) {
			/* Initialize connecting socket */
			if ((outgoingSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
				// BETTER ERROR HANDLING
				cerr << "socket() failed";

			/* Connect to server */
			if (connect(outgoingSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
				// BETTER ERROR HANDLING
				cerr << "connect() failed";

			/* Send message and close connection */
			if (write(outgoingSocket, message.c_str(), message.length()) < 0)
				//BETTER ERROR HANDLING
				cerr << "write() failed";
			::close(outgoingSocket);
		}

		string popMessage() {
			unique_lock<mutex> lck{incomingMutex};
			hasIncoming.wait(lck);
			auto m = incomingQueue.front();
			incomingQueue.pop();
			lck.unlock();
			return m;
		}

	private:
		int listeningSocket;
		int listeningPort;
		struct sockaddr_in listeningAddr;
		
		string ip;
		int serverPort;
		int outgoingSocket;
		struct sockaddr_in serverAddr;

		atomic<bool> running;
		thread accept_thread;

		queue<string> incomingQueue;
		condition_variable hasIncoming;
		mutex incomingMutex;

		void init_listen() {
		    /* Initialize listening socket */
		    if ((listeningSocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
		    	// BETTER ERROR HANDLING 
		        cerr << "socket() failed";
		      
		    /* Build listening address structure */
		    listeningAddr = {};     	
		    listeningAddr.sin_family = AF_INET;                
		    listeningAddr.sin_addr.s_addr = htonl(INADDR_ANY); 
		    listeningAddr.sin_port = 0; // Random port to be determined later

		    /* Bind */
		    if (::bind(listeningSocket, (struct sockaddr *)&listeningAddr, sizeof(listeningAddr)) < 0)
		    	// BETTER ERROR HANDLING 
		        cerr << "bind() failed";

		    /* Listen to incoming connections */
		    if (listen(listeningSocket, MAXPENDING) < 0)
		    	// BETTER ERROR HANDLING 
		        cerr << "listen() failed";

		    /* Get client port number */
		    socklen_t len = sizeof(listeningAddr);
		    if (getsockname(listeningSocket, (struct sockaddr *)&listeningAddr, &len) < 0)
		    	cerr << "getsockname() failed";
		    listeningPort = ntohs(listeningAddr.sin_port);

		    accept_thread = thread(&Connection::accept_messages, this);
		}

		void accept_messages() {
			struct sockaddr_in incomingAddr;
			unsigned int incAddrLen = sizeof(incomingAddr);
			int incomingSocket;

			while (running) {
				if ((incomingSocket = accept(listeningSocket, (struct sockaddr *)&incomingAddr, &incAddrLen)) < 0)
					// BETTER ERROR HANDLING
					cerr << "accept() failed";
					async(launch::async, &Connection::read_message, this, incomingSocket);
			}
		}

		void read_message(int sock) {
			FILE *fp = fdopen(sock, "r");
			if (fp == NULL) cerr << "fdopen() failed";

			char buffer [256];
			memset(buffer, 0, sizeof buffer);
			
			if (fgets(buffer, sizeof buffer, fp) == NULL) cerr << "fgets() failed";
			fclose(fp);

			unique_lock<mutex> lck {incomingMutex};
			incomingQueue.push(string(buffer));
			hasIncoming.notify_one();
		}

		void init_send() {
			// Better name later
			struct hostent *s_host;
			s_host = gethostbyname(ip.c_str());

			/* Build server address structure */
		    serverAddr = {};     	
		    serverAddr.sin_family = AF_INET;               
		    memcpy((char *) s_host->h_addr, // Copy given server hostname to server address
		    	   (char *)&serverAddr.sin_addr.s_addr, s_host->h_length);
		    serverAddr.sin_port = htons(serverPort); 
		}

};



class Chat {
	public:
		Chat(string ip, int port) : running{true}, connection{ip, port} {
			// synchronously identify here

		}

		void stop() {
			running = false; 
			// receive_thread.join();
			input_thread.join();
			cout << "input_thread joined \n";
		}

		void start() {
			input_thread = thread(&Chat::init_input, this);
			message_consumer = thread(&Chat::init_consumption, this);
			// receive_thread = thread(&Chat::init_receive, this);
		}

	private:
		Connection connection;
		thread input_thread;
		thread message_consumer;
		thread message_producer;
		atomic<bool> running;

		void init_consumption() {
			while(running) {
				string message = connection.popMessage();
				async(launch::async, Chat::parseMessage, message);
			}	
		}

		void init_input() {
			string input;
			while(running) {
				getline(cin, input);
				connection.send(input);
			}
		}

		static void parseMessage (string message) { 
			cout << "Received: " << message << "\n";
		};
};

int main(int argc, char *argv[]) {
	
	if (argc != 3) {
		cerr << "Usage: ./client <server_address> <port>\n";
		return 1;
	}

	string server_address = argv[1];
	int port = atoi(argv[2]);

	Chat chat {server_address, port};
	
	chat.start();
	this_thread::sleep_for(chrono::seconds{30});
	chat.stop();	

	return 0;
}