#include"socket.h"


// Function to receive messages
void *receive_messages(void *arg) {
    char buffer[MAX_BUFFER];
    while (1) {
        int n = recv(sockfd, buffer, MAX_BUFFER, 0);
        if (n <= 0) break;
        buffer[n] = '\0';
        printf("%s", buffer);
    }
    return NULL;
}

int main() {
    struct sockaddr_in server_addr;
    char buffer[MAX_BUFFER];

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        exit(1);
    }

    recv(sockfd, buffer, MAX_BUFFER, 0);
    printf("%s", buffer);
    fgets(buffer, MAX_BUFFER, stdin);
    send(sockfd, buffer, strlen(buffer), 0);

    recv(sockfd, buffer, MAX_BUFFER, 0);
    printf("%s", buffer);
    fgets(buffer, MAX_BUFFER, stdin);
    send(sockfd, buffer, strlen(buffer), 0);

    pthread_t receive_thread;
    pthread_create(&receive_thread, NULL, receive_messages, NULL);

    while (1) {
        fgets(buffer, MAX_BUFFER, stdin);
        send(sockfd, buffer, strlen(buffer), 0);
        if (strncmp(buffer, "exit", 4) == 0) break;
    }

    close(sockfd);
    return 0;
}



