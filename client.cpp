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
#include <sstream>
#include <string>
#include <iterator>
#include <fstream>
#include <numeric>
#include <algorithm>

#define MAXPENDING 5
#define HEARTBEAT_DELAY 30

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
			accept_thread.join();
		 }

		// void receive();
		void send(string message) {
			cout << "Sending: " << message << "\n";
			/* Initialize connecting socket */
			if ((outgoingSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
				// BETTER ERROR HANDLING
				cerr << "socket() failed";

			/* Connect to server */
			if (connect(outgoingSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
				// BETTER ERROR HANDLING
				cerr << "connect() failed " << errno;

			/* Send message and close connection */
			if (write(outgoingSocket, message.c_str(), message.length()) < 0)
				//BETTER ERROR HANDLING
				cerr << "write() failed";
			::close(outgoingSocket);
		}

		void send(string text, struct sockaddr_in addr) {

			cout << "Sending to client on port: " << ntohs(addr.sin_port) << "\n";

			int outSocket;

			/* Initialize connecting socket */
			if ((outSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
				// BETTER ERROR HANDLING
				cerr << "socket() failed";

			/* Connect to server */
			if (connect(outSocket, (struct sockaddr *)&addr, sizeof(addr)) < 0)
				// BETTER ERROR HANDLING
				cerr << "connect() failed " << errno;

			/* Send message and close connection */
			if (write(outSocket, text.c_str(), text.length()) < 0)
				//BETTER ERROR HANDLING
				cerr << "write() failed";
			::close(outSocket);
		}

		string popMessage() {
			unique_lock<mutex> lck{incomingMutex};
			hasIncoming.wait(lck);
			auto m = incomingQueue.front();
			incomingQueue.pop();
			lck.unlock();
			return m;
		}

		auto getListeningPort() {
			return to_string(listeningPort);
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
		Chat(string ip, int port) : running{true}, connection{ip, port}, peers{} {
			// synchronously identify here
			string username;
			string password;
			cout << "username: ";
			cin >> username;
			cout << "password: ";
			cin >> password;

			string credentials = username + ' ' + password + ' ' + connection.getListeningPort() + '\n';
			chatUser = username;
			connection.send(credentials);

			// For some reason, flush the standard input
			cin.ignore();
		}

		void stop() {
			running = false; 
			message_consumer.join();
			input_thread.join();
			heartbeat_thread.join();
		}

		void start() {
			input_thread = thread(&Chat::init_input, this);
			message_consumer = thread(&Chat::init_consumption, this);
			heartbeat_thread = thread(&Chat::init_heartbeat, this);
		}

		bool isRunning() {
			return running;
		}

	private:
		string chatUser;
		Connection connection;
		thread input_thread;
		thread message_consumer;
		thread heartbeat_thread;
		atomic<bool> running;
		/* peer : name address port */
		vector<tuple<string, struct sockaddr_in>> peers;

		void init_consumption() {
			while(running) {
				string message = connection.popMessage();
				async(launch::async, &Chat::parseMessage, this, message);
			}	
		}

		void init_input() {
			string input;
			while(running) {
				getline(cin, input);
				istringstream iss(input);
				string command;
				iss >> command;
				if (command == "private") {
					async(launch::async, &Chat::privateMessage, this, input);
				} else {
					connection.send(chatUser + ' ' + 
									string(connection.getListeningPort()) + 
									' ' + input);
				}
			}
		}

		void privateMessage(string input) {
			/* Expected Format:
			 * private <target> <message>
			 */

			istringstream iss(input);
			/* Separate input into tokens */
			vector<string> tokens{istream_iterator<string>{iss},
                  			      istream_iterator<string>{}};

            string target = tokens[1];
            string message;
            for (int i = 2; i < tokens.size(); ++i) message += tokens[i] + " ";

            for (auto &peer : peers ) {
            	if (get<0>(peer) == target) {
            		connection.send(message, get<1>(peer));
            	}
            }

		}

		void init_heartbeat() {
			while(running) {
				this_thread::sleep_for(chrono::seconds{HEARTBEAT_DELAY});
				connection.send(chatUser + ' ' + string(connection.getListeningPort())+ " heartbeat\n");
			}
		}

		void rememberPeer(vector<string> tokens) {
			// addr columbia 127.0.0.1 49229
			string name = tokens[1];
			string address = tokens[2];
			int port = stoi(tokens[3]);

			struct sockaddr_in peer;

			// Better name later
			struct hostent *s_host;
			s_host = gethostbyname(address.c_str());

			/* Build server address structure */
		    peer = {};     	
		    peer.sin_family = AF_INET;               
		    memcpy((char *) s_host->h_addr, // Copy given server hostname to server address
		    	   (char *)&peer.sin_addr.s_addr, s_host->h_length);
		    peer.sin_port = htons(port); 

		    peers.push_back(tuple<string, struct sockaddr_in> {name, peer});

		}

		void parseMessage (string message) {

			istringstream iss(message);
			/* Separate input into tokens */
			vector<string> tokens{istream_iterator<string>{iss},
                  			      istream_iterator<string>{}};

			if (tokens[0] == "logout") {
				stop();
			} else if (tokens[0] == "addr") {
				rememberPeer(tokens);
			} else {
				cout << message << "\n";
			}
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
	while(chat.isRunning())
		this_thread::sleep_for(chrono::seconds{10});
	// chat.stop();	

	return 0;
}