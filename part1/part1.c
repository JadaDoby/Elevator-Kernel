#include <unistd.h>

int main() {
    // Example system calls
    write(1, "Hello, System Call!\n", 20); 
    getpid(); 
    chdir("/tmp"); 
    access("/etc/passwd", F_OK); 
    sleep(2); 

    return 0;
}
