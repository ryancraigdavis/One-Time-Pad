// daviryan@oregonstate.edu
// Decryption Client - One-time Pad

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
  char * file_buffer_dec = NULL;
  char * key_buffer_dec = NULL;
  char * buffer_dec = NULL;
  long file_length_dec;
  long key_length_dec;
  long buffer_length_dec;

  // File variables for the text file and key
  FILE *file_read_dec;
  FILE *key_read_dec;

  // Open the input file
  file_read_dec = fopen(argv[1], "r+");

  // Used modfied code from this answer on reading a file into a string
  // https://stackoverflow.com/questions/174531/how-to-read-the-content-of-a-file-to-a-string-in-c
  if (file_read_dec) {
    fseek (file_read_dec, 0, SEEK_END);
    file_length_dec = ftell(file_read_dec);
    fseek (file_read_dec, 0, SEEK_SET);
    file_buffer_dec = malloc(file_length_dec+1);

    // As long as the buffer exists, read into it the file info
    // Then close the file
    if (file_buffer_dec) {
      fread (file_buffer_dec, 1, file_length_dec, file_read_dec);
    }
    fclose (file_read_dec);
  }

  // Remove the new line char at the end of the string and dec the file length by 1
  if (strstr(file_buffer_dec, "\n") != NULL) {
    char* new_line_dec = strstr(file_buffer_dec, "\n");
    *new_line_dec = '\0';
    file_length_dec = file_length_dec - 1;
  }

  // For loop goes through all of the input characters and verifies they aren't bad
  for (int i = 0; i < file_length_dec; ++i) {
    int check_int_dec = file_buffer_dec[i] - 65;

    // If the check int is below 0 and not -33 (space) then it is bad
    if (check_int_dec < 0 && check_int_dec != -33) {
      fprintf(stderr, "dec_client error: input contains bad characters\n");
      exit(1);

    // Or if the int is above 25
    } else if (check_int_dec > 25) {
      fprintf(stderr, "dec_client error: input contains bad characters\n");
      exit(1);
    }
  }
  printf("%d\n",3);
  // Same code as above, open the key file and read it in
  key_read_dec = fopen(argv[2], "r+");
  if (key_read_dec) {
    fseek (key_read_dec, 0, SEEK_END);
    key_length_dec = ftell(key_read_dec);
    fseek (key_read_dec, 0, SEEK_SET);
    key_buffer_dec = malloc(key_length_dec+1);

    // As long as the buffer exists, read into it the file info
    // Then close the file
    if (key_buffer_dec) {
      fread (key_buffer_dec, 1, key_length_dec, key_read_dec);
    }
    fclose (key_read_dec);
  }

    // Remove the new line char at the end of the string and dec the key length by 1
  if (strstr(key_buffer_dec, "\n") != NULL) {
    char* new_line_k_dec = strstr(key_buffer_dec, "\n");
    *new_line_k_dec = '\0';
    key_length_dec = key_length_dec - 1;
  }

  // Check to see if the key is at least the length of the file
  if (key_length_dec < file_length_dec){
    fprintf(stderr, "Error: key ‘%s’ is too short\n",argv[2]);
    exit(1);
  }
  printf("%d\n",4);
  // Set buffer length and the buffer to a size of 2x the input file
  // Buffer will start with a string DEC to differentiate it from the Enc client
  // Buffer will end with @@ that will indicate the end of the string
  buffer_length_dec = (file_length_dec * 2) + 5;
  buffer_dec = malloc((file_length_dec * 2) + 6);

  // Copy in DEC first
  strcpy(buffer_dec, "DEC");

  // Concat the input file into the buffer char var
  // Then concat it with key_buffer to the length of the input file
  strcat(buffer_dec, file_buffer_dec);
  strncat(buffer_dec, key_buffer_dec, file_length_dec);

  // Finally add on the terminating chars
  strcat(buffer_dec, "@@");
  printf("%d\n",5);
  // Create ints for reading/writing into the socket
  // Create the socket address struct
  int socketFD_dec, portNumber_dec, charsWritten_dec, charsRead_dec;
  struct sockaddr_in serverAddress_dec;

  // Check usage & args - that there are enough arguments
  if (argc < 2) { 
    fprintf(stderr, "USAGE: %s hostname port\n", argv[0]); 
    exit(1); 
  } 
  printf("%d\n",6);
  // Create a socket
  socketFD_dec = socket(AF_INET, SOCK_STREAM, 0); 
  if (socketFD_dec < 0){
    fprintf(stderr, "CLIENT: ERROR opening socket");
    exit(1);
  }

   // Set up the server address struct
  setupAddressStruct(&serverAddress_dec, atoi(argv[3]), "localhost");

  // Connect to server
  if (connect(socketFD_dec, (struct sockaddr*)&serverAddress_dec, sizeof(serverAddress_dec)) < 0){
    fprintf(stderr, "CLIENT: ERROR connecting");
    exit(1);
  }
  printf("%d\n",7);
  // Write to the server checking to make sure all the chars were written
  charsWritten_dec = send(socketFD_dec, buffer_dec, strlen(buffer_dec), 0);
  if (charsWritten_dec < 0){
    fprintf(stderr, "CLIENT: ERROR writing to socket");
  }
  if (charsWritten_dec < strlen(buffer_dec)){
    fprintf(stderr, "CLIENT: WARNING: Not all data written to socket!\n");
  }
  printf("%d\n",8);
  // Set buffer to null terminated chars, creates a socket buffer to read in chunks
  memset(buffer_dec, '\0', buffer_length_dec);
  char socket_buffer_dec[1000];

  // Read the server's response from the socket using a loop until
  // The terminating chars are found
  while (strstr(buffer_dec, "@@") == NULL){

    // First check to see if the connection error message was sent
    // This is an indication that there was a wrong client/server connection
    // Print to stderr, and then exit(2)
    if (strstr(buffer_dec, "~~") != NULL) {
      fprintf(stderr, "Error: could not contact dec_server on port %s\n", argv[3]);
      exit(2);
    }

    // First clear the socket buffer
    memset(socket_buffer_dec, '\0', 1000);

    // Read in the chunk from the socket
    charsRead_dec = recv(socketFD_dec, socket_buffer_dec, sizeof(socket_buffer_dec) - 1, 0); 
    if (charsRead_dec < 0){
      fprintf(stderr, "ERROR reading from socket\n");
    }

    // Add the chunk to the main buffer message
    strcat(buffer_dec, socket_buffer_dec);
  }

  // Remove the terminating characters
  char* terminalLoc_dec = strstr(buffer_dec, "@@");
  *terminalLoc_dec = '\0';

  // Add a newline char at the end
  strcat(buffer_dec, "\n");

  // Print to stdout
  fprintf(stdout,"%s", buffer_dec);

  // Close the socket
  close(socketFD_dec); 
  return 0;
}