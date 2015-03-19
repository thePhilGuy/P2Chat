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

using namespace std;

class Connection {
	public:
		Connection(int port) : listeningPort{port}, running{true}, incomingQueue{} {
			init_listen();
		}

		~Connection() { // Close socket
			running = false;
			close(listeningSocket);
			accept_thread.join();
		 }

		auto popMessage() {
			unique_lock<mutex> lck{incomingMutex};
			hasIncoming.wait(lck);
			auto m = incomingQueue.front();
			incomingQueue.pop();
			lck.unlock();
			return m;
		}

		void send(string text, struct sockaddr_in addr) {

			cout << "Sending to client on port: " << ntohs(addr.sin_port) << "\n";

			int outgoingSocket;

			/* Initialize connecting socket */
			if ((outgoingSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
				// BETTER ERROR HANDLING
				cerr << "socket() failed";

			/* Connect to server */
			if (connect(outgoingSocket, (struct sockaddr *)&addr, sizeof(addr)) < 0)
				// BETTER ERROR HANDLING
				cerr << "connect() failed " << errno;

			/* Send message and close connection */
			if (write(outgoingSocket, text.c_str(), text.length()) < 0)
				//BETTER ERROR HANDLING
				cerr << "write() failed";
			::close(outgoingSocket);
		}

	private:
		int listeningSocket;
		int listeningPort;
		struct sockaddr_in listeningAddr;

		atomic<bool> running;
		thread accept_thread;

		queue<tuple<string, struct sockaddr_in>> incomingQueue;
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
		    listeningAddr.sin_port = htons(listeningPort); 

		    /* Bind */
		    if (::bind(listeningSocket, (struct sockaddr *)&listeningAddr, sizeof(listeningAddr)) < 0)
		    	// BETTER ERROR HANDLING 
		        cerr << "bind() failed";
		    cout << "Listening on port: " << listeningPort << "\n";
		    /* Listen to incoming connections */
		    if (listen(listeningSocket, MAXPENDING) < 0)
		    	// BETTER ERROR HANDLING 
		        cerr << "listen() failed";

		    accept_thread = thread(&Connection::accept_messages, this);
		}

		void accept_messages() {
			struct sockaddr_in incomingAddr;
			unsigned int incAddrLen = sizeof(incomingAddr);
			int incomingSocket;

			cout << "Accepting messages.\n";

			while (running) {
				if ((incomingSocket = accept(listeningSocket, (struct sockaddr *)&incomingAddr, &incAddrLen)) < 0)
					// BETTER ERROR HANDLING
					cerr << "accept() failed";
					cout << "accepted someone\n";
					async(launch::async, &Connection::read_message, this, incomingSocket, incomingAddr);
			}
		}

		void read_message(int sock, struct sockaddr_in addr) {
			FILE *fp = fdopen(sock, "r");
			if (fp == NULL) cerr << "fdopen() failed";

			char buffer [256];
			memset(buffer, 0, sizeof buffer);
			
			if (fgets(buffer, sizeof buffer, fp) == NULL) {
				cerr << "fgets() failed with errno: " << errno << "\n";
			}
			fclose(fp);

			unique_lock<mutex> lck {incomingMutex};
			incomingQueue.push(tuple<string, struct sockaddr_in> {string(buffer), addr});
			hasIncoming.notify_one();
		}
};

class User {
	public:
		User(string n, string p) : 
		name{n}, password{p}, online{false}, last {}, blacklist {}, addr {}, logAttempts {0} { }

		string getName() { return name; }
		string getPassword() { return password; }

		void login() {
			online = true;
			updateLast();
		}

		void failAuth() { ++ logAttempts; }

		bool isOnline() {
			return online;
		}

		void updateLast() {
			last = chrono::high_resolution_clock::now();
		}

		auto getAddr() {
			return addr;
		}

		void updateAddress(struct sockaddr_in address, int &port) {
			addr = address;
			addr.sin_port = htons(port);
		}

		void block(string target) {
			bool listed = false;
			for (auto &blacklisted : blacklist ) {
				if (blacklisted == target) {
					cout << "User already blacklisted. \n";
					listed = true;
				}
			} 
			if (!listed) blacklist.push_back(target);
		}

		void unblock(string target) {
			remove(blacklist.begin(), blacklist.end(), target);
		}

		void logout() {
			updateLast();
			online = false;
		}

	private:
		string name;
		string password;
		bool online;
		chrono::high_resolution_clock::time_point last;
		vector<string> blacklist;
		struct sockaddr_in addr;
		int logAttempts;
};

class MessageCenter {
	public:
		MessageCenter(int port, string credentials) : connection(port), running{true} {
			ifstream credentialsFile {credentials, ifstream::in};
			cout << "Opened credentials\n";
			if (credentialsFile.is_open()) {
				string name, pass;
				while (credentialsFile >> name && credentialsFile >> pass) 
					users.push_back(User {name, pass});
			}
		}

