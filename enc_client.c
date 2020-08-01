#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdbool.h>

// Set up the address struct
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
    exit(1); 
  }

  // Copy the first IP address from the DNS entry to sin_addr.s_addr
  memcpy((char*) &address->sin_addr.s_addr, hostInfo->h_addr_list[0], hostInfo->h_length);
}

int main(int argc, char *argv[]) {

  // Variables for reading in the text from both the input file and key
  char * file_buffer = NULL;
  char * key_buffer = NULL;
  char * buffer = NULL;
  long file_length;
  long key_length;
  long buffer_length;

  // File variables for the text file and key
  FILE *file_read;
  FILE *key_read;

  // Open the input file
  file_read = fopen(argv[1], "r+");

  // Used modfied code from this answer on reading a file into a string
  // https://stackoverflow.com/questions/174531/how-to-read-the-content-of-a-file-to-a-string-in-c
  if (file_read) {
    fseek (file_read, 0, SEEK_END);
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

  // Remove the new line char at the end of the string and dec the file length by 1
  if (strstr(file_buffer, "\n") != NULL) {
    char* new_line = strstr(file_buffer, "\n");
    *new_line = '\0';
    file_length = file_length - 1;
  }

  // For loop goes through all of the input characters and verifies they aren't bad
  for (int i = 0; i < file_length; ++i) {
    int check_int = file_buffer[i] - 65;

    // If the check int is below 0 and not -33 (space) then it is bad
    if (check_int < 0 && check_int != -33) {
      fprintf(stderr, "enc_client error: input contains bad characters\n");
      exit(1);

    // Or if the int is above 25
    } else if (check_int > 25) {
      fprintf(stderr, "enc_client error: input contains bad characters\n");
      exit(1);
    }
  }

  // Same code as above, open the key file and read it in
  key_read = fopen(argv[2], "r+");
  if (key_read) {
    fseek (key_read, 0, SEEK_END);
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

    // Remove the new line char at the end of the string and dec the key length by 1
  if (strstr(key_buffer, "\n") != NULL) {
    char* new_line_k = strstr(key_buffer, "\n");
    *new_line_k = '\0';
    key_length = key_length - 1;
  }

  // Check to see if the key is at least the length of the file
  if (key_length < file_length){
    fprintf(stderr, "Error: key ‘%s’ is too short\n",argv[2]);
    exit(1);
  }

  // Set buffer length and the buffer to a size of 2x the input file
  // Buffer will start with a string ENC to differentiate it from the Dec client
  // Buffer will end with @@ that will indicate the end of the string
  buffer_length = (file_length * 2) + 5;
  buffer = malloc((file_length * 2) + 6);

  // Copy in ENC first
  strcpy(buffer, "ENC");

  // Concat the input file into the buffer char var
  // Then concat it with key_buffer to the length of the input file
  strcat(buffer, file_buffer);
  strncat(buffer, key_buffer, file_length);

  // Finally add on the terminating chars
  strcat(buffer, "@@");

  // Create ints for reading/writing into the socket
  // Create the socket address struct
  int socketFD, portNumber, charsWritten, charsRead;
  struct sockaddr_in serverAddress;

  // Check usage & args - that there are enough arguments
  if (argc < 2) { 
    fprintf(stderr, "USAGE: %s hostname port\n", argv[0]); 
    exit(1); 
  } 

  // Create a socket
  socketFD = socket(AF_INET, SOCK_STREAM, 0); 
  if (socketFD < 0){
    fprintf(stderr, "CLIENT: ERROR opening socket");
    exit(1);
  }

   // Set up the server address struct
  setupAddressStruct(&serverAddress, atoi(argv[3]), "localhost");

  // Connect to server
  if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0){
    fprintf(stderr, "CLIENT: ERROR connecting");
    exit(1);
  }

  // Write to the server checking to make sure all the chars were written
  charsWritten = send(socketFD, buffer, strlen(buffer), 0);
  if (charsWritten < 0){
    fprintf(stderr, "CLIENT: ERROR writing to socket");
  }
  if (charsWritten < strlen(buffer)){
    fprintf(stderr, "CLIENT: WARNING: Not all data written to socket!\n");
  }

  // Set buffer to null terminated chars, creates a socket buffer to read in chunks
  memset(buffer, '\0', buffer_length);
  char socket_buffer[1000];

  // Read the server's response from the socket using a loop until
  // The terminating chars are found
  while (strstr(buffer, "@@") == NULL){

    // First check to see if the connection error message was sent
    // This is an indication that there was a wrong client/server connection
    // Print to stderr, and then exit(2)
    if (strstr(buffer, "~~") != NULL) {
      fprintf(stderr, "Error: could not contact enc_server on port %s\n", argv[3]);
      exit(2);
    }

    // First clear the socket buffer
    memset(socket_buffer, '\0', 1000);

    // Read in the chunk from the socket
    charsRead = recv(socketFD, socket_buffer, sizeof(socket_buffer) - 1, 0); 
    if (charsRead < 0){
      fprintf(stderr, "ERROR reading from socket\n");
    }

    // Add the chunk to the main buffer message
    strcat(buffer, socket_buffer);
  }

  // Remove the terminating characters
  char* terminalLoc = strstr(buffer, "@@");
  *terminalLoc = '\0';

  // Add a newline char at the end
  strcat(buffer, "\n");

  // Print to stdout
  fprintf(stdout,"%s", buffer);

  // Close the socket
  close(socketFD); 
  return 0;
}