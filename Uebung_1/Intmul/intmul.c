/**
 * @file   intmul.c
 * @author Vedad Hadzic (e12042758@student.tuwien.ac.at)
 * @date   12.11.2023
 *
 * @brief  Intmul: A program for the implementation of hexadecimal multiplication.
 *
 * @details The intmul program reads and recursively multiplies two hexadecimal input values (strings) with an equal number of digits 
 *          from stdin and writes the result to stdout. The program accepts any number of digits and both upper and lowercase characters.
 *          The input numbers should have a length that is a power of two and they need to have the same length. If not, they should be
 *          filled with the correct number of leading zeroes so that both integers have equal length, which is a power of two. 
 *          If the program has two numbers with more than one digit inside, then it uses fork and execlp to recursively execute 
 *          this program in four child processes, one for each of the multiplications Ah · Bh, Ah · Bl, Al · Bh, and Al · Bl. 
 *          The four child processes run simultaneously, and wait and waitpid are used to read the exit status of the children.
 *          The output is in lowercase characters. Terminate the program with exit status EXIT_SUCCESS.
 *
 * @note    Example usage:
 *  
 *          $ ./intmul
 *          1a
 *          ab
 *          115e
 *          
 *
 * @return  EXIT_SUCCESS upon successful execution or EXIT_FAILURE upon error.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>

/**
 * @brief Checks if a given string is a valid hexadecimal number.
 *
 * @param s Pointer to the string to be checked throught function
 * 
 * @return 1 if the string is a valid hexadecimal number, 0 otherwise.
 */

int isValidHexadecimal(const char *s) {
    for (int i = 0; s[i] != '\0'; i++) {
        if (!((s[i] >= '0' && s[i] <= '9') ||
              (s[i] >= 'a' && s[i] <= 'f') ||
              (s[i] >= 'A' && s[i] <= 'F') ||
              s[i] == '\n')) {
            return 0;
        }
    }
    return 1;
}

/**
 * @brief Converts a hexadecimal string to its decimal equivalent.
 *
 * @param hex Pointer to the hexadecimal string.
 * 
 * @return Decimal number equivalent to the hexadecimal string.
 */

int hexDigitToInt(char hex) {
    if (isxdigit(hex)) {
        if (isdigit(hex)) {
            return hex - '0';
        } else {
            return tolower(hex) - 'a' + 10;
        }
    } else {
        return -1;
    }
}
/**
 * @brief Converting an integer to its hexadecimal representation.
 *
 * @param num The integer value to be converted to hexadecimal.
 * 
 * @return The hexadecimal character representation of the input integer.
 */

char intToHex(int num) {
    if (num >= 0 && num <= 15) {
        char hex_digits[] = "0123456789abcdef";
        return hex_digits[num];
    } else {
        return -1;
    }
}
/**
 * @brief This function reads two hexadecimal input strings from stdin and perform validation.
 *
 * @param inputA Pointer to the first input string.
 * @param inputB Pointer to the second input string.
 */

void readInput(char **inputA, char **inputB) {

    int len = 0;
    int readFirstInput;
    int readSecondInput;

    // Read the first input
    readFirstInput = getline(inputA, (size_t *) &len, stdin);

    if (readFirstInput == -1) {
        fprintf(stderr, "Error reading the first input!\n");
        exit(EXIT_FAILURE);
    }

    if (readFirstInput == 1 && (*inputA)[0] == '\n') {
        fprintf(stderr, "Error: Empty first input!\n");
        exit(EXIT_FAILURE);
    }

    // Read the second input
    readSecondInput = getline(inputB, (size_t *) &len, stdin);

    if (readSecondInput == -1) {
        fprintf(stderr, "Error reading the second input!\n");
        exit(EXIT_FAILURE);
    }

    if (readSecondInput == 1 && (*inputB)[0] == '\n') {
        fprintf(stderr, "Error: Empty second input!\n");
        exit(EXIT_FAILURE);
    }

    // Check if inputA is a valid hexadecimal number
    if (!isValidHexadecimal(*inputA)) {
        fprintf(stderr, "Invalid Input for the first number!\n");
        exit(EXIT_FAILURE);
    }

    // Check if inputB is a valid hexadecimal number
    if (!isValidHexadecimal(*inputB)) {
        fprintf(stderr, "Invalid Input for the second number!\n");
        exit(EXIT_FAILURE);
    }

    int lenA = strlen(*inputA);
    int lenB = strlen(*inputB);

    // Trim the newline character if present in inputA
    if (lenA > 0 && (*inputA)[lenA - 1] == '\n') {
        (*inputA)[lenA - 1] = '\0';
        lenA--;
    }

    // Trim the newline character if present in inputB
    if (lenB > 0 && (*inputB)[lenB - 1] == '\n') {
        (*inputB)[lenB - 1] = '\0';
        lenB--;
    }
}