		void stop() {
			running = false;
			message_consumer.join();
		}

		void start() {
			message_consumer = thread(&MessageCenter::init_consumption, this);
		}

	private:
		Connection connection;
		thread message_consumer;
		atomic<bool> running;
		vector<User> users;

		void init_consumption() {
			while(running) {
				auto message = connection.popMessage();
				async(launch::async, &MessageCenter::parseMessage, this, message);
			}
		}

		bool authenticate(tuple<string, struct sockaddr_in> message) {
			string username;
			istringstream iss(get<0>(message));
			iss >> username;
			cout << "Authenticating " << username << "\n";
			for (auto &user : users ) {
				if (user.getName() == username) {
					if (user.isOnline()) {
						int port;
						iss >> port;
						user.updateAddress(get<1>(message), port); 
						return true; 
					} else {
						string password;
						iss >> password;
						if (user.getPassword() == password) {
							user.login();
							int port;
							iss >> port;
							user.updateAddress(get<1>(message), port); 
							return true;
						} else { 
							user.failAuth();
							return false; 
						}
					}
				}
			}

			return false;
		}

		void parseMessage(tuple<string, struct sockaddr_in> message) {

			if (authenticate(message)) {
				string text;
				struct sockaddr_in addr;
				tie (text, addr) = message;
				istringstream iss(text);

				/* Separate input into tokens */
				vector<string> tokens{istream_iterator<string>{iss},
                      			      istream_iterator<string>{}};

                for (auto &s : tokens ) cout << s << "\n";

                if (tokens[2] == "message") {
                	/* Transfer message along to target */
                	async(launch::async, &MessageCenter::message, this, tokens, addr);
                } else if (tokens[2] == "online") {
                	/* List connected users */
                	async(launch::async, &MessageCenter::online, this, tokens, addr);
                } else if (tokens[2] == "broadcast") {
                	/* Transfer message along to all connected users */
                	async(launch::async, &MessageCenter::broadcast, this, tokens, addr);
                } else if (tokens[2] == "block") {
                	/* Add target to sender's blacklist */
                	async(launch::async, &MessageCenter::block, this, tokens, addr);
                } else if (tokens[2] == "unblock") {
                	/* Remove target from sender's blacklist */
                	async(launch::async, &MessageCenter::unblock, this, tokens, addr);
                } else if (tokens[2] == "logout") {
                	/* Log out sender */
                	async(launch::async, &MessageCenter::logout, this, tokens, addr);
                } 
			} else {
				cout << "Invalid credentials\n";
			}
		}

		void message(vector<string> tokens, struct sockaddr_in addr) {
			cout << "processing message sending \n";
			/* Expected Format:
			 * <sender> <port> message <target> <text>
			 */
			string sender = tokens[0];
			string target = tokens[3];
			string text;
			text = accumulate(begin(tokens) + 4, end(tokens), text);

			for (auto &user : users) {
				if (user.getName() == target && user.isOnline()) 
					connection.send(text, user.getAddr());
			}
		}

		void online(vector<string> tokens, struct sockaddr_in addr) {
			/* Expected Format:
			 * <sender> <port> online
			 */

			string usersOnline {};

			for (auto &user : users) 
			 	if (user.isOnline()) usersOnline += user.getName();
			
			connection.send(usersOnline, addr);
		}

		void broadcast(vector<string> tokens, struct sockaddr_in addr) {
			/* Expected Format:
			 * <sender> <port> broadcast <text>
			 */
			 string sender = tokens[0];
			 string text; 
			 text = accumulate(begin(tokens) + 3, end(tokens), text);

			 for (auto &user : users) {
			 	if (user.isOnline()) connection.send(text, user.getAddr());
			 }
		}

		void block (vector<string> tokens, struct sockaddr_in addr) {
			/* Expected Format:
			 * <sender> <port> block <target>
			 */
			string sender = tokens[0];
			string target = tokens[3];

			for (auto &user : users) {
				if (user.getName() == sender) {
					user.block(target);
				}
			}
		}

		void unblock (vector<string> tokens, struct sockaddr_in addr) {
			/* Expected Format:
			 * <sender> <port> unblock <target>
			 */
			string sender = tokens[0];
			string target = tokens[3];

			for (auto &user : users) {
				if (user.getName() == sender) {
					user.unblock(target);
				}
			}
		}

		void logout (vector<string> tokens, struct sockaddr_in addr) {
			/* Expected Format:
			 * <sender> <port> logout
			 */
			string sender = tokens[0];
		
			for (auto &user : users) {
				if (user.getName() == sender) {
					user.logout();
				}
			}
		}

};	

int main(int argc, char *argv[]) {
	
	if (argc != 2) {
		cerr << "Usage: ./center <port>\n";
		return 1;
	}

	int port = atoi(argv[1]);

	MessageCenter center {port, "credentials.txt"};

	center.start();
	this_thread::sleep_for(chrono::seconds{3000});
	center.stop();

	return 0;
}