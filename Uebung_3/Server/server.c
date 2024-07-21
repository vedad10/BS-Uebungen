/**
 * @file   server.c
 * @author Vedad Hadzic (e12042758@student.tuwien.ac.at)
 * @date   15.01.2024
 *
 * @brief A program for impelementation HTTP Server
 *
 * @details a server program which partially implement version 1.1 of the HTTP. The server waits for connections 
            from clients and transmits the requested files.
**/

#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h> 
#include <sys/stat.h>
#include <signal.h>

#define HTTP_PORT 8080
#define BUFFER_SIZE 2048
volatile sig_atomic_t signal_flag = 0;

// Function prototypes

static void usage(const char *progName);

static void error(const char *message);

static int parse_port(const char *optarg, int *port);

static void parse_arguments(int argc, char *argv[], int *port, char **indexFile, char **docRoot);

static void setup_server_socket(int *serverfd, const char *port);

static void process_http_request(int clientfd, const char *docRoot, const char *indexFile);

static void signal_handler(int signal_num);

//-------------------------------------------

/**
 * @brief Main function of the HTTP server program.
 * 
 * @details This function initializes the server, sets up the listening socket, and then
 * enters a loop to accept and handle client requests.
 *
 * @param argc The number of command line arguments.
 * @param argv The array of command line arguments.
 *
 * @return Returns 0 upon successful execution.
 */

int main(int argc, char *argv[]) {
    int serverfd;
    int port;
    char *indexFile;
    char *docRoot;
    char portStr[6]; 

    parse_arguments(argc, argv, &port, &indexFile, &docRoot);

    // Convert the integer port number to a string
    snprintf(portStr, sizeof(portStr), "%d", port);

    setup_server_socket(&serverfd, portStr);

    // Setup signal handling
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = signal_handler;

    // Setup signal handling for SIGINT
    if (sigaction(SIGINT, &sa, NULL) < 0) {
        error("Error setting up SIGINT handler");
        close(serverfd);
        exit(EXIT_FAILURE);
    }

    // Setup signal handling for SIGTERM
    if (sigaction(SIGTERM, &sa, NULL) < 0) {
        error("Error setting up SIGTERM handler");
        close(serverfd);
        exit(EXIT_FAILURE);
    }

    // Accept and process client connections
        while (!signal_flag) {
            struct sockaddr_storage client_addr;
            socklen_t addr_size = sizeof(client_addr);
            int clientfd = accept(serverfd, (struct sockaddr *)&client_addr, &addr_size);
            if (clientfd < 0) {
                if (signal_flag) {
                    break;
                }
                error("accept");
                continue;
            }

            process_http_request(clientfd, docRoot, indexFile);
            close(clientfd);
        }

        close(serverfd);
        return 0;
    }

//-------------------------------------------

/**
 * @brief Handles SIGINT and SIGTERM signals by setting a flag to indicate 
 *        the signal has been received.
 *
 * @param signal_num The signal number to handle.
 */

static void signal_handler(int signal_num) {
    if (signal_num == SIGINT || signal_num == SIGTERM) {
        signal_flag = 1;
    }
}

//-------------------------------------------
/**
 * @brief Display the usage message for the program 
 * 
 * @param progName Program name
 *
 * @return EXIT_FAILURE 
 */

static void usage(const char *progName) {
    fprintf(stderr, "Usage: %s [-p PORT] [-i INDEX] ument\n", progName);
    exit(EXIT_FAILURE);
}

//----------------------------------------------------------------

/**
 * @brief Prints an error message and exits the program.
 *
 * @details This function prints the provided error message along with the last system error message
 * (if available) to the standard error stream. It then exits the program with the EXIT_FAILURE status.
 *
 * @param message The error message to be printed.
 *
 */

static void error(const char *message) {
    fprintf(stderr, "\nError: %s", message);
    if (errno) {
        fprintf(stderr, " (%s)", strerror(errno));
    }
    fprintf(stderr, "\n\n");
    exit(EXIT_FAILURE);
}

//-----------------------------------------------------------------

/**
 * @brief Parses and validates a port number from a string.
 *
 * @details This function attempts to parse a port number from the given string. It checks
 * if the port number is within the valid range (1-65535) and if the string contains
 * only numeric characters. If the port number is valid, it is stored in the provided
 * port pointer.
 *
 * @param optarg The string containing the port number to be parsed.
 * @param port Pointer to an integer where the parsed port number will be stored.
 *
 * @return 0 on successful parsing and validation, -1 if the port number is invalid.
**/

static int parse_port(const char *optarg, int *port) {
    char *endptr;
    long portnum = strtol(optarg, &endptr, 10);

    if (*endptr != '\0' || portnum <= 0 || portnum > 65535) {
        return -1; 
    }

    *port = (int)portnum;
    return 0; 
}

//----------------------------------------------------------------

/**
 * @brief Parses command-line arguments to configure the HTTP server.
 *
 * @param argc      The number of command-line arguments.
 * @param argv      An array of command-line arguments.
 * @param port      A pointer to an integer that will store the server port.
 * @param indexFile A pointer to a string that will store the index file name.
 * @param docRoot   A pointer to a string that will store the document root path.
 */

