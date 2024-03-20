// For the using of sigaction
#define _XOPEN_SOURCE   600

#include <asm-generic/errno-base.h>
#include <asm-generic/socket.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h> 
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <sys/utsname.h>
#include "utils.h"

#define MAX_CON         3
#define BUF_SIZE        1024

static int serv_sock;
static bool running;

void shutdown_server()
{
    running = false;
}

int main(int argc, char *argv[])
{
    running = true;

    // Shutdown the server gracefully
    struct sigaction sa;
    sa.sa_handler = shutdown_server;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);

    if (argc < 3)
    {
        fprintf(stderr, "Usage: %s host port\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *host = argv[1];
    const char *port = argv[2];

    char buf[BUF_SIZE] = {0};

    // Initialize a server socket
    serv_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (serv_sock == -1)
        die("Failed to create a socket");

    int reuse_addr_port = 1;

    // Reuse socket's address and port
    if (setsockopt(serv_sock, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &reuse_addr_port, sizeof(reuse_addr_port)) == -1)
        die("Failed to set the socket options");

    struct sockaddr_in s_addr;

    memset(&s_addr, 0, sizeof(s_addr));
    s_addr.sin_family = AF_INET;
    s_addr.sin_addr.s_addr = inet_addr(host);
    s_addr.sin_port = htons(atoi(port));

    // Bind the server address to the socket
    if (bind(serv_sock, (struct sockaddr *)&s_addr, sizeof(s_addr)) == -1)
        die("Failed to bind the socket");

    // Listen for the incoming connections
    if (listen(serv_sock, MAX_CON) == -1)
        die("Unable to perform listen on the socket");

    printf("Ready to accept connections on %s:%s\n", host, port);

    // Main server loop
    while(running)
    {
        struct sockaddr_in c_addr;
        socklen_t c_addr_len = sizeof(c_addr);
        memset(buf, 0, sizeof(buf));

        // Accept the connection from the client
        // TODO: Add socket multiplexing
        int client_sock = accept(serv_sock, (struct sockaddr *)&c_addr, &c_addr_len);

        if (client_sock == -1)
        {
            // Hit Ctrl+C, trying to accept a new connection
            if (errno == EINTR)
                continue;
            else
                die("Failed to accept the connection from the client");
        }

        char client_ip[INET_ADDRSTRLEN] = {0};

        // Get the client address
        inet_ntop(AF_INET, &(c_addr.sin_addr.s_addr), client_ip, INET_ADDRSTRLEN);

        printf("Received a connection from %s:%d\n", client_ip, c_addr.sin_port);

        // Handle the server Ctrl+C
        size_t bytes = recv(client_sock, buf, BUF_SIZE, 0);

        printf("Received %zu/%d bytes from the client: %s\n", bytes, BUF_SIZE, buf);

        const char *http_resp = "HTTP/1.1 200 OK\r\n"
                                "Content-Type: text/plain\r\n"
                                "Connection: close\r\n\r\n"
                                "Linux distro is: ";

        // TODO: Ensure that we sent all the data to the client
        size_t sent_bytes = send(client_sock, http_resp, strlen(http_resp), 0);

        printf("Send %zu of %zu/%d bytes to the client: \n", sent_bytes, strlen(http_resp), BUF_SIZE);

        struct utsname dist_info;
        if (uname(&dist_info) == -1)
            die("Can't get distro info");

        const char *distro_name = dist_info.sysname;

        send(client_sock, distro_name, strlen(distro_name), 0);

        close(client_sock);
        printf("Connection closed\n");
    }

    printf("Server shutting down...\n");
    close(serv_sock);

    return 0;
}
