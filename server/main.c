/** References
 * - https://gi st.github.com/cellularmitosis/e4364c788dc8893b8eba76e5ad408929#file-single-threaded-c
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8989

void serve_client(int client_sock) {
    char buffer[4096];
    read(client_sock, buffer, sizeof(buffer) - 1);

    if (strstr(buffer, "GET /api/message")) {
        char *body = "Hello from C server!";
        char header[256];
        sprintf(header,
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/plain\r\n"
                "Content-Length: %ld\r\n"
                "\r\n", strlen(body));
        write(client_sock, header, strlen(header));
        write(client_sock, body, strlen(body));
    } else if (strncmp(buffer, "POST /api/calc", 14) == 0) {
        char *body = strstr(buffer, "\r\n\r\n");
        if (body) {
            body += 4; // skip header end
            printf("JSON received: %s\n", body);
            // {"text":"1+2"}
            body += 9;
            int a = *body - '0';
            int b = *(body+2) - '0';
            int answer;
            switch (*(body+1)) {
                case '+':
                    answer = a + b;
                break;
                case '-':
                    answer = a - b;
                break;
                case '*':
                    answer = a * b;
                break;
                case '/':
                    answer = a / b;
                break;
                default:
                    answer = 0;
                break;
            }

            // Return a response
            char response[1024];
            sprintf(response,  "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\n");
            sprintf(response,  "%s%d", response, answer);
            write(client_sock, response, strlen(response));
        }
    } else if (strstr(buffer, "GET /calc")){
        FILE *f = fopen("calc.html", "r");
        if (!f) {
            char *err = "HTTP/1.1 404 Not Found\r\n\r\n";
            write(client_sock, err, strlen(err));
        } else {
            fseek(f, 0, SEEK_END);
            long len = ftell(f);
            rewind(f);
            char *body = malloc(len + 1);
            fread(body, 1, len, f);
            fclose(f);
            body[len] = 0;

            char header[256];
            sprintf(header,
                    "HTTP/1.1 200 OK\r\n"
                    "Content-Type: text/html\r\n"
                    "Content-Length: %ld\r\n"
                    "\r\n", len);
            write(client_sock, header, strlen(header));
            write(client_sock, body, len);
            free(body);
        }
    } else {
        FILE *f = fopen("index.html", "r");
        if (!f) {
            char *err = "HTTP/1.1 404 Not Found\r\n\r\n";
            write(client_sock, err, strlen(err));
        } else {
            fseek(f, 0, SEEK_END);
            long len = ftell(f);
            rewind(f);
            char *body = malloc(len + 1);
            fread(body, 1, len, f);
            fclose(f);
            body[len] = 0;

            char header[256];
            sprintf(header,
                    "HTTP/1.1 200 OK\r\n"
                    "Content-Type: text/html\r\n"
                    "Content-Length: %ld\r\n"
                    "\r\n", len);
            write(client_sock, header, strlen(header));
            write(client_sock, body, len);
            free(body);
        }
    }
    close(client_sock);
}

int main() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr = {0};
    int port = 8989;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(PORT);

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    bind(server_fd, (struct sockaddr*)&addr, sizeof(addr));
    if (server_fd < 0) {
        perror("socket");
        exit(1);
    }
    listen(server_fd, 10);
    printf("Server started on http://localhost:%d\n", PORT);

    while (1) {
        int client = accept(server_fd, NULL, NULL);

        serve_client(client);
    }
}
