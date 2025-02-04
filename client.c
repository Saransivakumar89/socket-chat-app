#include "socket.h"


void *receive_messages(void *arg) {
    int sockfd = *(int *)arg;
    char buffer[MAX_BUFFER];

    while (1) {
        int n = recv(sockfd, buffer, MAX_BUFFER - 1, 0);
        if (n <= 0) {
            printf("Disconnected from server.\n");
            break;
        }
        buffer[n] = '\0';
        printf("%s", buffer);
    }
    return NULL;
}

int main() {
    struct sockaddr_in server_addr;
    char buffer[MAX_BUFFER];
    int sockfd;

    // Create socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        exit(1);
    }

    // Set server details
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // Connect to server
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        close(sockfd);
        exit(1);
    }

    // Receive and print the server's initial message
    int n = recv(sockfd, buffer, MAX_BUFFER - 1, 0);
    if (n > 0) {
        buffer[n] = '\0';
        printf("%s", buffer);
    }

    // Enter name
    fgets(buffer, MAX_BUFFER, stdin);
    send(sockfd, buffer, strlen(buffer), 0);

    // Start receiving thread
    pthread_t receive_thread;
    int *sock_ptr = malloc(sizeof(int));
    *sock_ptr = sockfd;
    if (pthread_create(&receive_thread, NULL, receive_messages, sock_ptr) != 0) {
        perror("Thread creation failed");
        free(sock_ptr);
        close(sockfd);
        exit(1);
    }
    pthread_detach(receive_thread);  // Auto cleanup

    // Sending loop
    while (1) {
        fgets(buffer, MAX_BUFFER, stdin);
        send(sockfd, buffer, strlen(buffer), 0);
        if (strncmp(buffer, "exit", 4) == 0) break;
    }

    // Cleanup
    close(sockfd);
    return 0;
}



