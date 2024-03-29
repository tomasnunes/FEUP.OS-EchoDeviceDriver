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

    int fd = open(argv[1], O_RDWR);
    if(fd == -1) {
        printf("Error: cannot open file %s\n", argv[1]);
        return -1;
    }

    char *buff = (char *)malloc(MAX_BUFF_SIZE);
    strcpy(buff, ">I'm in kernel space!\n           >Hello 'K'!!\n");

    ssize_t cwrite = write(fd, buff, strlen(buff));
    //cwrite = write(fd, buff, strlen(buff));
    ssize_t cread = read(fd, buff, MAX_BUFF_SIZE);

    printf("    -T- echo_write returned: %ld\n    -T- echo_read returned: %ld\n", cwrite, cread);

    free(buff);
    close(fd);

    return 0;
}
