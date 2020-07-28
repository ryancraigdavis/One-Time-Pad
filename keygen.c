// daviryan@oregonstate.edu
// Keygen - One-time Pad

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdbool.h>


int main(int argc, char** argv) {

    // Converts the inputted argument (a string) to an int - the size of the key
    int key_length = atoi(argv[1]);

    // Initializes all values including seeding the random generator
    int r_num;
    char alpha;
    char output[key_length+1];
    srand(time(NULL));

    // Based on the key size, generates that amount of random numbers between 0 and 26
    // 1 is added to each random number to get a number between 1-27
    // Uses the ASCII table to determine the letter in the key
    for (int i = 0; i < key_length; ++i) {
        r_num = rand() % 27;
        r_num = r_num + 1;

        // If the number generated was 27, set alpha = to 32, the [space] character
        if (r_num == 27) {
            alpha = 32;

        // Else add 64, which gives you a capitalized ASCII character (1+64 = 'A')
        } else {
            alpha = r_num + 64;
        }

        // Add the generated letter to the output string
        output[i] = alpha;
    }

    // The final character is a new line character (10)
    // Then send the output to stdout as a string
    output[key_length] = 10;
    fprintf(stdout,"%s", output);
    
    return 0;
}