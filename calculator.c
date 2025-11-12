// Define the minimum Windows version to be Windows Vista for API compatibility.
// This must be defined before including winsock2.h to expose InetPtonA/InetNtopA.
#ifdef _WIN32_WINNT
#undef _WIN32_WINNT
#endif
#define _WIN32_WINNT 0x0600
// The Winsock2 header must be included before any standard C library headers.
#include <winsock2.h>
#include <ws2tcpip.h> // For InetPtonA and InetNtopA

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h> // For uint32_t

// Link with the Ws2_32.lib library on Windows (for MSVC, ignored by GCC)
#pragma comment(lib, "Ws2_32.lib")

// A structure to hold all the calculated subnet information.
typedef struct {
    char network_address[16];
    char broadcast_address[16];
    char first_host[16];
    char last_host[16];
    char subnet_mask[16];
    long long total_hosts;
    long long usable_hosts;
    char ip_class[3];
    char ip_type[8]; // "Public" or "Private"
    char ip_binary[40];
    char mask_binary[40];
} SubnetInfo;

// Function prototypes to avoid implicit declaration warnings
void ip_to_string(uint32_t ip_n, char *str_buf);
void get_ip_class(uint32_t ip_addr, char *class_str);
void get_ip_type(uint32_t ip_addr_host_order, char *type_str);

// Converts a 32-bit integer IP address to its dotted-binary string representation.
void ip_to_binary_string(uint32_t ip_host_order, char *bin_str) {
    char temp_str[40] = {0};
    int pos = 0;
    for (int i = 3; i >= 0; i--) {
        unsigned char octet = (ip_host_order >> (i * 8)) & 0xFF;
        for (int j = 7; j >= 0; j--) {
            temp_str[pos++] = ((octet >> j) & 1) ? '1' : '0';
        }
        if (i > 0) {
            temp_str[pos++] = '.';
        }
    }
    strcpy(bin_str, temp_str);
}

// Converts a 32-bit integer IP address to its string representation (e.g., "192.168.1.1").
void ip_to_string(uint32_t ip_n, char *str_buf) {
    struct in_addr addr;
    addr.s_addr = ip_n;
    InetNtopA(AF_INET, &addr, str_buf, 16);
}

// Determines the class (A, B, C, etc.) of an IP address based on its first octet.
void get_ip_class(uint32_t ip_addr, char *class_str) {
    // Convert from network byte order to host byte order to easily check the first octet.
    unsigned char first_octet = (ntohl(ip_addr) >> 24) & 0xFF;
    if (first_octet >= 1 && first_octet <= 126) strcpy(class_str, "A");
    else if (first_octet >= 128 && first_octet <= 191) strcpy(class_str, "B");
    else if (first_octet >= 192 && first_octet <= 223) strcpy(class_str, "C");
    else if (first_octet >= 224 && first_octet <= 239) strcpy(class_str, "D");
    else if (first_octet >= 240 && first_octet <= 255) strcpy(class_str, "E");
    else strcpy(class_str, "N/A"); // For 0.x.x.x or 127.x.x.x
}

// Determines if an IP address is in a private range (RFC 1918).
void get_ip_type(uint32_t ip_addr_host_order, char *type_str) {
    unsigned char o1 = (ip_addr_host_order >> 24) & 0xFF;
    unsigned char o2 = (ip_addr_host_order >> 16) & 0xFF;

    if (o1 == 10 ||
        (o1 == 172 && (o2 >= 16 && o2 <= 31)) ||
        (o1 == 192 && o2 == 168)) {
        strcpy(type_str, "Private");
    } else {
        strcpy(type_str, "Public");
    }
}

int main(void) {
    // Initialize Winsock
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    // The CGI standard requires this HTTP header to be printed first.
    printf("Content-Type: application/json\n\n");

    char ip_str[16] = {0};
    int cidr = -1;

    // Read the POST data sent from the web form.
    char *content_length_str = getenv("CONTENT_LENGTH");
    if (content_length_str != NULL) {
        int len = atoi(content_length_str);
        if (len > 0 && len < 256) { // Basic security check
            char post_data[256];
            fgets(post_data, len + 1, stdin);
            // Parse the data from the format "ip=x.x.x.x&cidr=y"
            sscanf(post_data, "ip=%15[^&]&cidr=%d", ip_str, &cidr);
        }
    }

    // Validate the parsed input.
    if (strlen(ip_str) == 0 || cidr < 0 || cidr > 32) {
        printf("{\"error\": \"Invalid input. Please provide a valid IP and CIDR (0-32).\"}");
        WSACleanup();
        return 1;
    }

    struct in_addr ip_addr_struct;
    if (InetPtonA(AF_INET, ip_str, &ip_addr_struct) != 1) {
        printf("{\"error\": \"Invalid IP address format.\"}");
        WSACleanup();
        return 1;
    }

    // Perform the subnet calculations.
    uint32_t ip_addr = ip_addr_struct.s_addr; // Already in network byte order
    uint32_t mask = (cidr == 0) ? 0 : htonl(~(0xFFFFFFFF >> cidr));

    uint32_t network_addr = ip_addr & mask;
    uint32_t broadcast_addr = network_addr | ~mask;

    SubnetInfo info;

    // Populate the results structure.
    ip_to_string(network_addr, info.network_address);
    ip_to_string(broadcast_addr, info.broadcast_address);
    ip_to_string(mask, info.subnet_mask);

    // Handle host ranges based on CIDR.
    if (cidr <= 30) {
        ip_to_string(htonl(ntohl(network_addr) + 1), info.first_host);
        ip_to_string(htonl(ntohl(broadcast_addr) - 1), info.last_host);
        info.usable_hosts = (long long)pow(2, 32 - cidr) - 2;
    } else if (cidr == 31) { // Special case for point-to-point links (RFC 3021)
        strcpy(info.first_host, info.network_address);
        strcpy(info.last_host, info.broadcast_address);
        info.usable_hosts = 2;
    } else { // /32 has no usable hosts in the traditional sense.
        strcpy(info.first_host, "N/A");
        strcpy(info.last_host, "N/A");
        info.usable_hosts = 0;
    }

    info.total_hosts = (long long)pow(2, 32 - cidr);
    get_ip_class(ip_addr, info.ip_class);
    get_ip_type(ntohl(ip_addr), info.ip_type);
    
    ip_to_binary_string(ntohl(ip_addr), info.ip_binary);
    ip_to_binary_string(ntohl(mask), info.mask_binary);

    // Print the final results as a JSON object.
    printf("{\n");
    printf("  \"network_address\": \"%s\",\n", info.network_address);
    printf("  \"broadcast_address\": \"%s\",\n", info.broadcast_address);
    printf("  \"first_host\": \"%s\",\n", info.first_host);
    printf("  \"last_host\": \"%s\",\n", info.last_host);
    printf("  \"subnet_mask\": \"%s\",\n", info.subnet_mask);
    printf("  \"total_hosts\": %lld,\n", info.total_hosts);
    printf("  \"usable_hosts\": %lld,\n", info.usable_hosts);
    printf("  \"ip_class\": \"%s\",\n", info.ip_class);
    printf("  \"ip_type\": \"%s\",\n", info.ip_type);
    printf("  \"ip_binary\": \"%s\",\n", info.ip_binary);
    printf("  \"mask_binary\": \"%s\"\n", info.mask_binary);
    printf("}\n");

    // Cleanup Winsock
    WSACleanup();

    return 0;
}
