#include "network_utils.h"

#include <iostream>
#include <string>
#include <unistd.h>

void print_usage(const char* progName) {
    std::cerr << "Usage: " << progName << " [options]\n"
              << "Options:\n"
              << "  -e, --email     Email address\n"
              << "  -p, --password  Password (WARNING: insecure)\n"
              << "  -n, --name      Full name (enclose in quotes if it includes spaces)\n"
              << "  -g, --ghrepo    GitHub repository url\n"
              << "Example:\n"
              << "  " << progName << " -e user@example.com -p password123 -n \"John Doe\" -g rep-url\n";
}

int main(int argc, char *argv[]) {
    int port = 9009;
    std::string hostname = "challenge1.vitorian.com";
    std::string email;
    std::string password;
    std::string name;
    std::string gh_repo;
    int opt;

    while ((opt = getopt(argc, argv, "e:p:n:g:")) != -1) {
        switch (opt) {
            case 'e':
                email = optarg;
                break;
            case 'p':
                password = optarg;
                break;
            case 'n':
                name = optarg;
                break;
            case 'g':
                gh_repo = optarg;
                break;
            default: /* '?' */
                print_usage(argv[0]);
                return EXIT_FAILURE;
        }
    }

    if (email.empty() || password.empty() || name.empty() || gh_repo.empty()) {
        std::cerr << "All options are required.\n";
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    // At this point, all arguments have been captured.
    std::cout << "Email: " << email << std::endl;
    std::cout << "Password: " << password << std::endl;
    std::cout << "Name: " << name << std::endl;
    std::cout << "GitHub Repo: " << gh_repo << std::endl;

    int socket_fd = initialize_socket(hostname, port);
    if (socket_fd == -1) {
        std::cerr << "[-] Failed to create a socket. Exiting!\n";
        close(socket_fd);
        return EXIT_FAILURE;
    }
    bool login_success = attempt_login(socket_fd, email, password);
    if (login_success){
        std::cout << "[+] Login successful." << std::endl;
        bool submission_success = attempt_submission(socket_fd, name, email, gh_repo);
        if (!submission_success){
            std::cout << "[-] Submission failed." << std::endl;
            close(socket_fd);
            return EXIT_FAILURE;
        } 
        bool logout_success = attempt_logout(socket_fd);
        if (!logout_success){
            std::cout << "[-] Logout failed." << std::endl;
            close(socket_fd);
            return EXIT_FAILURE;
        }
    }
    else{
        std::cerr << "[-] Login failed" << std::endl;
        close(socket_fd);
        return EXIT_FAILURE;
    }
    return 0;
}