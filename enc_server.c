/* 
 * Author: Nick Francisco
 * Class: CS 344
 * Program: OTP - enc_server
 *
 * Description: Encodes data from client using
 * given key for one time pad encryption.   
 *
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/wait.h>


// Function prototypes
int sendall(int s, char *buf, int *len);
int readall(int s, char *buf, int *len);
void encode(char *plaintext, char *key);
int charToNum(char c);
void setupAddressStruct(struct sockaddr_in* address, int portNumber);


int main(int argc, char *argv[]){

  // Socket
  int connectionSocket;
  // Input buffer
  char buffer[1024];
  // Server 
  struct sockaddr_in serverAddress;
  // Client 
  struct sockaddr_in clientAddress;
  // Client socket size 
  socklen_t sizeOfClientInfo = sizeof(clientAddress);
  // Client name
  char *clientName = "enc_client";
  // Number of connections
  char numConnections = 0;

  // Process count
  int processCount = 0;
  // Total processes for rollover with %
  int totalProcessCount = 0;
  // Process pid array
  int processes[5] = {-1, -1, -1, -1, -1};
  // Child status
  int childStatus;
  // Parent pid
  pid_t parentPid = getpid();


  // Check usage & args
  if (argc < 2) { 
    fprintf(stderr,"USAGE: %s port\n", argv[0]); 
    exit(1);
  } 


  // Set up socket

  
  // Create the socket that will listen for connections
  int listenSocket = socket(AF_INET, SOCK_STREAM, 0);
  // Error
  if (listenSocket < 0) {
    fprintf(stderr, "enc_server: ERROR opening socket");
    exit(1);
  }

  // Set up the address struct for the server socket
  setupAddressStruct(&serverAddress, atoi(argv[1]));

  // Associate the socket to the port
  if (bind(listenSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0){
    fprintf(stderr, "enc_server: ERROR on binding");
    exit(1);
  }

  // Start listening for connetions. Allow up to 5 connections to queue up
  listen(listenSocket, 5); 


 
  // Infinite loop for parent  
  while(getpid() == parentPid){


    // Check for completed processes
    if (processCount > 0) {
      // Loop through processes and wait for one to end 
      for (int i = 0; i < 5; i++) {
        // Check if processes[i] has pid, skip if -1
        if (processes[i] != -1) {
          // 0 if child hasn't terminated
          pid_t spawnPid = waitpid(processes[i], &childStatus, WNOHANG);
          // Not 0, child terminated
          if (spawnPid != 0) {
            // Replace pid
            processes[i] = -1;
            // Decrese counts
            processCount--;
            numConnections--;
          }
        }
      } // for ( i < 5 )
    } // if processCount > 0
    

    // Create new connection if room is avaliable 
    if (numConnections < 5) {


      // Wait for connection 
      
      
      // New socketFD
      connectionSocket = accept(listenSocket, (struct sockaddr *)&clientAddress, &sizeOfClientInfo); 
      // Error
      if (connectionSocket < 0){
        fprintf(stderr, "enc_server: ERROR on accept");
      }
            

      // Creating child for request
      pid_t childPid = fork();

      // Fork Error 
      if (childPid == -1) {
        fprintf(stderr, "enc_server: ERROR fork()");
        close(connectionSocket);
		exit(1);
      }


      // Child Process
      else if (childPid == 0) {


        // Close listening socket
        close(listenSocket);
        // String variables
        char *plaintext;
        char *key;
        //Client port numbnr
        int clientPortNum = ntohs(clientAddress.sin_port);


        // Verify client 
        

        // Read the client's message from the socket
        memset(buffer, '\0', 1024);
        int ver_from_client = recv(connectionSocket, buffer, 1023, 0); 
        // Error  
        if (ver_from_client < 0){
          fprintf(stderr, "ERROR reading from socket\n");
          exit(1);
        }
        
        // Compare response to client name
        if (strcmp(buffer, clientName) != 0) {

          // Check if dec_client is calling
          if (strcmp(buffer, "dec_client") == 0) {

            //Reject message  
            char *rejectMes = "rej_client";
            int rejMesLen = strlen(rejectMes);

             // Send reject message  
            int rej_to_client = sendall(connectionSocket, rejectMes, &rejMesLen);
            // Error
            if (rej_to_client == -1) {
              fprintf(stderr, "enc_server error: ERROR writing to socket\n");
              exit(0);
            }

            // Error dec_client request exiting
            fprintf(stderr, "enc_server error: cannot use 'dec_client' %d\n", clientPortNum);
            exit(2);

          }

          // Error other client 
          fprintf(stderr, "enc_server error: Coult not verify client at port %d\n", clientPortNum);
          exit(2);
        }


        // Sending server name to client 
        char *message = "enc_server";
        int mesLen = strlen(message);

        // Send response 
        int name_to_client = sendall(connectionSocket, message, &mesLen); 
        // Error
        if (name_to_client == -1) {
          fprintf(stderr, "enc_server error: ERROR writing to socket encoded server name");
          exit(0);
        }
        

        // Get length 
        

        // Read the client's length message from the socket
         memset(buffer, '\0', 1024);
        int len_from_client = recv(connectionSocket, buffer, 1023, 0); 
        // Error 
        if (len_from_client < 0){
          fprintf(stderr, "ERROR reading from socket");
          exit(1);
        }


        // Allocate memory 


        // Set char array lengths
        int len = atoi(buffer);
        int plaintextLen = len;
        int keyLen = len;
        plaintext = (char*)malloc(len * sizeof(char));
        memset(plaintext, '\0', len);
        key = (char*)malloc(len * sizeof(char));
        memset(key, '\0', len);


        // Get Data


        // Get plaintext
        int plaintext_from_client = readall(connectionSocket, plaintext, &plaintextLen);
        // Error 
        if (plaintext_from_client < 0){
          fprintf(stderr, "ERROR reading from socket\n");
          exit(1);
        }
      

        // Get key
        int key_from_client = readall(connectionSocket, key, &keyLen);
        // Error
        if (key_from_client < 0){
          fprintf(stderr, "ERROR reading from socket\n");
          exit(1);
        }


        // Encode
        encode(plaintext, key);


        // Send data back


        // Send encoded string back to client 
        int encodedSendLen = len;
        int encoded_to_client = sendall(connectionSocket, plaintext, &encodedSendLen);
        // Error
        if (encoded_to_client == -1) {
          fprintf(stderr, "enc_server error: ERROR writing to socket encoded\n");
          exit(0);
        }


        // Close the connection socket for this client
        close(connectionSocket);

        // FREE
        free(plaintext);
        free(key);
        exit(0);

      }         


      // Parent process  
      else {

        // Get index to store at using % to roll over or find avaliable
        int rolloverIndex = totalProcessCount % 5;
        int avaliableIndex = rolloverIndex;
        // Find open spot in processes 
        for (int i = 0; i < 5; i++) {
          if (processes[i] == -1) {
            avaliableIndex = i;
          }
        }

        // Add childProcess to array 
        processes[avaliableIndex] = childPid;

        // Increment counters 
        processCount++;
        totalProcessCount++;
        numConnections++;

        // WNOHANG specified. If the child hasn't terminated, waitpid will immediately return with value 0
        childPid = waitpid(childPid, &childStatus, WNOHANG);


      } // Parent process
  
    } // if (processCount < 5)

  } // while (getpid() == parentPid)
    

  // Close the listening socket
  close(listenSocket); 
  return 0;

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
int sendall(int s, char *buf, int *len) {

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
 * Function: encode  
 *
 * Description: Encodes plaintext key to generate 
 * one time pad encryption.
 *
 * Source: https://beej.us/guide/bgnet/html/#sendall
 *
 */
