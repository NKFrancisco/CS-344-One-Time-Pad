/* 
 * Author: Nick Francisco
 * Class: CS 344
 * Program: OTP - dec_client
 *
 * Description: Sends request to dec_server to decode
 * given file using the given key for a one time pad 
 * decryption.   
 *
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>  
#include <sys/socket.h> 
#include <netdb.h>      
#include <stdbool.h>
#include <ctype.h>
                        

// Prototypes 
int sendall(int s, char *buf, int *len);
int readall(int s, char *buf, int *len);
char* checkFile(char *file);
void setupAddressStruct(struct sockaddr_in* address, int portNumber, char* hostname);

 
int main(int argc, char *argv[]) {

  // Socket
  int socketFD;
  // Socket port number
  int portNumber;
  // Server adress struct
  struct sockaddr_in serverAddress;
  // Input buffer
  char buffer[1024];
  // Program name
  char *programName = "dec_client";
  // Server name
  char *serverName = "dec_server";
  // Length of programName
  int programNameLen = strlen(programName);


  // Check usage & args 
  if (argc < 4) {
    fprintf(stderr, "USAGE: %s plaintext key port\n", argv[0]);
    exit(0);
  }


  // Validate files 
  

  // Names of files
  char *keyName = argv[2];
  char *encodedName = argv[1];               
  // Open, read, and validate key and encoded     
  char *key = checkFile(keyName);
  char *encoded = checkFile(encodedName);
  // Get file lengths
  int keyLen = strlen(key);
  int encodedLen = strlen(encoded);


  // Check if key is too short
  if(strlen(key) < strlen(encoded)) {
    fprintf(stderr, "Error: key '%s' is too short\n", argv[2]);
    exit(1);
  }
  
  // Truncate key if longer than encoded
  if (keyLen > encodedLen) {
    int diff = keyLen - encodedLen;
    key[keyLen - diff] = '\0';
    keyLen = keyLen - diff;
  }


  // Set up socket 


  // Create socketFD 
  socketFD = socket(AF_INET, SOCK_STREAM, 0); 
  // Error 
  if (socketFD < 0){
    fprintf(stderr, "CLIENT: ERROR opening socket\n");
    exit(0);
  }

  // Set up the server address struct with localhost 
  setupAddressStruct(&serverAddress, atoi(argv[3]), "localhost");

  // Try to connect to server
  if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0){
    // Error connecting to server
    fprintf(stderr, "CLIENT: ERROR connecting");
  }


  // Confirm server


  // Send program name for verification
  int ver_to_serv = sendall(socketFD, programName, &programNameLen);
  // Error 
  if (ver_to_serv == -1) {
    fprintf(stderr, "enc_client error: ERROR writing to socket\n");
    exit(0);
  }


  // Read the client's response message
  memset(buffer, '\0', 1024);
  int ver_from_server = recv(socketFD, buffer, 1023, 0); 
  // Error
  if (ver_from_server < 0){
    fprintf(stderr, "ERROR reading from socket\n");
    exit(1);
  }
  

  // Look for reject message 
  if (strcmp(buffer, "rej_client") == 0) {
    // Server rejected client exiting
    fprintf(stderr, "dec_client error: cannot use 'enc_server'\n");
    exit(2);
  }


  // Compare response to server name
  if (strcmp(buffer, serverName) != 0) {
    // Could not verify server exiting 
    fprintf(stderr, "dec_client error: Could not verify server at port %s\n", argv[3]);
    exit(2);
  }


  // Send data


  // Send string of encoded length
  int lenStringLen = 10;
  char lenString[10];
  sprintf(lenString, "%d", encodedLen);


  // Send encoded and key length
  int len_to_serv = sendall(socketFD, lenString, &lenStringLen);
  // Error  
  if (len_to_serv == -1) {
    fprintf(stderr, "enc_client error: ERROR writing to socket\n");
    exit(0);
  }


  // Send encoded   
  int plaintext_to_serv = sendall(socketFD, encoded, &encodedLen);
  // Error  
  if (plaintext_to_serv == -1) {
    fprintf(stderr, "enc_client error: ERROR writing to socket\n");
    exit(0);
  }


  // Send key 
  int key_to_serv = sendall(socketFD, key, &keyLen);
  // Error  
  if (key_to_serv == -1) {
    fprintf(stderr, "enc_client error: ERROR writing to socket\n");
    exit(0);
  }


  // Get data


  // Allocate space for response  
  char *decoded;
  decoded = (char*)malloc(encodedLen * sizeof(char));
  int decodedToRead = encodedLen;


  // Get decoded response
  int decoded_from_server = readall(socketFD, decoded, &decodedToRead);
  // Error 
  if (decoded_from_server < 0){
    fprintf(stderr, "ERROR reading from socket");
    exit(1);
  }

  // Print decoded to stdout with \n
  printf("%s\n", decoded);
  fflush(stdout);


  // Close FD and free memory
  close(socketFD);
  free(encoded);
  free(key);
  free(decoded);

}


/* 
 * Function: checkFile  
 *
 * Description: Checks given file by trying to open, 
 * reads from file, and validates chars in file.
 *
 */
