#include"socket.h"


typedef struct 
{
    int sockfd;
    char username[50];
} User;

User *users[MAX_USERS];
int user_count = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void broadcast_message(char *message, int sender_sock) {
    pthread_mutex_lock(&mutex);
    for (int i = 0; i < user_count; i++) {
        if (users[i]->sockfd != sender_sock) {
            send(users[i]->sockfd, message, strlen(message), 0);
        }
    }
    pthread_mutex_unlock(&mutex);
}

// Function to authenticate users
int authenticate_user(int client_sock, char *username) {
    char password[50], buffer[MAX_BUFFER];
    FILE *user_db = fopen("users.txt", "r");
    if (!user_db) {
        perror("Error opening users.txt");
        return 0;
    }

    send(client_sock, "Enter username: ", 17, 0);
    recv(client_sock, username, 50, 0);
    username[strcspn(username, "\n")] = '\0'; // Remove newline

    send(client_sock, "Enter password: ", 17, 0);
    recv(client_sock, password, 50, 0);
    password[strcspn(password, "\n")] = '\0';

    char db_username[50], db_password[50];
    int authenticated = 0;
    while (fscanf(user_db, "%s %s", db_username, db_password) != EOF) {
        if (strcmp(username, db_username) == 0 && strcmp(password, db_password) == 0) {
            authenticated = 1;
            break;
        }
    }
    fclose(user_db);

    if (authenticated) {
        send(client_sock, "Login successful\n", 17, 0);
        return 1;
    } else {
        send(client_sock, "Invalid username or password\n", 30, 0);
        return 0;
    }
}

// Function to handle a single client
void *handle_client(void *arg) {
    int client_sock = *(int *)arg;
    char buffer[MAX_BUFFER], username[50];

    if (!authenticate_user(client_sock, username)) {
        close(client_sock);
        return NULL;
    }

    pthread_mutex_lock(&mutex);
    User *new_user = (User *)malloc(sizeof(User));
    new_user->sockfd = client_sock;
    strcpy(new_user->username, username);
    users[user_count++] = new_user;
    pthread_mutex_unlock(&mutex);

    char welcome_msg[MAX_BUFFER];
    snprintf(welcome_msg, sizeof(welcome_msg), "%s joined the chat\n", username);
    broadcast_message(welcome_msg, client_sock);

    while (1) {
        int n = recv(client_sock, buffer, MAX_BUFFER, 0);
        if (n <= 0) break;
        buffer[n] = '\0';
        if (strcmp(buffer, "exit\n") == 0) break;

        char message[MAX_BUFFER];
        snprintf(message, sizeof(message), "%s: %s", username, buffer);
        broadcast_message(message, client_sock);
    }

    close(client_sock);
    pthread_mutex_lock(&mutex);
    for (int i = 0; i < user_count; i++) {
        if (users[i]->sockfd == client_sock) {
            free(users[i]);
            for (int j = i; j < user_count - 1; j++) {
                users[j] = users[j + 1];
            }
            user_count--;
            break;
        }
    }
    pthread_mutex_unlock(&mutex);

    snprintf(buffer, sizeof(buffer), "%s left the chat\n", username);
    broadcast_message(buffer, -1);
    return NULL;
}

int main() {
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr));
    listen(server_sock, 5);

    printf("Server started on port %d...\n", PORT);

    while (1) {
        client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_addr_len);
        pthread_t client_thread;
        pthread_create(&client_thread, NULL, handle_client, (void *)&client_sock);
    }

    close(server_sock);
    return 0;
}

