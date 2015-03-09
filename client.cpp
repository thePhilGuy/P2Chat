#include <iostream>
#include <cstdlib>
#include <thread>
#include <atomic>
#include <chrono>

using namespace std;

class Client {
	public:
		Client(string ip, int port) : ip{ip}, port{port}, m_stop{false} {
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
		string ip;
		int port;
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