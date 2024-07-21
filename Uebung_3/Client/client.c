/**
 * @file   client.c
 * @author Vedad Hadzic (e12042758@student.tuwien.ac.at)
 * @date   14.01.2024
 *
 * @brief A program for impelementation HTTP Client
 *
 * @details The Client program partially implements version 1.1 of the HTTP. The client takes an URL as
 * input, connects to the corresponding server and requests the file specified in the URL. The transmitted
 * content of that file is written to stdout or to a file.
**/

#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define HTTP_PORT 80

// Function prototypes

static void usage();

static void error(const char *message);

static int parse_port(const char *optarg, int *);

static char *handle_directory_option(const char *dir, const char *url);

static void parse_arguments(int argc, char *argv[], int *port, char **outputFilePath, char **url, char **hostname, char **path);

static int establish_connection(const char *hostname, const char *port);

static void send_http_get_request(int sockfd, const char *filepath, const char *host);

static void process_http_response(FILE *sockfile, FILE *outputFileStream);

//----------------------------------------------------------------------------

/**
 * @brief Main function to initiate the HTTP client.
 *
 * This function is the entry point for the HTTP client program. It parses command line
 * arguments, establishes a connection to the remote server, sends an HTTP GET request,
 * processes the HTTP response, and manages the opening and closing of file streams for
 * both the socket and the output file.
 *
 * @param argc The number of command line arguments.
 * @param argv An array of strings representing the command line arguments.
 *
 * @return An integer status code indicating the success or failure of the program.
 */

int main(int argc, char *argv[]) {
    int port = HTTP_PORT;
    char *outputFilePath = NULL;
    char *url = NULL;
    char *hostname = NULL;
    char *path = NULL;

    // Parse command line arguments
    parse_arguments(argc, argv, &port, &outputFilePath, &url, &hostname, &path);

    // Convert port number to string
    char portStr[6];
    snprintf(portStr, sizeof(portStr), "%d", port);

    // Establish connection
    int sockfd = establish_connection(hostname, portStr);
    if (sockfd < 0) {
        exit(EXIT_FAILURE);
    }

    // Send the HTTP GET request
    send_http_get_request(sockfd, path, hostname);

    // Open the output file stream or use stdout
    FILE *outputFileStream;
    if (outputFilePath != NULL) {
        outputFileStream = fopen(outputFilePath, "w");
        if (outputFileStream == NULL) {
            error("Error opening output file");
        }
    } else {
        outputFileStream = stdout;
    }

    // Open the socket file stream for reading
    FILE *sockfile = fdopen(sockfd, "r+");
    if (sockfile == NULL) {
        if (outputFileStream != stdout) fclose(outputFileStream);
        error("Error opening socket file stream");
    }

    // Process the HTTP response
    process_http_response(sockfile, outputFileStream);

    // Close the socket file stream (also closes the socket)
    fclose(sockfile);

    // Close the output file stream if it's not stdout
    if (outputFileStream != stdout) {
        fclose(outputFileStream);
    }

    // Free dynamically allocated memory
    if (hostname != NULL) {
        free(hostname);
    }
    if (path != NULL) {
        free(path);
    }

    return 0;
}

//------------------------------------------------------------------------

/**
 * @brief Display the usage message for the program 
 * 
 * @return EXIT_FAILURE 
 */

