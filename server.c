#include"socket.h"


typedef struct {
    int sockfd;
    char name[50];
} User;

User *users[MAX_USERS] = {NULL};  // Initialize user array to NULL
int user_count = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void broadcast_message(char *message, int sender_sock) {
    pthread_mutex_lock(&mutex);
    for (int i = 0; i < user_count; i++) {
        if (users[i] && users[i]->sockfd != sender_sock) {
            send(users[i]->sockfd, message, strlen(message), 0);
        }
    }
    pthread_mutex_unlock(&mutex);
}

// Function to handle a single client
void *handle_client(void *arg) {
    int client_sock = *(int *)arg;
    free(arg);  // Free dynamically allocated socket memory
    char buffer[MAX_BUFFER], name[50];

    // Ask for the user's name
    send(client_sock, "Enter your name: ", 17, 0);
    int n = recv(client_sock, name, sizeof(name) - 1, 0);
    if (n <= 0) {
        close(client_sock);
        return NULL;
    }
    name[n] = '\0';
    name[strcspn(name, "\n")] = '\0';  // Remove newline

    pthread_mutex_lock(&mutex);
    if (user_count >= MAX_USERS) {
        send(client_sock, "Server full\n", 12, 0);
        close(client_sock);
        pthread_mutex_unlock(&mutex);
        return NULL;
    }

    User *new_user = (User *)malloc(sizeof(User));
    if (!new_user) {
        perror("Memory allocation failed");
        close(client_sock);
        pthread_mutex_unlock(&mutex);
        return NULL;
    }

    new_user->sockfd = client_sock;
    strcpy(new_user->name, name);
    users[user_count++] = new_user;
    pthread_mutex_unlock(&mutex);

    // Notify others that a new user joined
    snprintf(buffer, sizeof(buffer), "%s joined the chat\n", name);
    broadcast_message(buffer, client_sock);

    while (1) {
        int n = recv(client_sock, buffer, MAX_BUFFER - 1, 0);
        if (n <= 0) break;
        buffer[n] = '\0';
        if (strcmp(buffer, "exit\n") == 0) break;

        char message[MAX_BUFFER];
        snprintf(message, sizeof(message), "%s: %.900s", name, buffer);  // Prevent overflow
        broadcast_message(message, client_sock);
    }

    // Notify others that the user left
    snprintf(buffer, sizeof(buffer), "%s left the chat\n", name);
    broadcast_message(buffer, -1);

    close(client_sock);
    pthread_mutex_lock(&mutex);
    for (int i = 0; i < user_count; i++) {
        if (users[i] && users[i]->sockfd == client_sock) {
            free(users[i]);
            for (int j = i; j < user_count - 1; j++) {
                users[j] = users[j + 1];
            }
            users[user_count - 1] = NULL;
            user_count--;
            break;
        }
    }
    pthread_mutex_unlock(&mutex);
    return NULL;
}

int main() {
    int server_sock, *client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        perror("Socket creation failed");
        exit(1);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        exit(1);
    }

    if (listen(server_sock, 5) < 0) {
        perror("Listen failed");
        exit(1);
    }

    printf("Server started on port %d...\n", PORT);

    while (1) {
        client_sock = malloc(sizeof(int));
        if (!client_sock) {
            perror("Memory allocation failed");
            continue;
        }

        *client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_addr_len);
        if (*client_sock < 0) {
            free(client_sock);
            continue;
        }

        pthread_t client_thread;
        if (pthread_create(&client_thread, NULL, handle_client, (void *)client_sock) != 0) {
            perror("Thread creation failed");
            free(client_sock);
            continue;
        }
        pthread_detach(client_thread);  // Detach thread to prevent memory leaks
    }

    close(server_sock);
    return 0;
}

