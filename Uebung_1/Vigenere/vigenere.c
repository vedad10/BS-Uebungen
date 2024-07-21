/**
 * @file   vigenere.c
 * @author Vedad Hadzic (e12042758@student.tuwien.ac.at)
 * @date   29.10.2023
 *
 * @brief A program for impelementation of Vigenere chipher (https://en.wikipedia.org/wiki/Vigenere_cipher)
 *
 * @details The vigenere program is designed to read files line by line and perform encryption 
 * or decryption on their content based on a provided key. 
 * 
 * By default, the program operates in encryption mode, but if the -d option is used, it switches 
 * to decryption mode. The program must accept lines of any length and be able to process data with numbers, 
 * upper and lower-case letters, whitespace, and special characters. Uppercase and lowercase letters 
 * are processed separately and printed as uppercase or lowercase in the output, respectively. The key is 
 * case-insensitive and must only contain alphabet characters. If no input file is specified, 
 * the program reads from stdin. However, if one or more input files are provided, the program 
 * processes each of them in the order they are given. For the output, if no output file is 
 * specified (no -o option), the output is written to stdout. Otherwise, it is written to the 
 * specified outfile.

**/

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <errno.h>


static void usage(char *myprog);
void getArguments(int argc, char **argv, char *myprog, char **outfilename, char **key, bool *decrypt);
FILE* openInput(char *filename);
FILE* openOutput(char *outfile);
int isvalidkey(char *key);
int char_int(char c);
char int_char(int c, char base);
static void encryption(FILE *inputText, const char *key, FILE *outputFile);
static void decryption(FILE *inputText, const char *key, FILE *outputFile);

/**
 * @brief The main execution of the vigenere program.
 * @details This function sumup all fuctions and executes the whole program.
 * 
 * @param argc The argument counter.
 * @param argv  The argument values.
 *
 * @return Returns EXIT_FAILURE upon failure or EXIT_SUCCESS upon success .
 **/

int main(int argc, char **argv) {

    char *myprog = argv[0];
    char *outfilename = NULL;
    char *key = NULL;
    bool decrypt = false;

    getArguments(argc, argv, myprog, &outfilename, &key, &decrypt);

    // Validate the key
    if (isvalidkey(key) == -1) {
        fprintf(stderr, "Not a valid key\n");
        return EXIT_FAILURE;
    }

    FILE *outputFile = stdout;
    FILE *inputFile = NULL;

    if (outfilename != NULL) {
        
        outputFile = openOutput(outfilename);  //open output for writing

        if (outputFile == NULL) {
            fprintf(stderr, "Error: Failed to open the output file for writing: %s, %s \n", outfilename, strerror(errno));
            return EXIT_FAILURE;
        }
    }

    int inputFilesStart = optind + 1; // Index of the first input file

    if (inputFilesStart < argc) {  // We have more input files
        
        for (int i = inputFilesStart; i < argc; i++) {
            inputFile = openInput(argv[i]);

            if (inputFile != NULL) {

                if (!decrypt) {
                    encryption(inputFile, key, outputFile);
                    }   
                else {
                    decryption(inputFile, key, outputFile);
                }

                fclose(inputFile);  // Close the input file if it was opened
            }
            else {
                fprintf(stderr, "Error: Fails to open input file for writing: %s , %s \n",argv[i], strerror(errno));
                if(outfilename != NULL)
                {
                    fclose(outputFile);
                }
                return EXIT_FAILURE;
            }
        }
     } else {
        
        if (!decrypt) {
            encryption(stdin, key, outputFile);
        } else {
            decryption(stdin, key, outputFile);
        }
    }

    if (outfilename != NULL) {
        fclose(outputFile); 

    return EXIT_SUCCESS;
}
}


/**
 * @brief Opens an input file with the read-only 
 *
 * @param filename The name of the file to be opened.
 * 
 * @return A pointer to a FILE object or NULL
 **/

FILE* openInput(char *filename) {
    FILE *file = fopen(filename, "r");
    return file;
}

/**
 * @brief Opens an output file with the writing mode 
 *
 * @param outfile The name of the file to be opened.
 * 
 * @return A pointer to a FILE object or NULL
 **/

FILE* openOutput(char *outfile) {
    FILE *file = fopen(outfile, "w");
    return file;
}

/**
 * @brief Display the user message for the program.
 *
 * @param myprog String containing the name of the program
 * 
 * @return EXIT_FAILURE
 **/

static void usage(char *myprog) {
    fprintf(stderr, "Usage: %s [-d] [-o outfile] key [file...]\n", myprog);
    exit(EXIT_FAILURE);
}

/**
 * @brief Parses and validates command-line arguments passed to the program.
 *  
 * @details It uses the getopt function to process command-line options ( -d and -o) 
 * and their corresponding arguments.
 * 
 * @param argc The argument counter.
 * @param argv  The argument values.
 * @param myprog The name of the program.
 * @param filename A pointer to a string where the output file name is stored.
 * @param key  A pointer to a string where the key is stored.
 * @param decrypt A pointer to the boolean flag that indicates decryption mode.
 * 
 * @return EXIT_FAILURE or EXIT_SUCCESS
 **/

