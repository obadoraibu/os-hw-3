#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>

#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

void* handle_client(void* arg);
void* seller(void* arg);

typedef struct node {
    int value;
    int client_socket;
    struct node *next;
} Node;

typedef struct queue {
    Node *head;
    Node *tail;
    pthread_mutex_t lock;
    pthread_cond_t cond;
} Queue;

void enqueue(Queue *q, int value, int client_socket) {
    Node *newNode = malloc(sizeof(Node));
    newNode->value = value;
    newNode->client_socket = client_socket;
    newNode->next = NULL;

    pthread_mutex_lock(&q->lock);
    if (q->tail) {
        q->tail->next = newNode;
    } else {
        q->head = newNode;
    }
    q->tail = newNode;
    pthread_cond_signal(&q->cond);
    pthread_mutex_unlock(&q->lock);
}

Node dequeue(Queue *q) {
    pthread_mutex_lock(&q->lock);
    while (!q->head)
        pthread_cond_wait(&q->cond, &q->lock);

    Node *temp = q->head;
    Node returnValue = *temp;
    q->head = q->head->next;
    if (!q->head) {
        q->tail = NULL;
    }
    pthread_mutex_unlock(&q->lock);
    free(temp);

    return returnValue;
}

Queue q1 = { .head = NULL, .tail = NULL, .lock = PTHREAD_MUTEX_INITIALIZER, .cond = PTHREAD_COND_INITIALIZER };
Queue q2 = { .head = NULL, .tail = NULL, .lock = PTHREAD_MUTEX_INITIALIZER, .cond = PTHREAD_COND_INITIALIZER };
Queue q3 = { .head = NULL, .tail = NULL, .lock = PTHREAD_MUTEX_INITIALIZER, .cond = PTHREAD_COND_INITIALIZER };

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <port>\n", argv[0]);
        return 1;
    }

    int server_socket, client_socket;
    struct sockaddr_in server_address, client_address;
    socklen_t client_address_len;
    int opt = 1;
    pthread_t thread_id, seller1, seller2, seller3;

    // Create server socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("Failed to create server socket");
        exit(EXIT_FAILURE);
    }

    // Set socket option to reuse address
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        perror("Failed to set socket option");
        exit(EXIT_FAILURE);
    }

    // Configure server address
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(atoi(argv[1]));

    // Bind the socket to the specified address and port
    if (bind(server_socket, (struct sockaddr*)&server_address, sizeof(server_address)) == -1) {
        perror("Failed to bind server socket");
        exit(EXIT_FAILURE);
    }
    // Start listening for incoming connections
    if (listen(server_socket, MAX_CLIENTS) == -1) {
        perror("Failed to listen for connections");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %s...\n", argv[1]);

    pthread_t seller1_thread, seller2_thread, seller3_thread;
    pthread_create(&seller1_thread, NULL, seller, &q1);
    pthread_create(&seller2_thread, NULL, seller, &q2);
    pthread_create(&seller3_thread, NULL, seller, &q3);


    while (1) {
    // Accept a new client connection
        client_address_len = sizeof(client_address);
        client_socket = accept(server_socket, (struct sockaddr*)&client_address, &client_address_len);
        if (client_socket == -1) {
            perror("Failed to accept client connection");
            exit(EXIT_FAILURE);
        }

        printf("New client connected: %s:%d\n", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));

        // Allocate memory for the client socket and copy its value to it
        int *client_socket_ptr = malloc(sizeof(*client_socket_ptr));
        if (!client_socket_ptr) {
            perror("Failed to allocate memory for client_socket");
            exit(EXIT_FAILURE);
        }
        *client_socket_ptr = client_socket;

        // Create a new thread to handle the client
        if (pthread_create(&thread_id, NULL, handle_client, client_socket_ptr) != 0) {
            perror("Failed to create thread");
            exit(EXIT_FAILURE);
        }

    // Detach the thread to clean up automatically
    pthread_detach(thread_id);
    }


    pthread_join(seller1_thread, NULL);
    pthread_join(seller2_thread, NULL);
    pthread_join(seller3_thread, NULL);
    return 0;
}

void *seller(void *arg) {
    Queue *q = (Queue*)arg;

    while (1) {
        Node node = dequeue(q);
        sleep(5);
        if (send(node.client_socket, "You have been served\n", strlen("You have been served\n"), 0) == -1) {
            perror("Failed to send message to client");
            exit(EXIT_FAILURE);
        }
    }
    return NULL;
}

void* handle_client(void* arg) {
    int client_socket = *(int*)arg;
    free(arg);  // free the memory after getting the value
    char buffer[BUFFER_SIZE];
    int received_bytes;

    while (1) {
        memset(buffer, 0, sizeof(buffer)); // Clear the buffer

        // Receive message from the client
        received_bytes = recv(client_socket, buffer, BUFFER_SIZE - 1, 0); // -1 to leave space for null-terminator
        if (received_bytes == -1) {
            perror("Failed to receive message from client");
            exit(EXIT_FAILURE);
        }

        if (received_bytes == 0) {
            printf("Client disconnected\n");
            break;
        }

        buffer[received_bytes] = '\0'; 

        if (strcmp(buffer, "1\n") == 0) {
            printf("client %d got in line 1\n", client_socket);
            
            if (send(client_socket, "you got in line 1\n", strlen("you got in line 1\n"), 0) == -1) {
                perror("Failed to send message to client");
                exit(EXIT_FAILURE);
            }

            enqueue(&q1, 1, client_socket);
        } else if (strcmp(buffer, "2\n") == 0) {
            printf("client %d got in line 2\n", client_socket);

            if (send(client_socket, "you got in line 2\n", strlen("you got in line 2\n"), 0) == -1) {
                perror("Failed to send message to client");
                exit(EXIT_FAILURE);
            }

            enqueue(&q2, 2, client_socket);
        } else if (strcmp(buffer, "3\n") == 0) {
            printf("client %d got in line 3\n", client_socket);

            if (send(client_socket, "you got in line 3\n", strlen("you got in line 3\n"), 0) == -1) {
                perror("Failed to send message to client");
                exit(EXIT_FAILURE);
            }

            enqueue(&q3, 3, client_socket);
        } else if (strcmp(buffer, "exit\n") == 0) {
            printf("client %d exit\n", client_socket);

            if (send(client_socket, "exit\n", strlen("exit\n"), 0) == -1) {
                perror("Failed to send message to client");
                exit(EXIT_FAILURE);
            }

            close(client_socket);
        }

    }
    pthread_exit(NULL);
}