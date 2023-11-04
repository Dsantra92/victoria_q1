#include "network_utils.h"

#include <iostream>
#include <string>

int main() {
	int port = 9009;
    std::string hostname = "challenge1.vitorian.com";
    std::string email = "deeptendu.santra@gmail.com";
    std::string password = "pwd123";
    std::string name = "Deeptendu Santra";
    std::string gh_repo = "test repo";
    
    int socket_fd = initialize_socket(hostname, port);
    bool login_success = attempt_login(socket_fd, email, password);
    if (login_success){
        bool submission_success = attempt_submission(socket_fd, name, email, gh_repo);
    }
}
