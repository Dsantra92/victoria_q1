#include "network_utils.h"
#include "message.h"

#include <iostream>
#include <cstring>
#include <thread>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
 

bool getIPv4Address(const std::string& hostname, in_addr& ipv4Addr) {
    addrinfo hints, *res, *p;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET; // AF_INET for IPv4
    hints.ai_socktype = SOCK_STREAM;

    int status = getaddrinfo(hostname.c_str(), nullptr, &hints, &res);
    if (status != 0) {
        std::cerr << "getaddrinfo error: " << gai_strerror(status) << '\n';
        return false;
    }

    for (p = res; p != nullptr; p = p->ai_next) {
        // Since we're only looking for IPv4, we can copy the first IPv4 address we find
        sockaddr_in* ipv4 = (sockaddr_in *)p->ai_addr;
        ipv4Addr = ipv4->sin_addr;
        freeaddrinfo(res);
        return true;
    }

    freeaddrinfo(res);
    return false;
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
    std::cout << "[+] Connected to the server\n";

    return socket_fd; // Socket successfully created and connected
}

bool send_request(int socket_fd, const void* request, size_t length) {
    const char* buffer = static_cast<const char*>(request);
    ssize_t bytes_sent = send(socket_fd, buffer, length, 0);
    return bytes_sent == static_cast<ssize_t>(length);
}

template<typename T>
bool send_typed_request(int socket_fd, const T& request) {
    return send_request(socket_fd, &request, sizeof(T));
}

bool attempt_login(int socket_fd, std::string& email, std::string& password) {
    LoginRequest login_request = LoginRequest(email.c_str(), password.c_str(), std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()));

    char response_buffer[sizeof(LoginResponse)]; // Adjust size as needed

    for (int attempts = 0; attempts < 3; ++attempts) {
        if (!send_typed_request(socket_fd, login_request)) {
            std::cerr << "Failed to send login request." << std::endl;
            continue; // Attempt to send the login request again
        }

        // Receive the response into the buffer
        ssize_t received_bytes = recv(socket_fd, response_buffer, sizeof(LoginResponse), MSG_WAITALL);
        std::cout<<sizeof(LoginResponse)<<std::endl;
        std::cout<<received_bytes<<std::endl;
        if (received_bytes == sizeof(LoginResponse)) {
            // Use placement new to construct LoginResponse object from the buffer
            auto login_response = reinterpret_cast<LoginResponse*>(response_buffer);

            // Check the response to determine if login was successful
            if (login_response->Code == 'Y') { // Assuming 'Y' signifies a successful login
                std::cout << "Login successful." << std::endl;
                login_response->~LoginResponse();  // Call destructor for the placement new'd object
                return true;
            }
            else{
                std::cout<<"Login failed"<<std::endl;
            }
            login_response->~LoginResponse();  // Call destructor for the placement new'd object
        } else {
            std::cerr << "Failed to receive login response or partial response received." << std::endl;
        }

        // Add a delay before retrying to comply with the throttling requirement
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    std::cerr << "All login attempts failed." << std::endl;
    return false;
}


bool attempt_submission(int socket_fd, std::string& name, std::string& email, std::string& repo) {
    // Create a submission request
    SubmissionRequest submission_request(name.c_str(), email.c_str(), repo.c_str(), 
        std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()));

    // Buffer to hold the response
    char response_buffer[sizeof(SubmissionResponse)];

    for (int attempts = 0; attempts < 3; ++attempts) {
        if (!send_typed_request(socket_fd, submission_request)) {
            std::cerr << "Failed to send submission request." << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(500)); // Retry delay
            continue; // Attempt to send the submission request again
        }

        // Attempt to receive the submission response
        ssize_t received_bytes = recv(socket_fd, response_buffer, sizeof(response_buffer), MSG_WAITALL);
        if (received_bytes != sizeof(response_buffer)) {
            std::cerr << "Failed to receive submission response." << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(500)); // Retry delay
            continue; // Attempt to receive the submission response again
        }

        // Assuming response_buffer contains the SubmissionResponse in the correct format
        auto submission_response = reinterpret_cast<SubmissionResponse*>(response_buffer);

        // Check the response Token to determine if submission was successful
        if (submission_response->Token[0] != '\0') { // Assuming non-empty Token signifies success
            std::cout << "Submission successful. Token: " << submission_response->Token << std::endl;
            return true;
        }

        // Add a delay before retrying to comply with the throttling requirement
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    std::cerr << "All submission attempts failed." << std::endl;
    return false;
}