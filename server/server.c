/**
 *   - https://medium.com/@nipunweerasiri/a-simple-web-server-written-in-c-cf7445002e6
 *   - https://dev.to/jeffreythecoder/how-i-built-a-simple-http-server-from-scratch-using-c-739
 *   - https://blog.csdn.net/Z_Stand/article/details/102535706
 */

/*
 * TODO:
 * - Use thread pool to handle clients
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>

#define PORT 8989
#define BACKLOG 10

void *handle_client(void *arg) {
    int client_fd = *((int *)arg);
    printf("Hello from client\n");
    close(client_fd);
    return NULL;
}

int main(void) {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(PORT);

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        fprintf(stderr, "Error: The server is not bound to the address.\n");
        return 1;
    }

    if (listen(server_fd, BACKLOG) < 0) {
        fprintf(stderr, "Error: The server is not listening.\n");
        return 1;
    }

    printf("Server started on http://localhost:%d\n", PORT);

    while (1) {
        struct sockaddr_in client_addr = {0};
        int client_addr_len = sizeof(client_addr);
        int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);
        printf("Client address: %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        pthread_t thread_id;
        pthread_create(&thread_id, NULL, handle_client, (void *)&client_fd);
        pthread_detach(thread_id);
    }

    return 0;
}
