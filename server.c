#include"socket.h"

typedef struct
{
    int sockID;
    char username[50];
}User;

User *users[MAX_USERS];
int user_count = 0;


int main()
{
    int server_sock, client_sock;
    
    struct sockaddr_in server_addr, client_addr;

    server_sock = socket(AF_INET, SOCK_STREAM,0);

    socklen_t client_addr_len = sizeof(client_addr);
  
    if(server_sock < 0)
    {
        printf("Failed!!!\n");
        exit(1);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);
    
    if(bind(server_sock,(struct sockaddr *)&server_addr, sizeof(server_addr)))
    {
        printf("Failed to Bind the connection...\n");
        exit(1);
    }
    
    if(listen(server_sock,5)<0)
    {
        printf("Failed to Listen...\n");
        exit(1);
    }

    printf("Connected to the Port : %d...\n",PORT);

    client_sock = accept(server_sock,(struct sockaddr *)&client_addr, &client_addr_len);
    
    if(client_sock < 0 )
    {
        printf("Failed to Accept the connection...\n");
        exit(1);
    }

    close(server_sock);

    return 0;
}
