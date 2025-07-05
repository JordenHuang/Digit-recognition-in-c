/**
 *   - https://medium.com/@nipunweerasiri/a-simple-web-server-written-in-c-cf7445002e6
 *   - https://dev.to/jeffreythecoder/how-i-built-a-simple-http-server-from-scratch-using-c-739
 *   - https://blog.csdn.net/Z_Stand/article/details/102535706
 *   - https://stackoverflow.com/questions/55598857/sockets-and-threads-in-c
 *   - https://github.com/davidleitw/socket
 */

/*
 * TODO:
 * - Close the worker threads
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <time.h>

#define NERUALIB_IMPLEMENTATION
#include "../neuralib.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../thirdparty/stb_image_write.h"

#define HTML_PAGE_NAME "drawer.html"

// #define MODEL_NAME "../models/0629_64.model"
#define MODEL_NAME "../models/0629_128_SC.model"
#define IMG_SIZE 784 // 28*28

#define PORT 8989
#define BACKLOG 10

#define THREAD_POOL_CAPACITY 5
#define CLIENT_QUEUE_CAPACITY 20

/* Global variables */
int server_fd;
pthread_t thread_pool[THREAD_POOL_CAPACITY];
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
int client_queue[CLIENT_QUEUE_CAPACITY];
// client_count will point to the next free position in client_queue;
int client_count = 0;

NeuralNet model;
char *drawer_page = NULL;
long drawer_page_lengh = -1;

/* Function portotypes */
void signal_handler(int signum);
void init_resources();
void *worker_func(void *arg);
int enqueue(int client_fd);
int dequeue(void);
int is_queue_empty();
void handle_client(int client_fd);
void predict(unsigned char *img_data, int *predict_num, float *result_probs);


int main(void) {
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
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

    // Prepare worker threads
    for (size_t i = 0; i < THREAD_POOL_CAPACITY; ++i) {
        pthread_create(&thread_pool[i], NULL, worker_func, NULL);
    }

    // Prepare signal handling
    signal(SIGINT, signal_handler);

    // Initialize the resources
    init_resources();

    printf("Server started on http://localhost:%d\n", PORT);
    printf("Press Ctrl+C to close the server\n");

    // Start accepting clients
    while (1) {
        struct sockaddr_in client_addr = {0};
        int client_addr_len = sizeof(client_addr);
        int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);
        printf("Client address: %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        pthread_mutex_lock(&mutex);
        if (enqueue(client_fd) == 1) {
            pthread_cond_signal(&cond);
        } else {
            printf("[WARNING] Exceed maximum client count: %d/%d\n", client_count, CLIENT_QUEUE_CAPACITY);
        }
        pthread_mutex_unlock(&mutex);
    }

    return 0;
}

void signal_handler(int signum) {
    // Close server
    close(server_fd);

    // Free the memory of drawer page
    free(drawer_page);

    // Free model
    nl_model_free(model);

    // Terminate the program
    exit(0);
}

void init_resources() {
    // Drawer page
    printf("Load page resource...\n");
    FILE *fptr = fopen(HTML_PAGE_NAME, "r");
    fseek(fptr, 0, SEEK_END);
    drawer_page_lengh = ftell(fptr);
    rewind(fptr);
    drawer_page = malloc(drawer_page_lengh + 1);
    fread(drawer_page, 1, drawer_page_lengh, fptr);
    fclose(fptr);
    drawer_page[drawer_page_lengh] = '\0';

    // Model
    printf("Load Model...\n");
    nl_model_load(MODEL_NAME, &model);
}

void *worker_func(void *arg) {
    while (1) {
        pthread_mutex_lock(&mutex);
        while (is_queue_empty()) {
            pthread_cond_wait(&cond, &mutex);
        }
        int client_fd = dequeue();
        pthread_mutex_unlock(&mutex);

        handle_client(client_fd);
        close(client_fd);
    }
}

