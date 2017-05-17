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

    ssize_t cwrite = 0;
    //WRITE TEST!
    //strcpy(buff, "Hello from the othersiiiiiiide!\n");
    //ssize_t cwrite = write(fd, buff, strlen(buff));
    //printf("    -T- I send %ld chars to the otherside:\n%s\n", cwrite, buff);
    //WRITE TEST!

    /*
    char c = '\0';
    int ii = 0;
    while(scanf("%c", c) != EOF) {
      buff[ii++] = c;

      if(ii == MAX_BUFF_SIZE-1) {
        cwrite = write(fd, buff, strlen(buff));
        ii = 0;
      }
    }

    if(ii != 0)
    cwrite = write(fd, buff, strlen(buff));
    */

    ssize_t cread = read(fd, buff, MAX_BUFF_SIZE);

    if(cread > 0)
      printf("    -T- I got %ld chars from the otherside:\n-> %s\n\n", cread, buff);
    else
      printf("    -T- There was an error reading!\n");

    printf("    -T- echo_write returned: %ld\n    -T- echo_read returned: %ld\n", cwrite, cread);

    free(buff);
    close(fd);

    return 0;
}