static void usage() {
    fprintf(stderr, "Usage: [-p PORT] [ -o FILE | -d DIR ] URL\n");
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
 * @brief Validates if the output file can be opened for writing.
 *
 * @param filename The name of the file to be opened for writing.
 */

static void validate_output_file(const char *filename) {
    FILE *file = fopen(filename, "w");
    if (file == NULL) {
        error("Cannot open output file for writing");
    }
    fclose(file);
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
    char *endo;
    long portnum = strtol(optarg, &endo, 10);

    if (*endo != '\0' || portnum <= 0 || portnum > 65535) {
        return -1;
    }

    *port = (int) portnum;
    return 0;
}

//----------------------------------------------------------------

/**
 * @brief Construct a full file path based on the given directory and URL.
 *
 * @details This function takes a directory path and a URL as input, and constructs a full
 * file path by extracting the filename from the URL and combining it with the directory.
 * If the URL does not contain a filename, it uses "index.html" as the default filename.
 *
 * @param dir The directory path where the file should be located.
 * @param url The URL from which the filename is extracted.
 *
 * @return A dynamically allocated string representing the full file path.
 *         Returns NULL if memory allocation fails or if the inputs are invalid.
 */

static char *handle_directory_option(const char *dir, const char *url) {
    // Extract the filename from the URL
    const char *lastSlash = strrchr(url, '/');
    if (lastSlash == NULL) {
        lastSlash = url; // No '/' found, use the entire URL
    } else {
        lastSlash++; // Move past the '/'
    }

    // Find the position of the first query parameter or delimiter, if there is one 
    const char *queryParam = strpbrk(lastSlash, "?;:/@=&");
    size_t filenameLength = (queryParam != NULL) ? (size_t) (queryParam - lastSlash) : strlen(lastSlash);

    // Use 'index.html' if the filename is empty
    char *filename = (filenameLength == 0) ? strdup("index.html") : strndup(lastSlash, filenameLength);
    if (filename == NULL) {
        return NULL; // Memory allocation failed
    }

    // Construct the full path
    size_t dir_len = strlen(dir);
    int needs_slash = (dir[dir_len - 1] != '/');
    char *fullPath = malloc(dir_len + strlen(filename) + needs_slash + 1); // +1 for '\0'
    if (fullPath == NULL) {
        free(filename);
        return NULL; // Memory allocation failed
    }

    snprintf(fullPath, dir_len + strlen(filename) + needs_slash + 1, "%s%s%s", dir, needs_slash ? "/" : "", filename);
    free(filename); // Free the temporary filename
    return fullPath;
}

//----------------------------------------------------------------

/**
 * @brief Parses the URL to extract the hostname and path.
 *
 * @details This function takes a URL and extracts the hostname and path components from it.
 *
 * @param url The URL to parse.
 * @param hostname A pointer to store the extracted hostname.
 * @param path A pointer to store the extracted path.
 */

static void parse_url(const char *url, char **hostname, char **path) {
    // Ensure the URL starts with "http://"
    if (strncmp(url, "http://", 7) != 0) {
        error("URL must start with 'http://'");
    }

    // Skip "http://"
    const char *urlWithoutScheme = url + 7;

    // Find the end of the hostname using strpbrk which looks for any of the specified delimiters.
    const char *hostnameEnd = strpbrk(urlWithoutScheme, "/?");

    if (hostnameEnd != NULL) {
        size_t hostnameLength = hostnameEnd - urlWithoutScheme;
        *hostname = strndup(urlWithoutScheme, hostnameLength);

        // The rest of the string, starting from the found delimiter, is the path
        *path = strdup(hostnameEnd);
    } else {
        // If none of the delimiters are found, the entire string after "http://" is the hostname
        *hostname = strdup(urlWithoutScheme);
        *path = strdup("/");  // Default path
    }

    // Validate the hostname
    if (*hostname == NULL || strlen(*hostname) == 0) {
        error("Invalid or missing hostname");
    }
}
//----------------------------------------------------------------

/**
 * @brief Parse command-line arguments and set program options.
 *
 * @details This function parses the command-line arguments, sets the program options based on the provided flags,
 * and extracts the URL, hostname, and path components from the command line.
 *
 * @param argc The number of command-line arguments.
 * @param argv An array of command-line argument strings.
 * @param port A pointer to store the selected port number.
 * @param outputFile A pointer to store the output file path if provided.
 * @param url A pointer to store the URL.
 * @param hostname A pointer to store the extracted hostname from the URL.
 * @param path A pointer to store the extracted path from the URL.
 */

static void parse_arguments(int argc, char *argv[], int *port, char **outputFile, char **url, char **hostname, char **path) {
    int opt, is_o_flag_used = 0, is_d_flag_used = 0, is_p_flag_used = 0;
    char *dir_option_arg = NULL;

    *port = HTTP_PORT;
    *outputFile = NULL;
    *url = NULL;

    while ((opt = getopt(argc, argv, "p:o:d:")) != -1) {
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

            case 'o':
                if (is_o_flag_used || is_d_flag_used) {
                    error("Multiple output options (-o, -d) are not allowed");
                }
                *outputFile = optarg;
                is_o_flag_used = 1;
                break;

            case 'd':
                if (is_o_flag_used || is_d_flag_used) {
                    error("Multiple output options (-o, -d) are not allowed");
                }
                is_d_flag_used = 1;
                dir_option_arg = optarg;
                break;

            case '?':
                usage();
                break;

            default:
                usage();
                break;
        }
    }

    // Extract URL
    if (optind < argc) {
        *url = argv[optind];
        parse_url(*url, hostname, path); // Extract hostname and path
    } else {
        usage();

    }

    // Handle the '-d' option, if it was set
    if (is_d_flag_used) {
        char *fullPath = handle_directory_option(dir_option_arg, *url);
        if (fullPath == NULL) {
            error("Failed to handle directory option");
        }
        if (*outputFile != NULL) {
            free(*outputFile); // Free previous memory if allocated
        }
        *outputFile = fullPath;
    }

    // Validate outputFile if -o option is used
    if (*outputFile != NULL && is_o_flag_used) {
        validate_output_file(*outputFile);
    }

    // Check for too many arguments
    if (optind < argc - 1) {
        error("Too many arguments");
    }
}