int enqueue(int client_fd) {
    if (client_count < CLIENT_QUEUE_CAPACITY) {
        client_queue[client_count++] = client_fd;
        return 1;
    }
    // TODO:
    // If queue is full, send 503 and close client_fd
    return 0;
}

int dequeue(void) {
    if (!is_queue_empty()) {
        client_count -= 1;
        return client_queue[client_count];
    }
}

int is_queue_empty() {
    return (client_count-1) < 0;
}

void handle_client(int client_fd) {
    char buffer[1024 * 512];
    read(client_fd, buffer, sizeof(buffer) - 1);

    if (strstr(buffer, "POST /api/predict")) {
        char *body = strstr(buffer, "\r\n\r\n");
        if (body) {
            body += 4; // skip header end
            body += 1; // skip "

            // Extract pixel data from json string
            unsigned char img_data[IMG_SIZE] = {0};
            unsigned char num, c;
            for (size_t i = 0, idx = 0; i < IMG_SIZE; ++i) {
                num = 0;
                c = body[idx++];
                // Pixel data is sperated by a space
                while (c != '"' && c != ' ') {
                    num = num * 10 + (c - '0');
                    c = body[idx++];
                }
                img_data[i] = num;
                // printf("%d ", num);
            }

            // Predict
            int predict_num;
            float probs[10] = {0};
            predict(img_data, &predict_num, probs);

            // Prepare JSON string
            char json_str[1024] = {0};

#define JSON_STR_FORMAT "{"                                                           \
    "\"prediction\": %d,"                                                             \
    "\"probabilities\": [%.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f]" \
    "}"
            sprintf(json_str, JSON_STR_FORMAT,
                    predict_num,
                    probs[0],
                    probs[1],
                    probs[2],
                    probs[3],
                    probs[4],
                    probs[5],
                    probs[6],
                    probs[7],
                    probs[8],
                    probs[9]
                    );

            // Write to a file
            time_t now = time(NULL);
            struct tm *t = localtime(&now);
            char buf[100] = {0};
            strftime(buf, sizeof(buf), "client_images/%Y%m%d_%H%M%S.png", t);
            printf("  Save as file: %s\n", buf);
            stbi_write_png(buf, 28, 28, 1, img_data, 28);

            // Return a response
            char response[1024];
            sprintf(response,  "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\n");
            // sprintf(response,  "%s%d", response, predict_num);
            sprintf(response,  "%s%s", response, json_str);
            write(client_fd, response, strlen(response));
        }
    }
    // Get the page
    else if (strstr(buffer, "GET /")) {
        char header[256];
        sprintf(header,
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/html\r\n"
                "Content-Length: %ld\r\n"
                "\r\n", drawer_page_lengh);
        write(client_fd, header, strlen(header));
        write(client_fd, drawer_page, drawer_page_lengh);
    }
    close(client_fd);
}

void predict(unsigned char *img_data, int *predict_num, float *result_probs) {
    Mat input = nl_mat_alloc(IMG_SIZE, 1);
    Mat prediction = nl_mat_alloc(10, 1);

    /* Normalize pixel data */
    for (size_t r = 0; r < (size_t)(IMG_SIZE); ++r) {
        NL_MAT_AT(input, r, 0) = (float)img_data[r] / 255.f;
        if (r % 28 == 0) printf("\n");
        /* View client's image */
        if (img_data[r] == 0) printf(".");
        else printf("#");
    }
    printf("\n");

    nl_model_predict(model, input, prediction);
    float maxProb = -1.f;
    int predicted_number = -1;
    for (size_t r = 0; r < prediction.rows; ++r) {
        if (NL_MAT_AT(prediction, r, 0) > maxProb) {
            maxProb = NL_MAT_AT(prediction, r, 0);
            predicted_number = (int)r;
        }
        printf("%.3f ", NL_MAT_AT(prediction, r, 0));
        result_probs[r] = NL_MAT_AT(prediction, r, 0);
    }
    printf("\n");
    printf("  Predicted: %1d\n", predicted_number);

    *predict_num = predicted_number;

    nl_mat_free(input);
    nl_mat_free(prediction);
}
