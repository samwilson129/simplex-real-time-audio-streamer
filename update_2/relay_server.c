#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>

#define SERVER_PORT 12345
#define BUFFER_SIZE 1024

int sender_sockfd, receiver_sockfd;
struct sockaddr_in senderAddr, receiverAddr;
socklen_t addrLen = sizeof(struct sockaddr_in);

void *handleSender(void *arg) {
    char buffer[BUFFER_SIZE];
    while (1) {
        ssize_t receivedBytes = recv(sender_sockfd, buffer, sizeof(buffer), 0);
        if (receivedBytes <= 0) {
            perror("recv from sender");
            close(sender_sockfd);
            return NULL;
        }
        
        ssize_t sentBytes = send(receiver_sockfd, buffer, receivedBytes, 0);
        if (sentBytes < 0) {
            perror("send to receiver");
            close(receiver_sockfd);
            return NULL;
        }
    }
    return NULL;
}

int main() {
    int server_sockfd;
    pthread_t senderThread;

    server_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sockfd < 0) {
        perror("socket");
        return 1;
    }

    int opt = 1;
    if (setsockopt(server_sockfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        return 1;
    }

    memset(&senderAddr, 0, sizeof(senderAddr));
    senderAddr.sin_family = AF_INET;
    senderAddr.sin_port = htons(SERVER_PORT);
    senderAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_sockfd, (struct sockaddr *)&senderAddr, sizeof(senderAddr)) < 0) {
        perror("bind");
        return 1;
    }

    if (listen(server_sockfd, 2) < 0) {
        perror("listen");
        return 1;
    }

    printf("Waiting for sender to connect...\n");

    sender_sockfd = accept(server_sockfd, (struct sockaddr *)&senderAddr, &addrLen);
    if (sender_sockfd < 0) {
        perror("accept sender");
        return 1;
    }
    printf("Sender connected.\n");

    printf("Waiting for receiver to connect...\n");

    receiver_sockfd = accept(server_sockfd, (struct sockaddr *)&receiverAddr, &addrLen);
    if (receiver_sockfd < 0) {
        perror("accept receiver");
        return 1;
    }
    printf("Receiver connected.\n");

    if (pthread_create(&senderThread, NULL, handleSender, NULL) != 0) {
        perror("pthread_create");
        return 1;
    }

    pthread_join(senderThread, NULL);

    close(sender_sockfd);
    close(receiver_sockfd);
    close(server_sockfd);

    return 0;
}

