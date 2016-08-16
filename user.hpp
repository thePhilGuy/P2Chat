#ifndef USER_HPP
#define USER_HPP

#include <vector>
#include <chrono>

using namespace std;

class User {
    using address = struct sockaddr_in;
private:
    string name,
    string password,
    bool online, // why do I have this?
    chrono::high_resolution_clock::time_point lastSeen; 
    vector<string> blacklist;
    address addr;
    int logAttempts;
public:
    User(string name, string password);

    string getName();
    string getPassword();

    void login();
    void failAuth();
    bool isOnline();
    void updateLastSeen();
};

#endif // USER_HPP
