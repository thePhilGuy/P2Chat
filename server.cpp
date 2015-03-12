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
					async(launch::async, &Connection::read_message, this, incomingSocket, incomingAddr);
			}
		}

		void read_message(int sock, struct sockaddr_in addr) {
			FILE *fp = fdopen(sock, "r");
			if (fp == NULL) cerr << "fdopen() failed";

			char buffer [256];
			memset(buffer, 0, sizeof buffer);
			
			if (fgets(buffer, sizeof buffer, fp) == NULL) cerr << "fgets() failed";
			fclose(fp);

			unique_lock<mutex> lck {incomingMutex};
			incomingQueue.push(tuple<string, struct sockaddr_in> {string(buffer), addr});
			hasIncoming.notify_one();
		}
};

class User {
	public:
		User(string n, string p) : 
		name{n}, password{p}, online{false}, last {}, blacklist {}, addr {} { }

		string getName() { return name; }
		string getPassword() { return password; }

		void login() {
			online = true;
			updateLast();
		}

		bool isOnline() {
			return online;
		}

		void updateLast() {
			last = chrono::high_resolution_clock::now();
		}

	private:
		string name;
		string password;
		bool online;
		chrono::high_resolution_clock::time_point last;
		vector<string> blacklist;
		struct sockaddr_in addr;
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
						return true; 
					} else {
						string password;
						iss >> password;
						if (user.getPassword() == password) {
							user.login();
							return true;
						}
					}
				}
			}
			
			return false;
		}

		void parseMessage(tuple<string, struct sockaddr_in> message) {
			if (authenticate(message)) {
				cout << "user.isOnline() = " << users[0].isOnline() << "\n";

				// string text;
				// struct sockaddr_in addr;
				// tie (text, addr) = message;
				// istringstream iss(text);
			 //    copy(istream_iterator<string>(iss),
			 //         istream_iterator<string>(),
			 //         ostream_iterator<string>(cout, "\n"));
				cout << "Succes\n";
			} else {
				cout << "Invalid credentials\n";
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
	this_thread::sleep_for(chrono::seconds{20});
	center.stop();

	return 0;
}