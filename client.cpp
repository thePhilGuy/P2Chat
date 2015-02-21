#include <iostream>
#include <boost/asio.hpp>
#include <thread>

using namespace std;

int main(int argc, char *argv[]) {
	
	if (argc != 3) {
		cerr << "Usage: ./client <server_address> <port>\n";
		return 1;
	}

	// Create boost io_service
	boost::asio::io_service io_service;

	boost::asio::ip::tcp::socket s(io_service);
	boost::asio::ip::tcp::resolver resolver(io_service);

	boost::asio::connect(s, resolver.resolve({argv[1], argv[2]}));

	cout << "Enter message: ";
	string request;
	cin.getline(request, 1024);
	auto request_length = request.length();
	boost::asio::write(s, boost::asio::buffer(request, request_length));

	cout << "Message sent\n.";
	return 0;
}