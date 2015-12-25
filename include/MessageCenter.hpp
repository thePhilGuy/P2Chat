#ifndef MESSAGECENTER_HPP
#define MESSAGECENTER_HPP

#include "transceiver.hpp"

class MessageCenter {
    using address = struct sockaddr_in;
public:
    MessageCenter(int port, string credentials);
    void start();
    void stop();
private:
    /// Initializes asynchronous message handling from queue
    void init_consumption();
    /// Authenticate message sender
    bool authenticate(tuple<string, address> message);
    /// Dispatch message to appropraite handling function
    void parseMessage(tuple<string, address> message);
    /// Send requested address to asker peer
    void givePeerAddress(vector<string> tokens, address asker); /* formerly getaddress */
    /// Forward message to peer
    void sendMesssage(vector<string> tokens, address sender); /* formerly message */
protected:
};

#endif // MESSAGECENTER_HPP
