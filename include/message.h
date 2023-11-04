#ifndef MESSAGE_H
#define MESSAGE_H

#include <chrono>
#include <cstdint>
#include <cstring>
#include <cstddef>

// Function prototype for checksum calculation
uint16_t checksum16(const uint8_t* buf, uint32_t len);

#pragma pack(push, 1) // Ensure structures are packed with no padding

// Base message structure for TCP protocol
struct Message {
    char MsgType;        // Message Type
    uint16_t MsgLen;     // Message Length
    uint64_t Timestamp;  // Timestamp as a uint64_t representing nanoseconds
    uint16_t ChkSum;     // Checksum

    // Constructor
    Message(char type, uint16_t len, std::chrono::nanoseconds timestamp)
        : MsgType(type), MsgLen(len), Timestamp(timestamp.count()), ChkSum(0) {
        // ChkSum is calculated in derived structs
    }
};

// Inherited structure for login messages
struct LoginRequest : public Message {
    char User[64];       // User text
    char Password[32];   // Password text

    // Constructor
    LoginRequest(const char* user, const char* password, std::chrono::nanoseconds timestamp)
        : Message('L', 109, timestamp) { // 109 is the length of LoginRequest including the headers
        std::memset(User, 0, sizeof(User));
        std::memset(Password, 0, sizeof(Password));

        std::strncpy(User, user, sizeof(User) - 1);
        std::strncpy(Password, password, sizeof(Password) - 1);

        ChkSum = 0;
        // Calculate checksum for the entire message with ChkSum set to 0
        ChkSum = checksum16(reinterpret_cast<const uint8_t*>(this), sizeof(LoginRequest));
    }
};

struct LoginResponse : public Message {
    char Code;        // Response Code
    char Reason[32];  // Reason text
};

// Inherited structure for submission messages
struct SubmissionRequest : public Message {
    char Name[64];       // Name text
    char Email[64];      // Email text
    char Repo[64];       // Repository URL text

    // Constructor
    SubmissionRequest(const char* name, const char* email, const char* repo, std::chrono::nanoseconds timestamp)
        : Message('S', 205, timestamp) {
        std::memset(Name, 0, sizeof(Name));
        std::memset(Email, 0, sizeof(Email));
        std::memset(Repo, 0, sizeof(Repo));

        std::strncpy(Name, name, sizeof(Name) - 1);
        std::strncpy(Email, email, sizeof(Email) - 1);
        std::strncpy(Repo, repo, sizeof(Repo) - 1);

        ChkSum = 0;
        // Calculate checksum for the entire message with ChkSum set to 0
        ChkSum = checksum16(reinterpret_cast<const uint8_t*>(this), sizeof(SubmissionRequest));
    }
};

// Inherited structure for submission responses
struct SubmissionResponse : public Message {
    char Token[32];  // Token text
};

#pragma pack(pop) // Stop structure packing
#endif // MESSAGE_H