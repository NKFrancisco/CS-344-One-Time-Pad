/* 
 * Author: Nick Francisco
 * Class: CS 344
 * Program: OTP - enc_client
 *
 * Description: Sends request to enc_server to encode
 * given file using the given key for a one time pad 
 * encryption.   
 *
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>  
#include <sys/socket.h> 
#include <netdb.h>      
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
  char *programName = "enc_client";
  // Acepted server name
  char *serverName = "enc_server";
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
  char *plaintextName = argv[1];                 
  // Open, read, and validate key and plain text        
  char *key = checkFile(keyName);
  char *plaintext = checkFile(plaintextName);
  // Get file lengths 
  int keyLen = strlen(key);
  int plaintextLen = strlen(plaintext);


  // Check if key is too short
  if(strlen(key) < strlen(plaintext)) {
    fprintf(stderr, "Error: key '%s' is too short\n", argv[2]);
    exit(1);
  }
  
  // Truncate key if longer than plaintext
  if (keyLen > plaintextLen) {
    int diff = keyLen - plaintextLen;
    key[keyLen - diff] = '\0';
    keyLen = keyLen - diff;
  }


  // Set up socket 
  

  // Create a socket
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
  // If error sending programn name 
  if (ver_to_serv == -1) {
    fprintf(stderr, "enc_client error: ERROR writing to socket\n");
    exit(0);
  }


  // Read the client's message 
  memset(buffer, '\0', 1024);
  int charsRead = recv(socketFD, buffer, 1023, 0); 
  // Error receving
  if (charsRead < 0){
    fprintf(stderr, "ERROR reading from socket\n");
    exit(1);
  }


  // Look for reject message
  if (strcmp(buffer, "rej_client") != 0) {
    // Server rejected client
    fprintf(stderr, "enc_client error: cannot use 'enc_server'\n");
    exit(2);
  }


  // Compare response to server name
  if (strcmp(buffer, serverName) != 0) {
    // Could not verify server exiting 
    fprintf(stderr, "enc_client error: Coult not verify server at port %s\n", argv[3]);
    exit(2);
  }


  // Send data


  // Set string of plaintext length
  int lenStringLen = 10;
  char lenString[10];
  sprintf(lenString, "%d", plaintextLen);


  // Send plaintext and key length
  int len_to_serv = sendall(socketFD, lenString, &lenStringLen);
  // If error sending plaintext length 
  if (len_to_serv == -1) {
    fprintf(stderr, "enc_client error: ERROR writing to socket\n");
    exit(0);
  }


  // Send plaintext 
  int plaintext_to_serv = sendall(socketFD, plaintext, &plaintextLen);
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
  char *encoded;
  encoded = (char*)malloc(plaintextLen * sizeof(char));
  int encodedToRead = plaintextLen;


  // Get encoded response
  int encoded_from_serv = readall(socketFD, encoded, &encodedToRead);
  // Error
  if (encoded_from_serv == -1) {
    fprintf(stderr, "enc_client error: ERROR writing to socket\n");
    exit(0);
  }


  // Print encoded to stdout
  printf("%s\n", encoded);
  fflush(stdout);


  // COLOSE FDs and FREE memory
  close(socketFD);
  free(plaintext);
  free(key);
  free(encoded);


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

  //printf("readall\n");
  //fflush(stdout);
  
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