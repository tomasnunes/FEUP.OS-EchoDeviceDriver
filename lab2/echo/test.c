#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#define MAX_BUFF_SIZE 100

int main(int argc, char *argv[]) {
    if(argc < 2) {
        printf("Usage: %s <device_to_test>\n", argv[0]);
        return -1;
    }

    int fd = open(argv[1], O_WRONLY);
    if(fd == -1) {
        printf("Error: cannot open file %s\n", argv[1]);
        return -1;
    }

    char *buff = (char *)malloc(MAX_BUFF_SIZE);
    strcpy(buff, "      -U- I'm in kernel space!\n      -U- Hello 'K'!\n");

    int cwrite = write(fd, buff, MAX_BUFF_SIZE);
    int cread = read(fd, buff, MAX_BUFF_SIZE);

    printf("    -T- echo_write returned: %d\n    -T- echo_read returned: %d\n", cwrite, cread);

    free(buff);
    close(fd);

    return 0;
}
