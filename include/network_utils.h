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
bool attempt_login(int socket_fd, const std::string& email, const std::string& password);

// Declaration of the function that attempts a submission
bool attempt_submission(int socket_fd, const std::string& name, const std::string& email, const std::string& repo);

// Declaration of the function that logs out of the system
bool attempt_logout(int socket_fd);

#endif // NETWORK_UTILS_H