#include <arpa/inet.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdio.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include "../src/coroutine.h"

#define PORT 8080

#define TIMEOUT 50 // milliseconds

void echo_server(sp_stack stack, void* args) {
    ssize_t bytes_read;
    struct pollfd client = {0};
    char buffer[1024] = {0};

    client.fd = (int)(size_t)args;
    client.events = POLLIN | POLLERR | POLLHUP;

    while (true) {
        poll(&client, 1, TIMEOUT);

        if (client.revents & POLLIN) { // Data available to read
            bytes_read = read(client.fd, buffer, sizeof(buffer));

            if (bytes_read <= 0) {
                break; // Client disconnected or error
            }

            send(client.fd, buffer, bytes_read, 0);
        } else if ( client.revents & POLLHUP) { // Client disconnected
            printf("Client disconnected\n");
            break;
        } else if (client.revents & POLLERR) { // Error occurred
            printf("Poll error on client socket\n");
            break;
        }

        yield_ctx(stack);
        client.revents = 0; // Reset revents for the next poll
    }

    close(client.fd);

    return;
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    int server_fd;
    int client_fd;
    int opt;

    sp_stack stack = init_stack(0);

    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;

    struct pollfd server_poll;

    socklen_t client_len = sizeof(client_addr);


    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        return 1;
    }

    opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        return 1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        return 1;
    }

    listen(server_fd, 1);
    printf("Server listening on port %d\n", PORT);

    server_poll.fd = server_fd;
    server_poll.events = POLLIN;

    while (true) {
        poll(&server_poll, 1, TIMEOUT);

        if (server_poll.revents & POLLIN) {
            client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
            if (client_fd < 0) {
                perror("accept");
                return 1;
            }

            printf("Client connected from %s:%d\n",
                   inet_ntoa(client_addr.sin_addr),
                   ntohs(client_addr.sin_port));

            create_ctx(stack, echo_server, (void*)(size_t)client_fd); // currently leaking coroutines
        }

        yield_ctx(stack);
        server_poll.revents = 0;
    }

    close(server_fd);
    deinit_stack(stack);
    return 0;
}
