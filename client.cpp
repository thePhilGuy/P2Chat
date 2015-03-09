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

#define MAXPENDING 5

using namespace std;

class Connection {
	public:
		Connection(string ip, int port) : ip{ip}, server_port{port} {
			init_listen();
			init_send();
		}

		~Connection() { // Close socket
		 }

		void init_listen() {
		    /* Initialize listening socket */
		    if ((l_socket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
		    	// BETTER ERROR HANDLING 
		        cerr << "socket() failed";
		      
		    /* Build listening address structure */
		    l_addr = {};     	
		    l_addr.sin_family = AF_INET;                
		    l_addr.sin_addr.s_addr = htonl(INADDR_ANY); 
		    l_addr.sin_port = 0; // Random port to be determined later

		    /* Bind */
		    if (bind(l_socket, (struct sockaddr *)&l_addr, sizeof(l_addr)) < 0)
		    	// BETTER ERROR HANDLING 
		        cerr << "bind() failed";

		    /* Listen to incoming connections */
		    if (listen(l_socket, MAXPENDING) < 0)
		    	// BETTER ERROR HANDLING 
		        cerr << "listen() failed";

		    /* Get client port number */
		    socklen_t len = sizeof(l_addr);
		    if (getsockname(l_socket, (struct sockaddr *)&l_addr, &len) < 0)
		    	cerr << "getsockname() failed";
		    l_port = ntohs(l_addr.sin_port);
		}

		void init_send() {
			/* Initialize connecting socket */
			if ((client_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
				// BETTER ERROR HANDLING
				cerr << "socket() failed";

			// Better name later
			struct hostent *s_host;
			s_host = gethostbyname(ip.c_str());

			/* Build server address structure */
		    server_addr = {};     	
		    server_addr.sin_family = AF_INET;               
		    memcpy((char *) s_host->h_addr, // Copy given server hostname to server address
		    	   (char *)&server_addr.sin_addr.s_addr, s_host->h_length);
		    server_addr.sin_port = htons(server_port); 
		}

		// void receive();
		void send(string message) {
			/* Connect to server */
			if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
				// BETTER ERROR HANDLING
				cerr << "connect() failed";

			if (write(client_socket, message.c_str(), message.length()) < 0)
				//BETTER ERROR HANDLING
				cerr << "write() failed";

		}

	private:
		int l_socket;
		int l_port;
		struct sockaddr_in l_addr;
		
		string ip;
		int server_port;
		int client_socket;
		struct sockaddr_in server_addr;

};

class Client {
	public:
		Client(string ip, int port) : m_stop{false}, connection{ip, port} {
			cout << "Created a client!\n";
			// synchronously identify here
		}

		void stop() {
			m_stop = true; 
			// receive_thread.join();
			input_thread.join();
			cout << "input_thread joined \n";
		}

		void start() {
			input_thread = thread(&Client::init_input, this);
			// receive_thread = thread(&Client::init_receive, this);
		}

	private:
		Connection connection;
		thread input_thread;
		// thread receive_thread;
		atomic<bool> m_stop;

		void init_receive() {
		}

		void init_input() {
			// Follow this model later for ideal behavior
			// while(!exit_flag)
			// {
			//     obtain_resource_with_timeout(a_short_while);
			//     if (resource_ready)
			//         use_resource();
			// }

			string input;
			while(!m_stop) {
				getline(cin, input);
				cout << "Input: " << input << '\n';
			}
			cout << "Exiting gracefully ! \n";
		}
};

int main(int argc, char *argv[]) {
	
	if (argc != 3) {
		cerr << "Usage: ./client <server_address> <port>\n";
		return 1;
	}

	string server_address = argv[1];
	int port = atoi(argv[2]);

	Client chat {server_address, port};
	
	chat.start();
	this_thread::sleep_for(chrono::seconds{10});
	chat.stop();	

	return 0;
}