#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

/**
* Client code
* 1. Create a socket and connect to the server specified in the command arugments.
* 2. Prompt the user for input and send that input as a message to the server.
* 3. Print the message received from the server and exit the program.
*/

// Error function used for reporting issues
void error(const char *msg) { 
  perror(msg); 
  exit(0); 
} 

// Set up the address struct
void setupAddressStruct(struct sockaddr_in* address, 
                        int portNumber, 
                        char* hostname){
 
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

int main(int argc, char *argv[]) {
  int socketFD, portNumber, charsWritten, charsRead;
  struct sockaddr_in serverAddress;

  // Check usage & args
  if (argc < 3) { 
    fprintf(stderr,"USAGE: %s hostname port\n", argv[0]); 
    exit(0); 
  } 

  // Create a socket
  socketFD = socket(AF_INET, SOCK_STREAM, 0); 
  if (socketFD < 0){
    error("CLIENT: ERROR opening socket");
  }

   // Set up the server address struct
  setupAddressStruct(&serverAddress, atoi(argv[4]), argv[3]);

  // Connect to server
  if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0){
    error("CLIENT: ERROR connecting");
  }

  // Variables for reading in the text from both the input file and key
  char * file_buffer = NULL;
  char * key_buffer = NULL;
  char * buffer = NULL;
  long file_length;
  long key_length;
  long buffer_length;
  FILE *file_read;
  FILE *key_read;

  // Open the input file
    file_read = fopen(argv[1], "r+");

    // Used modfied code from this answer on reading a file into a string
    // https://stackoverflow.com/questions/174531/how-to-read-the-content-of-a-file-to-a-string-in-c
    if (file_read) {
    fseek (file_read, -1, SEEK_END);
    file_length = ftell(file_read);
    fseek (file_read, 0, SEEK_SET);
    file_buffer = malloc(file_length+1);

    // As long as the buffer exists, read into it the file info
    // Then close the file
    if (file_buffer) {
      fread (file_buffer, 1, file_length, file_read);
    }
    fclose (file_read);
  }

  // Same code as above, open the key file and read it in
  key_read = fopen(argv[2], "r+");
  if (key_read) {
    fseek (key_read, -1, SEEK_END);
    key_length = ftell(key_read);
    fseek (key_read, 0, SEEK_SET);
    key_buffer = malloc(key_length+1);

    // As long as the buffer exists, read into it the file info
    // Then close the file
    if (key_buffer) {
      fread (key_buffer, 1, key_length, key_read);
    }
    fclose (key_read);
  }

  // Check to see if the key is at least the length
  if (key_length < file_length){
    fprintf(stderr, "Error: key ‘%s’ is too short\n",argv[2]);
    exit(1);
  }

  // Set buffer length and the buffer to a size of 2x the input file
  buffer_length = file_length * 2;
  buffer = malloc((file_length * 2) + 1);

  // Copy the input file into the buffer char var
  // Then concat it with key_buffer to the length of the input file
  strcpy(buffer, file_buffer);
  strncat(buffer, key_buffer, file_length);

  // Send message to server
  // Write to the server
  charsWritten = send(socketFD, buffer, strlen(buffer), 0); 
  if (charsWritten < 0){
    error("CLIENT: ERROR writing to socket");
  }
  if (charsWritten < strlen(buffer)){
    printf("CLIENT: WARNING: Not all data written to socket!\n");
  }

  // Get return message from server
  // Clear out the buffer again for reuse
  memset(buffer, '\0', strlen(buffer));
  // Read data from the socket, leaving \0 at end
  charsRead = recv(socketFD, buffer, strlen(buffer) - 1, 0); 
  if (charsRead < 0){
    error("CLIENT: ERROR reading from socket");
  }
  printf("CLIENT: I received this from the server: \"%s\"\n", buffer);

  // Close the socket
  close(socketFD); 
  return 0;
}