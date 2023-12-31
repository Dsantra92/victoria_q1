#include "network_utils.h"
#include "message.h"

#include <iostream>
#include <string>
#include <thread>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

bool send_request(int socket_fd, const void* request, size_t length) {
    const char* buffer = static_cast<const char*>(request);
    ssize_t bytes_sent = send(socket_fd, buffer, length, 0);
    return bytes_sent == static_cast<ssize_t>(length);
}

template<typename T>
bool send_typed_request(int socket_fd, const T& request) {
    return send_request(socket_fd, &request, sizeof(T));
}

bool isPrivateIPAddress(const in_addr& addr) {
    uint32_t ip = ntohl(addr.s_addr);

    // Check for private IP address ranges
    return ((ip >= 0x0A000000 && ip <= 0x0AFFFFFF) ||  // 10.0.0.0 - 10.255.255.255
            (ip >= 0xAC100000 && ip <= 0xAC1FFFFF) ||  // 172.16.0.0 - 172.31.255.255
            (ip >= 0xC0A80000 && ip <= 0xC0A8FFFF) ||  // 192.168.0.0 - 192.168.255.255
            (ip >= 0x7F000000 && ip <= 0x7FFFFFFF));   // 127.0.0.0 - 127.255.255.255
}

bool getIPv4Address(const std::string& hostname, in_addr& ipv4Addr) {
    addrinfo hints, *res, *p;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET; // AF_INET for IPv4
    hints.ai_socktype = SOCK_STREAM;

    int status = getaddrinfo(hostname.c_str(), nullptr, &hints, &res);
    if (status != 0) {
        std::cerr << "[-] Failed to resolve hostname: " << hostname << ": " << gai_strerror(status) << '\n';
        return false;
    }

    for (p = res; p != nullptr; p = p->ai_next) {
        // Since we're only looking for IPv4, we can copy the first public (non-fallback) IPv4 address we find
        sockaddr_in* ipv4 = (sockaddr_in *)p->ai_addr;
        ipv4Addr = ipv4->sin_addr;
        if (!isPrivateIPAddress(ipv4->sin_addr)) {
            ipv4Addr = ipv4->sin_addr; // Copy the public address
            freeaddrinfo(res); // Free the addrinfo structure
            return true;
        }
    }

    freeaddrinfo(res);
    return false;
}

void handle_server_logout(char* response_buffer){
    auto logout_response = reinterpret_cast<LogoutResponse*>(response_buffer);
    if (logout_response->Reason[0] != '\0') { // Assuming a non-empty reason means successful logout
        std::cout<< "Logout from server: Reason: " << logout_response->Reason << std::endl;
    } else {
        std::cerr << "Logout failed with no reason provided." << std::endl;
    }
}

bool checksum_is_correct(char* response_buffer, uint32_t size){
    // get the checksum from the response
    uint16_t received_checksum = static_cast<uint16_t>(
        (static_cast<unsigned char>(response_buffer[12]) << 8) |
        static_cast<unsigned char>(response_buffer[11])
    );
    response_buffer[11] = 0; // Reset checksum to zero before calculation
    response_buffer[12] = 0; // Reset checksum to zero before calculation
    uint16_t computed_checksum = checksum16(reinterpret_cast<const uint8_t*>(response_buffer), size);
    return received_checksum == computed_checksum;
}

int initialize_socket(const std::string& hostname, int port) {
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd == -1) {
        std::cerr << "[-] Failed to create a socket. Exiting!\n";
        return -1;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port); // Host-to-network short for endian conversion
    getIPv4Address(hostname.c_str(), server_addr.sin_addr);

    if (connect(socket_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        close(socket_fd);
        std::cerr << "[-] Can't connect to the server\n";
        return -1; // Connection failed
    }
    return socket_fd; // Socket successfully created and connected
}

