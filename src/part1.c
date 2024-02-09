   #include <stdio.h>
   #include <unistd.h>
   #include <fcntl.h>

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

int main() {
    // write-1
    write(STDOUT_FILENO, "Hello, this is a write system call.\n", 37);

    // read-2
    char buffer[100];
    ssize_t bytesRead = read(STDIN_FILENO, buffer, sizeof(buffer) - 1);
    if (bytesRead < 0) {
        perror("Error reading from stdin");
        return 1;
    }
    buffer[bytesRead] = '\0'; // Null-terminate the string
    printf("System Call 2: %s\n", buffer);

    // open-3
    int fileDescriptor = open("example.txt", O_CREAT | O_WRONLY, 0666);
    if (fileDescriptor == -1) {
        perror("Error opening file");
        return 1;
    }
    write(STDOUT_FILENO, "File opened successfully.\n", 26);

    // close-4
    int fileDescriptorToClose = open("example.txt", O_RDONLY);
    if (fileDescriptorToClose == -1) {
        perror("Error opening file for closing");
        return 1;
    }
    close(fileDescriptorToClose);
    write(STDOUT_FILENO, "File closed successfully.\n", 26);

    // lseek-5
    // Use lseek to move the file offset to the beginning
    off_t offset = lseek(fileDescriptor, 0, SEEK_SET);

    // Check if lseek was successful
    if (offset == -1) {
        perror("Error using lseek");
        return 1;
    }

    // Print a message indicating that lseek was successful
    write(STDOUT_FILENO, "lseek operation successful. File offset set to the beginning.\n", 65);

    // Close the file descriptor opened with open-3
    close(fileDescriptor);

    return 0;
}