char* checkFile(char *file){
  
  // File
  FILE *in = fopen(file, "r");
  // Input string 
  char *inString; 
  // Input string length
  int inLen = 0;

  // Error opening
  if (in == NULL) {
    fprintf(stderr, "enc_client error: Could not open file %s.\n", file);
    exit(0);
  }
  
  // Get file length
  fseek(in, 0, SEEK_END);
  inLen = ftell(in);
  rewind(in);

  // Set array size
  inString = (char*)malloc(inLen * sizeof(char));

  // Read file into variable
  fread(inString, inLen, 1, in);

  // Replace \n at enf of file with null terminator
  inString[inLen - 1] = '\0';

  // Validate characters 
  for (int i = 0; i < inLen - 1; i++) {
    // Check if current char is A-Z or ' ' 
    if (!(isupper(inString[i]) != 0 || inString[i] == ' ')) {
      fprintf(stderr, "enc_client error: input contains bad character at index: %d", i);
      exit(1);
    }
  }

  // Close file
  fclose(in);

  // Return read string   
  return inString;

}


/* 
 * Function: sendall  
 *
 * Description: While loop to send buffer over 
 * socket s using buffer length to ensure all
 * data is sent.
 *
 * Source: https://beej.us/guide/bgnet/html/#sendall
 *
 */
int sendall (int s, char *buf, int *len) {

    int total = 0;        // how many bytes we've sent
    int bytesleft = *len; // how many we have left to send
    int n;

    while(total < *len) {
        n = send(s, buf+total, bytesleft, 0);
        if (n == -1) { break; }
        total += n;
        bytesleft -= n;
    }

    *len = total; // return number actually sent here

    return n==-1?-1:0; // return -1 on failure, 0 on success
}


/* 
 * Function: readall  
 *
 * Description: While loop to read buffer over 
 * socket s using buffer length to ensure all
 * data is read.
 *
 * Source: https://beej.us/guide/bgnet/html/#sendall
 *
 */
int readall(int s, char *buf, int *len) {
  
    int total = 0;        // how many bytes we've read
    int bytesleft = *len; // how many we have left to read
    int n;

    while(total < *len) {
        n = recv(s, buf+total, bytesleft, 0);
        if (n == -1) { break; }
        total += n;
        bytesleft -= n;
    }

    *len = total; // return number actually sent here

    return n==-1?-1:0; // return -1 on failure, 0 on success

}


/* 
 * Function: setupAddressStruct 
 *
 * Description: Sets uo the adress struct
 * 
 * Source: Class module 
 *
 */
void setupAddressStruct(struct sockaddr_in* address, int portNumber, char* hostname){
 
  // Clear out the address struct
  memset((char*) address, '\0', sizeof(*address)); 

  // The address should be network capable
  address->sin_family = AF_INET;
  // Store the port number
  address->sin_port = htons(portNumber);

  // Get the DNS entry for this host name
  struct hostent* hostInfo = gethostbyname(hostname); 
  if (hostInfo == NULL) { 
    fprintf(stderr, "CLIENT: ERROR, no such host\n"); 
    exit(0); 
  }

  // Copy the first IP address from the DNS entry to sin_addr.s_addr
  memcpy((char*) &address->sin_addr.s_addr, 
        hostInfo->h_addr_list[0],
        hostInfo->h_length);
}