#ifndef MESSAGECENTER_HPP
#define MESSAGECENTER_HPP

#include "transceiver.hpp"
#include "user.hpp"

class MessageCenter {
    using address = struct sockaddr_in;
public:
    MessageCenter(int port, string credentials);
    void start();
    void stop();
private:
    Transceiver connection;
    thread messageConsumer;
    atomic<bool> running;
    vector<User> users;

    /// Initializes asynchronous message handling from queue
    void init_consumption();
    /// Authenticate message sender
    bool authenticate(tuple<string, address> message);
    /// Dispatch message to appropraite handling function
    void parseMessage(tuple<string, address> message);
    /// Send requested address to asker peer
    void givePeerAddress(vector<string> tokens, address asker); /* formerly getaddress */
    void givePeerAddress(string asker_username, string peer_username, address asker_addr);
    /// Forward message to peer
    void sendMesssage(vector<string> tokens, address sender); /* formerly message */
    void sendMessage(string sender_username, string recipient, string text, address sender_addr);
    /// Forward list of reachable users -- I want a better name for this
    void online(vector<string> tokens, address asker);
    void online(string asker_username, address asker_addr); // Username to check if asker is blacklisted
    /// Broadcast message to all reachable users
    void broadcastMessage(vector<string> tokens, address sender); /* formerly broadcast */
    void broadcastMessage(string sender_username, string text, address sender_addr);
    /// Blacklist recipient from reachable list for sender
    void blacklist(vector<string> tokens, address addr); /* formerly block */
    void blacklist(string sender_username, string blacklist_username, address sender_addr);
    /// Whitelist recipient from reachable list for sender
    void whitelist(vector<string> tokens, address addr); /* formerly unblock */
    void whitelist(string sender_username, string whitelist_username, address sender_addr);
    /// Log user out from message center
    void logout(vector<string> tokens, address addr);
    void logout(string username, address addr);
protected:
};

#endif // MESSAGECENTER_HPP