bool attempt_login(int socket_fd, const std::string& email, const std::string& password) {
    LoginRequest login_request(email.c_str(), password.c_str(),
        std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()));

    char response_buffer[sizeof(LoginResponse)]; // Buffer to receive data

    if (!send_typed_request(socket_fd, login_request)) {
        std::cerr << "[-] Socket failed to send login request. Error: " << strerror(errno) << std::endl;
        return false;
    }

    while (true) {
        ssize_t received_bytes = recv(socket_fd, response_buffer, sizeof(LoginResponse), 0);
        if (received_bytes < 0) {
            std::cerr << "[-] Socket recv failed with error: " << strerror(errno) << std::endl;
            return false;
        } else if (received_bytes == 0) {  // Connection has been closed gracefully
            std::cout << "[-] Connection closed by the server." << std::endl;
            return false;
        }

        if (checksum_is_correct(response_buffer, received_bytes)) {
            break; // Exit the loop if checksum is correct
        } else {
            std::cerr << "[-] Received message with incorrect checksum. Waiting for another message." << std::endl;
            // Just drop the message and wait for another one, as per instructions.
        }
    }

    if (response_buffer[0] == 'G') {
        handle_server_logout(response_buffer); // should exit here
        return false;
    }

    LoginResponse* login_response = reinterpret_cast<LoginResponse*>(response_buffer);

    if (login_response->Code == 'Y') {
        return true;
    }
    return false;
}

bool attempt_submission(int socket_fd, const std::string& name, const std::string& email, const std::string& repo) {
    // Create a submission request
    SubmissionRequest submission_request(name.c_str(), email.c_str(), repo.c_str(),
        std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()));

    char response_buffer[sizeof(SubmissionResponse)]; // Buffer to receive data

    if (!send_typed_request(socket_fd, submission_request)) {
        std::cerr << "[-] Socket failed to send submission request. Error: " << strerror(errno) << std::endl;
        return false;
    }

    while (true) {
        // Attempt to receive the submission response
        ssize_t received_bytes = recv(socket_fd, response_buffer, sizeof(SubmissionResponse), 0);
        if (received_bytes < 0) {
            std::cerr << "[-] Socket recv failed with error: " << strerror(errno) << std::endl;
            return false;
        } else if (received_bytes == 0) { // Connection has been closed gracefully
            std::cout << "[-] Connection closed by the server." << std::endl;
            return false;
        }

        if (checksum_is_correct(response_buffer, received_bytes)) {
            break; // Exit the loop if checksum is correct
        } else {
            std::cerr << "[-] Received message with incorrect checksum. Waiting for another message." << std::endl;
            // Just drop the message and wait for another one, as per instructions.
        }
    }

    if (response_buffer[0] == 'G'){
        handle_server_logout(response_buffer); // should exit here
        return false;
    }

    // Construct SubmissionResponse object from the buffer using placement new
    SubmissionResponse* submission_response = reinterpret_cast<SubmissionResponse*>(response_buffer);

    // Check the response Token to determine if submission was successful
    if (submission_response->Token[0] != '\0') { // Assuming non-empty Token signifies success
        std::cout << "[+] Submission successful. Token: " << submission_response->Token << std::endl;
        return true;
    }
    return false;
}

bool attempt_logout(int socket_fd) {
    // Create a logout request
    LogoutRequest logout_request(std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::system_clock::now().time_since_epoch()));

    // Buffer to hold the response
    char response_buffer[sizeof(LogoutResponse)];

    // Send the logout request
    if (!send_typed_request(socket_fd, logout_request)) {
        std::cerr << "[-] Socket failed to send logout request. Error: " << strerror(errno) << std::endl;
        return false;
    }

    while (true) {
        // Attempt to receive the logout response
        ssize_t received_bytes = recv(socket_fd, response_buffer, sizeof(response_buffer), 0);
        if (received_bytes < 0) {
            std::cerr << "[-] Socket recv failed with error: " << strerror(errno) << std::endl;
            return false;
        } else if (received_bytes == 0) {
            std::cout << "[-] Connection closed by the server." << std::endl;
            return false; // Connection has been closed gracefully
        }

        if (checksum_is_correct(response_buffer, received_bytes)) {
            break; // Exit the loop if checksum is correct
        } else {
            std::cerr << "[-] Received message with incorrect checksum. Waiting for another message." << std::endl;
            // Just drop the message and wait for another one, as per instructions.
        }
    }

    // Construct LogoutResponse object from the buffer
    LogoutResponse* logout_response = reinterpret_cast<LogoutResponse*>(response_buffer);

    // Check the reason for logout
    if (logout_response->Reason[0] != '\0') { // Assuming a non-empty reason means successful logout
        std::cout << "[+] Logout successful. Reason: " << logout_response->Reason << std::endl;
        return true;
    }
    return false;
}