static void parse_arguments(int argc, char *argv[], int *port, char **indexFile, char **docRoot) {
    int opt, is_p_flag_used = 0, is_i_flag_used = 0;
    *port = HTTP_PORT; // Default port
    *indexFile = "index.html"; // Default index file

    while ((opt = getopt(argc, argv, "p:i:")) != -1) {
        switch (opt) {
            case 'p':
                if (is_p_flag_used) {
                    error("Multiple ports are not allowed");
                }
                if (parse_port(optarg, port) == -1) {
                    error("Invalid port number");
                }
                is_p_flag_used = 1;
                break;
            case 'i':
                if (is_i_flag_used) {
                    error("Multiple index files are not allowed");
                }
                *indexFile = optarg;
                is_i_flag_used = 1;
                break;
            default:
                usage(argv[0]);
                break;
        }
    }

    if (optind >= argc) {
        usage(argv[0]);
    }

      int ArgCount = argc - optind; // Count of non-option arguments
    if (ArgCount != 1) { 
        usage(argv[0]);
    }

    *docRoot = argv[optind]; // The last argument should be the document root
}

//--------------------------------------------------------------------------------

/**
 * @brief FUnction to set up a server socket for the HTTP server.
 *
 * @details This function initializes and configures a server socket for the HTTP server to listen
 * on the specified port.
 *
 * @param serverfd A pointer to an integer that will store the server socket file descriptor.
 * @param port     The port on which the server should listen, provided as a string.
 */

static void setup_server_socket(int *serverfd, const char *port) {
    struct addrinfo hints, *res, *p;
    int status;
    int yes = 1;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET; 
    hints.ai_socktype = SOCK_STREAM; 
    hints.ai_flags = AI_PASSIVE; 

    if ((status = getaddrinfo(NULL, port, &hints, &res)) != 0) {
        error("getaddrinfo failed");
    }

    for (p = res; p != NULL; p = p->ai_next) {
        if ((*serverfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            error("server: socket");
            continue;
        }

        if (setsockopt(*serverfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
            error("setsockopt failed");
        }

        if (bind(*serverfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(*serverfd);
            error("server: bind");
            continue;
        }

        break; // Successfully bound
    }

    if (p == NULL) {
        error("server: failed to bind");
    }

    if (listen(*serverfd, 10) == -1) { // Backlog set to 10
        error("listen failed");
    }

    freeaddrinfo(res); // Free the linked list
}

//-----------------------------------------------------------------------

 /**
 * @brief Retrieves the current date and time in a specified format and stores it in a provided string.
 *
 * @param date_str A character array to store the current date and time as a string.
 * @param size     The size of the character array `date_str`.
 */

static void get_current_date(char *date_str, size_t size) {
    time_t current_time;
    struct tm *time_info;

    time(&current_time);
    time_info = gmtime(&current_time);

    strftime(date_str, size, "%a, %d %b %y %H:%M:%S", time_info);
}

//---------------------------------------------------------------

/**
 * @brief Processes an HTTP request received from a client socket and sends an appropriate response.
 *
 * @param socket_client  The client socket file descriptor.
 * @param document_root     The document root directory where web content is stored.
 * @param default_page The default page to serve when the URI ends with a '/'.
 */
void process_http_request(int socket_client, const char* document_root, const char* default_page) {
    char *request_line = NULL;
    ssize_t bytes_read;
    char buffer[BUFFER_SIZE];

    // Read the request line from the client socket
    bytes_read = read(socket_client, buffer, BUFFER_SIZE - 1);
    if (bytes_read <= 0) {
        error("Read error");
        close(socket_client);
        return;
    }
    buffer[bytes_read] = '\0';

    request_line = strdup(buffer);

    // Parse the request line into method, URI, and protocol
    char *method = strtok(request_line, " ");
    char *uri = strtok(NULL, " ");
    char *protocol = strtok(NULL, "\r\n");

    // Check the validity of the request
    if (!method || !uri || !protocol || strcmp(protocol, "HTTP/1.1") != 0) {
        // Invalid request, send a 400 Bad Request response
        write(socket_client, "HTTP/1.1 400 Bad Request\r\nConnection: close\r\n\r\n", 47);
        close(socket_client);
        free(request_line);
        return;
    }

    if (strcmp(method, "GET") != 0) {
        // Unsupported HTTP method, send a 501 Not Implemented response
        write(socket_client, "HTTP/1.1 501 Not Implemented\r\nConnection: close\r\n\r\n", 51);
        close(socket_client);
        free(request_line);
        return;
    }

    char file_path[BUFFER_SIZE];
    snprintf(file_path, BUFFER_SIZE, "%s%s", document_root, uri);

    // Handle URI ending with '/' by appending the default page
    if (uri[strlen(uri) - 1] == '/') {
        strncat(file_path, default_page, BUFFER_SIZE - strlen(file_path) - 1);
    }

    // Open and read the requested file
    int file_fd = open(file_path, O_RDONLY);
    if (file_fd < 0) {
        // File not found, send a 404 Not Found response
        write(socket_client, "HTTP/1.1 404 Not Found\r\nConnection: close\r\n\r\n", 45);
        close(socket_client);
        free(request_line);
        return;
    }

    // Get file size
    struct stat stat_buf;
    fstat(file_fd, &stat_buf);
    off_t file_size = stat_buf.st_size;

    char date_str[BUFFER_SIZE];
    get_current_date(date_str, sizeof(date_str));

    // Construct and send the HTTP response header
    char header[BUFFER_SIZE];
    int header_length = snprintf(header, BUFFER_SIZE,
                                 "HTTP/1.1 200 OK\r\n"
                                 "Date: %s\r\n"
                                 "Content-Length: %ld\r\n"
                                 "Connection: close\r\n\r\n",
                                 date_str, file_size);
    write(socket_client, header, header_length);

    // Send the file content in chunks
    while ((bytes_read = read(file_fd, buffer, BUFFER_SIZE)) > 0) {
        write(socket_client, buffer, bytes_read);
    }

    // Cleanup and close the file, close the client socket
    close(file_fd);
    close(socket_client);
    free(request_line);
}