void getArguments(int argc, char **argv, char *myprog, char **filename, char **key, bool *decrypt) {
    
    int opt;
    int counto = 0;
    int countd = 0;

    while ((opt = getopt(argc, argv, "do:")) != -1) {

        switch (opt) {

            case 'd':  //Decryption mode
                counto++;
                *decrypt = true;
                break;

            case 'o': //Output specified 
                countd++;
                *filename = optarg;
                break;

            case '?': //missing argument
                usage(myprog);
                break;

            default: 
                assert(0);
        }
    }

    if (counto > 1 || countd > 1) {   // Check if -d' or '-o' are provided more than once.
        usage(myprog);
    }

    int keyPos = optind; 

    if (keyPos >= argc || !argv[keyPos]) {  
        usage(myprog);
    }

    *key = argv[keyPos];  // Parse and validate the key
}


/**
 * @brief Check if the key is valid
 *
 * @param key The key to be validated.
 * 
 * @return A-1 if the Key is invalid, otherwise 0
 **/

int isvalidkey(char *key) {
    for (int i = 0; i < strlen(key); i++) {
        if (!((key[i] >= 'a' && key[i] <= 'z') || (key[i] >= 'A' && key[i] <= 'Z'))){
            return -1;
        }
    }
    return 0;
}

/**
 * @brief Covert the characters to the integers.
 *
 * @param c Character to be coverted
 * 
 * @return coverted caracter or c when is the special character
 **/

int char_int(char c) {
    if (c >= 'a' && c <= 'z') {
        return c - 'a';
    } else if (c >= 'A' && c <= 'Z') {
        return c - 'A';
    }
    return c;
}

/**
 * @brief Covert integers back to the characters.
 *
 * @param c Integer to be converted.
 * @param base Indicates when the output character should be in lowercase ('a') or uppercase ('A'). 
 * 
 * @return coverted caracter or c when is the special character
 **/

char int_char(int c, char base) {
    if (base >= 'a' && base <= 'z') {
        if (c >= 0 && c <= 25) {
            return 'a' + c; // Use 'a' as the base character for lowercase
        }
    } else if (base >= 'A' && base <= 'Z') {
        if (c >= 0 && c <= 25) {
            return 'A' + c; // Use 'A' as the base character for uppercase
        }
    }
    return c; // Return as is for non-alphabet characters
}

/**
 * @brief Function that implements encryption on the of an input text with the given key 
 * and write the encrypted result to an output file or stdout.
 *
 * @param inputText A pointer to the input text file.
 * @param key The key for the encryption process. 
 * @param outputFile A pointer to the output file.
 * 
 **/

static void encryption(FILE *inputText, const char *key, FILE *outputFile) {

    int key_length = strlen(key);
    char *line = NULL;
    size_t len = 0;

    if (inputText == NULL) { // If the input file failed to open.

        fprintf(stderr, "Error: Failed to open input file.\n");

        return;
    }

    while (getline(&line, &len, inputText) != -1) {  // Loop to read the input text line by line.
        
        for (int i = 0; i < len; i++) {

            char currLetter = line[i];
            int currInt = char_int(currLetter);
            int keyInt = char_int(key[i % key_length]);

            if (currLetter >= 'A' && currLetter <= 'Z') { 
                line[i] = int_char((currInt + keyInt) % 26, 'A'); //Perform encryption with the formula: E[i] = (L[i] + K[i mod lK]) mod 26.
                
            }

            if (currLetter >= 'a' && currLetter <= 'z') {
                line[i] = int_char((currInt + keyInt) % 26, 'a'); //Perform encryption with the formula: E[i] = (L[i] + K[i mod lK]) mod 26.
               
            }
            
        }
        fprintf(outputFile, "%s", line); // Write to the specified output file
    }

    free(line);
}

/**
 * @brief Function that implements decryption on the of an input text with the given key 
 * and write the encrypted result to an output file or stdout.
 *
 * @param inputText A pointer to the input text file.
 * @param key The key for the encryption process. 
 * @param outputFile A pointer to the output file.
 * 
 **/

static void decryption(FILE *inputText, const char *key, FILE *outputFile) {

    int key_length = strlen(key);
    char *line = NULL;
    size_t len = 0;

    if (inputText == NULL) {  // If the input file failed to open.
        fprintf(stderr, "Error: Failed to open input file.\n");
        return;
    }

    while (getline(&line, &len, inputText) != -1) {  // Loop to read the input text line by line.
 
        for (int i = 0; i < len; i++) {
            char currLetter = line[i];

            if (currLetter >= 'A' && currLetter <= 'Z') {
                int CurrInt = char_int(currLetter);
                int keyInt = char_int(key[i % key_length]);
                line[i] = int_char((CurrInt - keyInt + 26) % 26, 'A'); //Perform decryption with the formula: D[i] = (L[i] −K[i mod lK] + 26) mod 26.
               
            } else if (currLetter >= 'a' && currLetter <= 'z') {
                int CurrInt = char_int(currLetter);
                int keyInt = char_int(key[i % key_length]);
                line[i] = int_char((CurrInt - keyInt + 26) % 26, 'a'); //Perform decryption with the formula: D[i] = (L[i] −K[i mod lK] + 26) mod 26.
                
            }
            
        }
        fprintf(outputFile, "%s", line); // Write to the specified output file
    }

    free(line);
}