/**
* @brief Multiply two hexadecimal digits and calculate low-order and high-order digits (assuming a and b are valid hexadecimal digits).
*
* @param a         The first hexadecimal digit.
* @param b         The second hexadecimal digit.
* @param product   Pointer to store the low-order digit result.
* @param carryOut  Pointer to store the high-order digit result (carry).
*/
void multiplyHexDigits(char a, char b, char *product, char *carryOut) {

    int numA = hexDigitToInt(a);
    int numB = hexDigitToInt(b);
    int result = numA * numB;

    // Compute low-order digit (remainder when divided by 16)
    *product = intToHex(result % 16);

    // Compute high-order digit (result of the division by 16)
    *carryOut = intToHex(result / 16);
}
/**
 * @brief Add two hexadecimal digits and a carry to produce the result (assuming a, b, and carry are valid hexadecimal digits).
 *
 * @param a         The first hexadecimal digit.
 * @param b         The second hexadecimal digit.
 * @param carry     The carry digit from the previous addition.
 * @param carryOut  Pointer to store the carry for the next addition.
 *
 * @return          The result of adding 'a', 'b', and 'carry'.
 */

char addHexDigits(char a, char b, char *carryOut) {

    int numA = hexDigitToInt(a);
    int numB = hexDigitToInt(b);
    int numCarry = hexDigitToInt(*carryOut);

    int result = numA + numB + 16 * numCarry; // Calculate the sum of the digits and carry
    *carryOut = intToHex(result / 16); // Compute the carry for the next addition

    return intToHex(result % 16); // Return the low-order digit
}

/**
 * @brief Pad and align two hexadecimal numbers to have equal length and be a power of two.
 *
 * @param numA  Pointer to the first hexadecimal number (string representation).
 * @param numB  Pointer to the second hexadecimal number (string representation).
 */

void padAndAlignNumbers(char **numA, char **numB) {
    int lenA = strlen(*numA);
    int lenB = strlen(*numB);

    // Calculate the new length to be a power of two
    int maxLength = 1;
    while (maxLength < lenA || maxLength < lenB) {
        maxLength *= 2;
    }

    // Allocate memory for the padded numbers
    char *paddedA = (char *)malloc(maxLength + 1); // +1 for null terminator
    char *paddedB = (char *)malloc(maxLength + 1);

    if (paddedA == NULL || paddedB == NULL) {
        fprintf(stderr, "Memory allocation error\n");

        // Free original memory before exiting
        free(*numA);
        free(*numB);
        exit(EXIT_FAILURE);
    }

    // Fill with leading zeros
    memset(paddedA, '0', maxLength - lenA);
    strcpy(paddedA + maxLength - lenA, *numA);

    memset(paddedB, '0', maxLength - lenB);
    strcpy(paddedB + maxLength - lenB, *numB);

    // Free the original memory
    free(*numA);
    free(*numB);

    // Update the original numbers with padded ones
    *numA = paddedA;
    *numB = paddedB;
}
/**
 * @brief Splits two hexadecimal numbers into high and low-order halves.
 *
 * @param numA The first hexadecimal number.
 * @param numB The second hexadecimal number.
 * @param Ah   Pointer to the allocated memory for the high-order half of numA.
 * @param Al   Pointer to the allocated memory for the low-order half of numA.
 * @param Bh   Pointer to the allocated memory for the high-order half of numB.
 * @param Bl   Pointer to the allocated memory for the low-order half of numB.
 *
 */

