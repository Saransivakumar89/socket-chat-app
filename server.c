#include"socket.h"


typedef struct
{
    int sockfd;
    char username[50];
} User;

User *users[MAX_USERS];
int user_count = 0;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;


void broadcast_message(char *message, int sender_sock)
{
    pthread_mutex_lock(&mutex);
    
    for (int i = 0; i < user_count; i++)
    {
        if (users[i]->sockfd != sender_sock) 
        {
            send(users[i]->sockfd, message, strlen(message), 0);
        }
    }
    pthread_mutex_unlock(&mutex);
}


int authenticate_user(int client_sock) 
{
    char username[50], password[50];
    FILE *user_db = fopen("users.txt", "r");
    
    if (user_db == NULL) 
    {
        perror("Database not found");
        return 0;
    }

    send(client_sock, "Enter username: ", 17, 0);
    recv(client_sock, username, 50, 0);

    send(client_sock, "Enter password: ", 17, 0);
    recv(client_sock, password, 50, 0);

    char db_username[50], db_password[50];
    
    int authenticated = 0;
    
    while (fscanf(user_db, "%s %s", db_username, db_password) != EOF)
    {
        if (strcmp(username, db_username) == 0 && strcmp(password, db_password) == 0)
        {
            authenticated = 1;
            break;
        }
    }
    fclose(user_db);

    if (authenticated) 
    {
        send(client_sock, "Login successful\n", 17, 0);
        return 1;
    }
    
    else 
     {
        send(client_sock, "Invalid username or password\n", 30, 0);
        return 0;
     } 
}


void *handle_client(void *arg) 
{
    int client_sock = *(int *)arg;
    char buffer[MAX_BUFFER];
    
    char welcome_msg[] = "Welcome to the chat room!";
    char user_list_msg[MAX_BUFFER];


    if (!authenticate_user(client_sock)) 
    {
        close(client_sock);
        return NULL;
    }

    pthread_mutex_lock(&mutex);
    
    users[user_count] = (User *)malloc(sizeof(User));
    users[user_count]->sockfd = client_sock;
    
    recv(client_sock, users[user_count]->username, 50, 0);
    user_count++;
    
    pthread_mutex_unlock(&mutex);

    send(client_sock, welcome_msg, sizeof(welcome_msg), 0);
    
    snprintf(user_list_msg, MAX_BUFFER, "Online users: ");
    
    for (int i = 0; i < user_count; i++) 
    {
        strcat(user_list_msg, users[i]->username);
        strcat(user_list_msg, " ");
    }
    
    send(client_sock, user_list_msg, strlen(user_list_msg), 0);

    // Listen for messages from the client
    while (1) {
        int n = recv(client_sock, buffer, MAX_BUFFER, 0);
        if (n <= 0) {
            break;
        }
        buffer[n] = '\0';
        broadcast_message(buffer, client_sock);
    }

    pthread_mutex_lock(&mutex);
    for (int i = 0; i < user_count; i++) 
    {
        if (users[i]->sockfd == client_sock)
         {
            free(users[i]);
            for (int j = i; j < user_count - 1; j++)
            {
                users[j] = users[j + 1];
            }
            user_count--;
            break;
        }
    }
    pthread_mutex_unlock(&mutex);

    close(client_sock);
    return NULL;
}

int main()
{
    int server_sock, client_sock;
    
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) 
    {
        perror("Socket creation failed");
        exit(1);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Binding failed");
        exit(1);
    }

    if (listen(server_sock, 5) < 0) 
    {
        perror("Listen failed");
        exit(1);
    }

    printf("Server listening on port %d...\n", PORT);

    while (1) 
    {
        client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_sock < 0) 
        {
            perror("Accept failed");
            continue;
        }

        pthread_t client_thread;
        pthread_create(&client_thread, NULL, handle_client, (void *)&client_sock);
    }

    close(server_sock);
    return 0;
}