void encode(char *plaintext, char *key) {

  const char charArr[27] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ ";
  int len = strlen(plaintext);

  for (int i = 0; i < len; i++) {
    // Get plaintext char number 
    int charToEncodeNum = charToNum(plaintext[i]);
    // Get key char number 
    int keyToNum = charToNum(key[i]);
    // Encoded num
    int encodeNum = (charToEncodeNum + keyToNum) % 27;

    plaintext[i] = charArr[encodeNum];     
  }
}


/* 
 * Function: charToNum  
 *
 * Description: Gets num equivilant to char
 * for encoding.
 *  
 */
int charToNum (char c) {
  
  const char charArr[27] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ ";
  
  // Loop to find incex of char to get num 
  for (int i = 0; i < 27; i++) {
    if (charArr[i] == c){
      return i;
    }
  }

  // Error
  return -1;
}


/* 
 * Function: setupAddressStruct 
 *
 * Description: Sets uo the adress struct
 * 
 * Source: Class module 
 *
 */
void setupAddressStruct(struct sockaddr_in* address, int portNumber){
 
  // Clear out the address struct
  memset((char*) address, '\0', sizeof(*address)); 

  // The address should be network capable
  address->sin_family = AF_INET;
  // Store the port number
  address->sin_port = htons(portNumber);
  // Allow a client at any address to connect to this server
  address->sin_addr.s_addr = INADDR_ANY;
}