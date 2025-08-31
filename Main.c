#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/wait.h>

#define NUM_MSGS 1000000   // 1 million messages

int main() {
    int p2c[2], c2p[2];   // pipes
    pid_t pid;

    // Create pipes
    if (pipe(p2c) == -1 || pipe(c2p) == -1) {
        perror("pipe");
        exit(1);
    }

    pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(1);
    }

    if (pid == 0) {  
        // === CHILD PROCESS ===
        close(p2c[1]);  // close write end of parent->child
        close(c2p[0]);  // close read end of child->parent

        int x;
        for (int i = 0; i < NUM_MSGS; i++) {
            // Read integer from parent
            if (read(p2c[0], &x, sizeof(x)) != sizeof(x)) {
                perror("child read");
                exit(1);
            }
            // Send integer back to parent
            if (write(c2p[1], &x, sizeof(x)) != sizeof(x)) {
                perror("child write");
                exit(1);
            }
        }

        close(p2c[0]);
        close(c2p[1]);
        exit(0);
    } 
    else {  
        // === PARENT PROCESS ===
        close(p2c[0]);  // close read end of parent->child
        close(c2p[1]);  // close write end of child->parent

        struct timeval start, end;
        gettimeofday(&start, NULL);  // start time

        int x = 42, y;
        for (int i = 0; i < NUM_MSGS; i++) {
            // Send integer to child
            if (write(p2c[1], &x, sizeof(x)) != sizeof(x)) {
                perror("parent write");
                exit(1);
            }
            // Read back from child
            if (read(c2p[0], &y, sizeof(y)) != sizeof(y)) {
                perror("parent read");
                exit(1);
            }
        }

        gettimeofday(&end, NULL);  // end time

        long seconds = end.tv_sec - start.tv_sec;
        long usec = end.tv_usec - start.tv_usec;
        double total_time = seconds + usec / 1000000.0;

        printf("Total messages: %d\n", NUM_MSGS);
        printf("Total time: %.6f seconds\n", total_time);
        printf("Requests per second: %.2f\n", NUM_MSGS / total_time);
        printf("Average roundtrip time per message: %.9f sec\n",
               total_time / NUM_MSGS);

        close(p2c[1]);
        close(c2p[0]);
        wait(NULL);  // wait for child
    }

    return 0;
}
