#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/select.h>

#define HOSTNAME "127.0.0.1"
#define PORT 3000
#define BUFFER_SIZE 1024

typedef struct Client {
    int sockfd;
    char *nickname;
    struct Client *next;
} Client;

Client *clients = NULL;

void broadcast(int sender_fd, const char *message) {
    Client *curr = clients;
    while (curr) {
        if (curr->sockfd != sender_fd) {
            if (send(curr->sockfd, message, strlen(message), 0) < 0) {
                perror("send");
            }
        }
        curr = curr->next;
    }
}

Client* find_client_by_socket(int sockfd) {
    Client *curr = clients;
    while (curr) {
        if (curr->sockfd == sockfd)
            return curr;
        curr = curr->next;
    }
    return NULL;
}

Client* find_client_by_nickname(const char *nickname) {
    Client *curr = clients;
    while (curr) {
        if (curr->nickname && strcmp(curr->nickname, nickname) == 0)
            return curr;
        curr = curr->next;
    }
    return NULL;
}

void welcome_user(int sockfd) {
    size_t total_size = 1;
    int userCount = 0;
    Client *curr = clients;

    while (curr) {
        if (curr->sockfd != sockfd && curr->nickname) {
            total_size += strlen(curr->nickname) + 2;
        }
        userCount++;
        curr = curr->next;
    }

    char *online_users = malloc(total_size);
    if (!online_users) {
        perror("malloc");
        return;
    }

    online_users[0] = '\0';

    curr = clients;
    while (curr) {
        if (curr->sockfd != sockfd && curr->nickname) {
            if (strlen(online_users) > 0)
                strcat(online_users, ", ");
            strcat(online_users, curr->nickname);
        }
        curr = curr->next;
    }
    
    int needed_size = snprintf(NULL, 0,
        userCount == 1 ? 
        "🎉 Welcome! You are the only user here.\r\n" : 
        "🎉 Welcome! There are %d users online.\r\n👥 Online users: %s\r\n",
        userCount, online_users);

    char *welcome_msg = malloc(needed_size + 1);
    if (!welcome_msg) {
        perror("malloc");
        free(online_users);
        return;
    }
    snprintf(welcome_msg, needed_size + 1,
        userCount == 1 ? 
        "🎉 Welcome! You are the only user here.\r\n" : 
        "🎉 Welcome! There are %d users online.\r\n👥 Online users: %s\r\n",
        userCount, online_users);
    if (send(sockfd, welcome_msg, strlen(welcome_msg), 0) < 0) {
        perror("send");
    }
    free(online_users);
    free(welcome_msg);
}

void register_user(int sockfd, const char *nickname) {
    if (find_client_by_nickname(nickname) != NULL) {
        const char *err_msg = "❌ Error: Nickname already taken, choose another:\r\n> ";
        send(sockfd, err_msg, strlen(err_msg), 0);
        return;
    }
    
    Client *new_client = malloc(sizeof(Client));
    if (!new_client) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    new_client->sockfd = sockfd;
    new_client->nickname = strdup(nickname);
    if (!new_client->nickname) {
        perror("strdup");
        free(new_client);
        exit(EXIT_FAILURE);
    }
    new_client->next = clients;
    clients = new_client;
    
    printf("👤 User registered: %s\n", nickname);
    welcome_user(sockfd);
}

void remove_client(int sockfd) {
    Client **curr = &clients;
    while (*curr) {
        if ((*curr)->sockfd == sockfd) {
            Client *to_remove = *curr;
            *curr = (*curr)->next;
            printf("❌ User '%s' disconnected.\n", to_remove->nickname ? to_remove->nickname : "unknown");
            char msg[BUFFER_SIZE];
            snprintf(msg, sizeof(msg), "👋 User '%s' left the chat.\r\n", 
                     to_remove->nickname ? to_remove->nickname : "unknown");
            broadcast(sockfd, msg);
            free(to_remove->nickname);
            free(to_remove);
            return;
        }
        curr = &((*curr)->next);
    }
}

void handle_data(int sockfd) {
    char buffer[BUFFER_SIZE];
    int nbytes = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
    if (nbytes <= 0) {
        if (nbytes == 0) {
            printf("Connection closed by client (fd=%d).\n", sockfd);
        } else {
            perror("recv");
        }
        remove_client(sockfd);
        close(sockfd);
        return;
    }

    buffer[nbytes] = '\0';
    for (int i = 0; i < nbytes; i++) {
        if (buffer[i] == '\r' || buffer[i] == '\n') {
            buffer[i] = '\0';
            break;
        }
    }

    if (strlen(buffer) == 0)
        return;

    Client *client = find_client_by_socket(sockfd);
    if (client == NULL) {
        register_user(sockfd, buffer);
    } else {
        int needed_size = snprintf(NULL, 0, "💬 %s: %s\r\n", client->nickname, buffer);
        char *msg = malloc(needed_size + 1);
        if (!msg) {
            perror("malloc");
            return;
        }
        snprintf(msg, needed_size + 1, "💬 %s: %s\r\n", client->nickname, buffer);
        printf("📢 %s: %s\n", client->nickname, buffer);
    }
}

int main() {
    int listener, newfd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addrlen;
    fd_set master_set, read_fds;
    int fdmax;

    if ((listener = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    
    int yes = 1;
    if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) < 0) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(HOSTNAME);
    server_addr.sin_port = htons(PORT);
    memset(&(server_addr.sin_zero), '\0', 8);
    
    if (bind(listener, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        exit(EXIT_FAILURE);
    }
    
    if (listen(listener, 10) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    
    printf("🚀 TCP Chat Server is running on %s:%d\n", HOSTNAME, PORT);
    
    FD_ZERO(&master_set);
    FD_SET(listener, &master_set);
    fdmax = listener;
    
    while (1) {
        read_fds = master_set;
        if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1) {
            perror("select");
            exit(EXIT_FAILURE);
        }
        
        for (int i = 0; i <= fdmax; i++) {
            if (FD_ISSET(i, &read_fds)) {
                if (i == listener) {
                    addrlen = sizeof(client_addr);
                    if ((newfd = accept(listener, (struct sockaddr *)&client_addr, &addrlen)) == -1) {
                        perror("accept");
                    } else {
                        FD_SET(newfd, &master_set);
                        if (newfd > fdmax)
                            fdmax = newfd;
                        printf("✅ Client connected: %s\n", inet_ntoa(client_addr.sin_addr));
                        const char *prompt = "👋 Welcome! Enter your nickname:\r\n> ";
                        if (send(newfd, prompt, strlen(prompt), 0) < 0)
                            perror("send");
                    }
                } else {
                    handle_data(i);
                }
            }
        }
    }
    
    Client *curr = clients;
    while (curr) {
        Client *next = curr->next;
        free(curr->nickname);
        free(curr);
        curr = next;
    }
    
    close(listener);
    return 0;
}
