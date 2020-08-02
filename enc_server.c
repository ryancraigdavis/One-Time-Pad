#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <sys/wait.h>

// Set up the address struct for the server socket
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

int main(int argc, char *argv[]){

  // Various variables for connecting, buffers, and the socket
  // Connect bool is for checking that the right client is connecting
  int connectionSocket, charsRead, charsWritten;
  char buffer[150000];
  char pipe_buffer[1000];
  struct sockaddr_in serverAddress, clientAddress;
  socklen_t sizeOfClientInfo = sizeof(clientAddress);
  bool connect_bool = true;

  // Check usage & args
  if (argc < 2) { 
    fprintf(stderr,"USAGE: %s port\n", argv[0]); 
    exit(1);
  } 
  
  // Create the socket that will listen for connections
  int listenSocket = socket(AF_INET, SOCK_STREAM, 0);
  if (listenSocket < 0) {
    fprintf(stderr, "ERROR opening socket");
    exit(1);
  }

  // Set up the address struct for the server socket
  setupAddressStruct(&serverAddress, atoi(argv[1]));

  // Associate the socket to the port
  if (bind(listenSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0){
    fprintf(stderr, "ERROR on binding");
    exit(1);
  }

  // Start listening for connetions. Allow up to 5 connections to queue up
  listen(listenSocket, 5); 
  
  // Accept a connection, blocking if one is not available until one connects
  while(1){

    // Accept the connection request which creates a connection socket
    connectionSocket = accept(listenSocket, (struct sockaddr *)&clientAddress, &sizeOfClientInfo); 
    if (connectionSocket < 0){
      fprintf(stderr, "ERROR on accept");
    }

    // Set the buffer to null terminated chars, creates a socket buffer to
    // read in chunks, and sets beginning_read to true
    memset(buffer, '\0', 150000);
    char socket_buffer[1000];
    bool beginning_read = true;

    // Read the client's message from the socket using a loop until
    // the terminating chars are found
    while (strstr(buffer, "@@") == NULL){

      // First clear the socket buffer
      memset(socket_buffer, '\0', 1000);

      // Read in the chunk from the socket
      charsRead = recv(connectionSocket, socket_buffer, sizeof(socket_buffer) - 1, 0); 
      if (charsRead < 0){
        fprintf(stderr, "ERROR reading from socket\n");
      }

      // Checks to make sure that the connection is coming from the ENC client
      // First checks to see if beginning read is true
      if (beginning_read == true) {
        char check_source[4];
        strncpy(check_source, socket_buffer, 3);
        int cmp_enc = strcmp(check_source, "ENC");
        if (cmp_enc != 0) {

          // If the first few chars aren't ENC, send an error message back to the
          // client and close the connection by changing connect_bool
          charsRead = send(connectionSocket, "~~ERROR: Source is Wrong", 22, 0); 
          if (charsRead < 0){
            fprintf(stderr, "ERROR reading from socket\n");
          }

          // Set connect bool to false, this skips down to the end where the connection is closed
          connect_bool = false;
          break;

        // Otherwise, this is a valid connection, and change beginning_read to false
        // So the loop can continue reading
        } else {
          beginning_read = false;
        }
      }

      // Add the chunk to the main buffer message
      strcat(buffer, socket_buffer);
    }

    // Checks to make sure the connection is valid before proceeding
    if (connect_bool == true) {

      // Remove the terminal indicator of "@@"
      char* terminal_buff = strstr(buffer, "@@");
      *terminal_buff = '\0';

      // Remove the first 3 chars of ENC from the string
      char input_string[strlen(buffer)-2];
      memset(input_string, '\0', strlen(buffer)-2);
      strcpy(input_string, &buffer[3]);

      // The received buffer string includes both the plain text and the key in one string
      // Since each piece is the same length, the length of each piece is half of the whole
      long input_length = strlen(input_string)/2;

      // Create and null terminate a string for the plain text, key, and resulting cypher
      char plain_text[input_length+1];
      memset(plain_text, '\0', input_length+1);
      char key[input_length+1];
      memset(key, '\0', input_length+1);
      char cypher_text[input_length+3];
      memset(cypher_text, '\0', input_length+3);

      // Copy the first half of the buffer to plain text and second half to the key
      strncpy(plain_text, input_string, input_length);
      strcpy(key, &input_string[input_length]);

      // Creates the ints used to open the pipe - array is for input/output
      int pipeProc[2];
      int pipe_int;

      // Create the pipe with error check
      if (pipe(pipeProc) == -1) {
        fprintf(stderr,"Call to pipe() failed\n"); 
      }

      // Fork a child process
      pid_t childPid = fork();
      if(childPid == 0){

        // Go through each character of the text file and convert it to
        // an int 0-26, do the same with the key
        for (int i = 0; i < input_length; ++i) {
          int text_int = plain_text[i] - 65;
          if (text_int == -33) {
            text_int = 26;
          }
          int key_int = key[i] - 65;
          if (key_int == -33) {
            key_int = 26;
          }

        // Add them together then put the modulus result in cypher_text
          if (((text_int + key_int) % 27) == 26) {
            cypher_text[i] = 32;
          } else {
            cypher_text[i] = ((text_int + key_int) % 27)+65;
          }
        }

        // Add "@@" onto the end of the full buffer
        strcat(cypher_text, "@@");

        // Close the input pipe
        close(pipeProc[0]);

        // Write the entire string to the pipe
        write(pipeProc[1], cypher_text, input_length+2); 

        // Terminate the child
        exit(0);
        break;

      } else {

        // Parent process will read from the pipe, so close the output file desriptor.
        close(pipeProc[1]);

        // As long as we haven't found the terminal indicators @@
        while (strstr(cypher_text, "@@") == NULL){

          // Reset the pipe buffer
          memset(pipe_buffer, '\0', 1000);

          // Read the next chunk of data from the pipe - the '\0'
          pipe_int = read(pipeProc[0], pipe_buffer, sizeof(pipe_buffer) - 1);

          // Add the chunk we read to what we have read up to now
          strcat(cypher_text, pipe_buffer); 
          
          // Check for errors
          if (pipe_int == -1) { 
            // -1 indicates an error
            fprintf(stderr, "ERROR with reading from pipe\n"); 
            break; 
          } 
          if (pipe_int == 0) {
            // 0 indicates an error or end-of-file
            fprintf(stderr, "ERROR end of file reached\n"); 
            break; 
          }
        }
      }

      // Send the cyphertext back to the client's stdout with the @@ intact so that
      // the client knows when the buffer has ended
      charsWritten = send(connectionSocket, cypher_text, strlen(cypher_text), 0); 
      if (charsWritten < 0){
        fprintf(stderr, "ERROR writing to socket\n");
      }
      if (charsWritten < strlen(cypher_text)){
        fprintf(stderr, "SERVER: WARNING: Not all data written to socket!\n");
      }
    }

    // Close the connection socket for this client and terminates child processes
    wait(NULL);
    close(connectionSocket); 
  }
  // Close the listening socket
  close(listenSocket); 
  return 0;
}
