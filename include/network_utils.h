#ifndef NETWORK_UTILS_H
#define NETWORK_UTILS_H

#include "message.h" // Include the message definitions
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

// Declaration of the function that initializes the socket
int initialize_socket(const std::string& hostname, int port);

// Declaration of the function that attempts login
bool attempt_login(int socket_fd, std::string& email, std::string& password);

// Declaration of the function that attempts a submission
bool attempt_submission(int socket_fd, std::string& name, std::string& email, std::string& repo);

#endif // NETWORK_UTILS_H