//---------------------------------------------------------------

/**
 * @brief Establish a connection to a remote server using hostname and port.
 *
 * @details This function resolves the provided hostname and port, creates a socket, and
 * establishes a connection to the remote server.
 *
 * @param hostname The hostname or IP address of the remote server.
 * @param port The port number to connect to on the remote server.
 *
 * @return On success, the function returns a valid socket file descriptor.
 *         On failure, it returns -1 and displays an error message using the `error` function.
 */

static int establish_connection(const char *hostname, const char *port) {
    struct addrinfo hints, *server_info;
    int socket_fd;

    // Setting up the hints structure for getaddrinfo
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;     
    hints.ai_socktype = SOCK_STREAM; 

    // Resolving the server address and port
    int status = getaddrinfo(hostname, port, &hints, &server_info);
    if (status != 0) {
        error("Error in resolving server address");
        return -1;
    }

    // Creating a socket
    socket_fd = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol);
    if (socket_fd == -1) {
        freeaddrinfo(server_info);
        error("Error in socket creation");
        return -1;
    }

    // Connecting to the server
    if (connect(socket_fd, server_info->ai_addr, server_info->ai_addrlen) == -1) {
        close(socket_fd); // Close the socket to prevent resource leak
        freeaddrinfo(server_info);
        error("Error in connecting to server");
        return -1;
    }

    freeaddrinfo(server_info);

    return socket_fd;
}

//----------------------------------------------------------------------------

/**
 * @brief Send an HTTP GET request to a remote server over the provided socket.
 *
 * @details This function sends an HTTP GET request to a remote server using the provided
 * socket file descriptor, path, and host.
 *
 * @param sockfd The socket file descriptor connected to the remote server.
 * @param path The path of the resource to request, including query parameters.
 * @param host The hostname or IP address of the remote server.
 */

static void send_http_get_request(int sockfd, const char *path, const char *host) {
    // Associate a file stream with the socket for writing
    FILE *sockfile = fdopen(sockfd, "w");
    if (sockfile == NULL) {
        error("Error opening socket file stream");
        return;
    }

    // Send the HTTP GET request
    fprintf(sockfile, "GET %s HTTP/1.1\r\n", path);
    fprintf(sockfile, "Host: %s\r\n", host);
    fprintf(sockfile, "Connection: close\r\n\r\n"); // End of headers

    // Check for errors in sending
    if (ferror(sockfile)) {
        fclose(sockfile); // Close the stream to also close the socket
        error("Error sending HTTP GET request");
    }

    fflush(sockfile);
}


//-------------------------------------------------------------------------------

/**
 * @brief Process the HTTP response received from the remote server.
 *
 * @param sockfile The file stream associated with the socket for reading the HTTP response.
 * @param outputFileStream The file stream where the HTTP response body will be written.
 */

static void process_http_response(FILE *sockfile, FILE *outputFileStream) {
    char *line = NULL;
    size_t len = 0;
    ssize_t read;

    // Read the status line
    if ((read = getline(&line, &len, sockfile)) == -1) {
        if (line) free(line);
        error("Error reading status line");
    }

    // Check HTTP version and status code
    char *token = strtok(line, " ");
    if (token == NULL || strcmp(token, "HTTP/1.1") != 0) {
        free(line);
        fprintf(stderr, "Protocol error!\n");
        exit(2);
    }

    token = strtok(NULL, " ");

    // status code check
    char *parseEnd = NULL;
    long statusCode = strtol(token, &parseEnd, 10);

    if (*parseEnd != '\0') {
        free(line);
        fprintf(stderr, "Protocol error!\n");
        exit(2); 

    } else if (statusCode != 200) {
        // Extract and print only the status code and reason phrase
        char *reason_phrase = strtok(NULL, "\r\n");
        if (reason_phrase != NULL) {
            fprintf(stderr, "%ld %s\n", statusCode, reason_phrase);
        } else {
            fprintf(stderr, "%ld\n", statusCode);
        }
        free(line);
        exit(3); // Exit with status code 3 for non-200 responses
    } else {
        // Handle 200 OK

        // Skip headers
        while ((read = getline(&line, &len, sockfile)) != -1) {
            if (strcmp(line, "\r\n") == 0) {
                break;
            }
        }

        // Read and write the response body
        while ((read = getline(&line, &len, sockfile)) != -1) {
            fprintf(outputFileStream, "%s", line);
        }
    }
    if (line) {
    free(line);
    }
}

    