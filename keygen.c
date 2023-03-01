/* Author: Nick Francisco 
 * Class: CS 344
 * Program: OTP - keygen
 * Description: Generates random key for one time pad 
 *              encryption. The length is given as an 
 *              argument to this program.             
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

int main(int argc, char **argv) {

    // Characters to generate key 
    const char charArr[27] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ ";

    // Check for correct amount of args 
    if (argc != 2) {
        // print errpr message to stderr
        fprintf(stderr, "USAGE: %s keyLength", argv[0]);
        exit(0);
    }

    // Seed rand()
    srand(time(0));

    // Get key length from argv input
    int keyLen = atoi(argv[1]);

    // Print random char from charArr keyLen times
    for (int i = 0; i < keyLen; i++) {
      int r = (rand() % (26 - 0 + 1) + 0);
      printf("%c", charArr[r]);
    }

    // Print new line char
    printf("\n");

}