void splitNumbers(char *numA, char *numB, char *Ah, char *Al, char *Bh, char *Bl) {

    // Calculate the midpoint
    int len = strlen(numA);    //it can also be numB, both are the same
    int mid = len / 2;

    // Copy the halves from the original numbers
    strncpy(Ah, numA, mid);
    Ah[mid] = '\0';
    //printf("Ah: %s\n", Ah);

    strcpy(Al, numA + mid);
    Al[mid] = '\0';
    //printf("Al: %s\n", Ai);

    strncpy(Bh, numB, mid);
    Bh[mid] = '\0';
    // printf("Bh: %s\n", Bh);

    strcpy(Bl, numB + mid);
    Bl[mid] = '\0';
    //printf("Bl: %s\n", Bl);

}

/**
 * @brief Sends the divided portions of hexadecimal numbers (Ah, Bh, Al, Bi) to the child process through a pipe.
 *
 * @param pipefd File descriptor of the pipe (write end) for communication with the child process.
 * @param A Divided portion of the first hexadecimal number.
 * @param B Divided portion of the second hexadecimal number.
 */
void sendInputToChild(int pipefd, char *A, char *B) {
    FILE *stream = fdopen(pipefd, "w");
    if (stream == NULL) {
        fprintf(stderr, "Error opening stream: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    // Send input to the child
    if (fprintf(stream, "%s\n%s\n", A, B) < 0) {
        fprintf(stderr, "Error writing to child process: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    fclose(stream);
}

/**
* @brief Wait for multiple child processes to finish and check their exit statuses.
*
* @param child An array of process IDs representing the child processes to wait for.
* @param numChildren The number of child processes in the 'child' array.
*
* @note The function terminates the program with EXIT_FAILURE if any child process exits abnormally.
*/

void waitForChildren(pid_t child[], int numChildren) {
    int status;
    for (int i = 0; i < numChildren; i++) {
        waitpid(child[i], &status, 0);
        if (WEXITSTATUS(status) != EXIT_SUCCESS) {
            fprintf(stderr, "Error in child process %d\n", i);
            exit(EXIT_FAILURE);
        }
    }
}
/**
 * @brief Free the memory allocated for intermediate results.
 *
 * @param intermediateResults Array of pointers to intermediate results.
 * @param numResults Number of intermediate results.
 */
void freeIntermediateResults(char* intermediateResults[], int numResults) {
    for (int i = 0; i < numResults; i++) {
        free(intermediateResults[i]);
    }
}


/**
 * @brief Free the memory allocated for Ah, Al, Bh, Bl.
 *
 * @param Ah High-order half of the first hexadecimal number.
 * @param Al Low-order half of the first hexadecimal number.
 * @param Bh High-order half of the second hexadecimal number.
 * @param Bl Low-order half of the second hexadecimal number.
 */
void freeSplitNumbers(char* Ah, char* Al, char* Bh, char* Bl) {
    free(Ah);
    free(Al);
    free(Bh);
    free(Bl);
}


/**
 * @brief The main function of the intmul program executes the multiplication process, utilizing
 * parallel computation through multiple child processes. 
 * 
 * @details It manages input processing, pipe creation, child process forking, communication with children, waiting for child processes, reading intermediate
 * results, shifting results, summing them to obtain the final result, and printing the outcome. The program
 * handles various scenarios, including empty inputs, single-digit multiplication, and parallelizing
 * computation for larger inputs.
 * 
 * @param argc The number of command-line arguments.
 * @param argv An array of strings representing the command-line arguments.
 *
 * @return Returns 0 on successful execution, EXIT_FAILURE otherwise.
 */

int main(int argc, char *argv[]) {

    if (argc != 1) {
        fprintf(stderr, "Usage: %s\n", argv[0]);
        fprintf(stderr, "This program does not accept command-line arguments.\n");
        return EXIT_FAILURE;
    }

    char *inputA = NULL;
    char *inputB = NULL;

    // Read and validate input
    readInput(&inputA, &inputB);
    padAndAlignNumbers(&inputA, &inputB);

    // printf("First number: %s\n second number: %s\n", inputA, inputB);

    int lenA = strlen(inputA);

    if (lenA > 1) { // Perform the calculation using multiple processes

        char *Ah, *Al, *Bh, *Bl;
        int mid = lenA / 2;
        Ah = (char *)malloc(mid + 1);  // +1 for null terminator
        Al = (char *)malloc(mid + 1);
        Bh = (char *)malloc(mid + 1);
        Bl = (char *)malloc(mid + 1);

        if (Ah == NULL || Al == NULL || Bh == NULL || Bl == NULL) {
            fprintf(stderr, "Memory allocation error\n");
            exit(EXIT_FAILURE);
        }
        splitNumbers(inputA, inputB, Ah, Al, Bh, Bl);
        // printf("%s\n%s\n%s\n%s\n", Ah, Al, Bh, Bl);

        // Create a pipe for each child process
        int pipefd[4][4];

        for (int i = 0; i < 4; i++) {
            if (pipe(pipefd[i]) == -1) {
                fprintf(stderr, "Pipe creation error\n");
                exit(EXIT_FAILURE);
            }
            if (pipe(pipefd[i] + 2) == -1) {
                fprintf(stderr, "Pipe creation error\n");
                exit(EXIT_FAILURE);
            }
        }

        // Fork child processes
        pid_t child[4];

        for (int i = 0; i < 4; i++) {
            pid_t pid = fork();
            if (pid == -1) {
                fprintf(stderr, "Error forking child process\n");
                exit(EXIT_FAILURE);
            }
            if (pid == 0) { // Child process
                if (dup2(pipefd[i][0], STDIN_FILENO) == -1) {
                    fprintf(stderr, "Error in dup2 pipe1\n");
                    exit(EXIT_FAILURE);
                }
                if (dup2(pipefd[i][3], STDOUT_FILENO) == -1) {
                    fprintf(stderr, "Error in dup2 pipe2\n");
                    exit(EXIT_FAILURE);
                }

                for (int j = 0; j < 4; j++) {
                    for (int k = 0; k < 4; k++) {
                        close(pipefd[j][k]);
                    }
                }
                execlp(argv[0], argv[0], NULL);

                // If execlp fails
                fprintf(stderr, "Error in execlp: %s\n", strerror(errno));
                exit(EXIT_FAILURE);

            } else {
                child[i] = pid;
            }
        }
        // close parent pipes (handle the error or exit the program)
        for (int i = 0; i < 4; i++) {
            if (close(pipefd[i][0]) == -1) {
                fprintf(stderr, "Error closing read end of pipe %d\n", i);
            }

            if (close(pipefd[i][3]) == -1) {
                fprintf(stderr, "Error closing write end of pipe %d\n", i);
            }
        }

        // Send input to children
        for (int i = 0; i < 4; i++) {
            if (i == 0)
                sendInputToChild(pipefd[i][1], Ah, Bh);
            if (i == 1)
                sendInputToChild(pipefd[i][1], Ah, Bl);
            if (i == 2)
                sendInputToChild(pipefd[i][1], Al, Bh);
            if (i == 3)
                sendInputToChild(pipefd[i][1], Al, Bl);
        }

        // Wait for all child processes to finish
        waitForChildren(child, 4);

        // Read intermediate results from children
        char *intermediateResults[4];
        size_t resultSize = lenA;

        // Allocate memory for intermediate results
        for (int i = 0; i < 4; i++) {
            intermediateResults[i] = (char *)malloc(sizeof(char)  *lenA + lenA);
            if (intermediateResults[i] == NULL) {
                fprintf(stderr, "Error allocating memory for intermediateResults: %s\n", strerror(errno));
                exit(EXIT_FAILURE);
            }
        }

        // Read data from pipes into allocated buffers
        for (int i = 0; i < 4; i++) {
            FILE *readStream = fdopen(pipefd[i][2], "r");
            if (readStream == NULL) {
                fprintf(stderr, "Error opening readStream: %s\n", strerror(errno));
                exit(EXIT_FAILURE);
            }

            ssize_t bytesRead = getline(&intermediateResults[i], &resultSize, readStream);
            if (bytesRead == -1) {
                fprintf(stderr, "Error reading from pipe: %s\n", strerror(errno));
                exit(EXIT_FAILURE);
            }

            fclose(readStream);
        }

        // Perform shifting
        int maxStringLength = 2 * lenA;
        char shiftedResults[4][maxStringLength];

        for (int i = 0; i < 4; i++) {
            int currentStringLength = lenA;
            int zerosBefore = lenA / 2;

            if (i == 0) { // Add zeros at the end
                for (int j = 0; j < currentStringLength; j++) {
                    shiftedResults[i][j] = intermediateResults[i][j];
                }
                for (int j = currentStringLength; j < maxStringLength; j++) {
                    shiftedResults[i][j] = '0';
                }
            } else if (i == 1 || i == 2) { // Add zeros at the beginning and end
                for (int j = 0; j < zerosBefore; j++) {
                    shiftedResults[i][j] = '0';
                }
                for (int j = zerosBefore; j < zerosBefore + currentStringLength; j++) {
                    shiftedResults[i][j] = intermediateResults[i][j - zerosBefore];
                }
                for (int j = zerosBefore + currentStringLength; j < maxStringLength; j++) {
                    shiftedResults[i][j] = '0';
                }
            } else if (i == 3) { // Add zeros at the beginning
                for (int j = 0; j < lenA; j++) {
                    shiftedResults[i][j] = '0';
                }
                for (int j = lenA; j < maxStringLength; j++) {
                    shiftedResults[i][j] = intermediateResults[i][j - lenA];
                }
            }
        }
        // Error handling for memory allocation
        for (int i = 0; i < 4; i++) {
            if (shiftedResults[i] == NULL) {
                fprintf(stderr, "Error allocating memory for shiftedResults[%d]: %s\n", i, strerror(errno));

                exit(EXIT_FAILURE);
            }
        }

        // Sum all four results to get the final result
        char final_result[2 * lenA + 1];
        final_result[2 * lenA] = '\0';
        char carry_digit = '0';

        for (int i = 2 * lenA - 1; i >= 0; i--) {
            final_result[i] = carry_digit;
            carry_digit = '0';
            for (int j = 0; j < 4; j++) {
                final_result[i] = addHexDigits(final_result[i], shiftedResults[j][i], &carry_digit);
            }
        }

        // Error handling for printf
        if (printf("%s\n", final_result) < 0) {
            fprintf(stderr, "Error printing final result: %s\n", strerror(errno));
            // Add cleanup code if necessary
            exit(EXIT_FAILURE);
        }

        // Free memory allocated for Ah, Al, Bh, Bl
        // Free memory allocated for intermediate results and split numbers
        freeIntermediateResults(intermediateResults, 4);
        freeSplitNumbers(Ah, Al, Bh, Bl);

        // Free memory for padded numbers
        free(inputA);
        free(inputB);

    } else { // handle the case when both numbers have length == 1
        if (lenA == 1) {
            *inputA = tolower(*inputA);
            *inputB = tolower(*inputB);
            int br1 = hexDigitToInt(*inputA);
            int br2 = hexDigitToInt(*inputB);
            int res = br1 * br2;
            printf("%02x\n", res);
        }

        // Free memory for padded numbers
        free(inputA);
        free(inputB);


    }

    return 0;
}