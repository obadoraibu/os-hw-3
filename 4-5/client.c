#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 1024

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <ip_address> <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    int client_fd;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    int port = atoi(argv[2]);
    
    // Create a socket
    client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }
    
    // Prepare the server address structure
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    
    if (inet_pton(AF_INET, argv[1], &server_addr.sin_addr) <= 0) {
        perror("Invalid address");
        exit(EXIT_FAILURE);
    }
    
    // Connect to the server
    if (connect(client_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }
    
    printf("Connected to server!\n");
    
    while (1) {
        // Write message to server
        printf("Go to department number: ");
        fgets(buffer, BUFFER_SIZE, stdin);
        if (write(client_fd, buffer, strlen(buffer)) < 0) {
            perror("Writing to socket failed");
            exit(EXIT_FAILURE);
        }
        
        // Read server response
        memset(buffer, 0, BUFFER_SIZE);
        if (read(client_fd, buffer, BUFFER_SIZE) <= 0) {
            perror("Reading from socket failed");
            exit(EXIT_FAILURE);
        }
        
        printf("%s", buffer);
        
        if (strcmp(buffer, "exit\n") == 0)
            break;

        // Read server response
        memset(buffer, 0, BUFFER_SIZE);
        if (read(client_fd, buffer, BUFFER_SIZE) <= 0) {
            perror("Reading from socket failed");
            exit(EXIT_FAILURE);
        }
        
        printf("%s", buffer);
        
        if (strcmp(buffer, "exit\n") == 0)
            break;

    }
    
    // Close the socket
    close(client_fd);
    
    return 0;
}