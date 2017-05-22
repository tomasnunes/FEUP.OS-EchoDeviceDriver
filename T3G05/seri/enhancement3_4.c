#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#define IOCTL_WL5     0x00 //Set Word Length to 5
#define IOCTL_WL6     0x01 //Set Word Length to 6
#define IOCTL_WL7     0x02 //Set Word Length to 7
#define IOCTL_WL8     0x03 //Set Word Length to 8
#define IOCTL_SB1     0x00 //Set Number of Stop Bits to 1
#define IOCTL_SB2     0x04 //Set Number of Stop Bits to 2
#define IOCTL_PTN     0x00 //Set Parity to None
#define IOCTL_PTO     0x08 //Set Parity to Odd
#define IOCTL_PTE     0x18 //Set Parity to Even
#define IOCTL_PT1     0x28 //Set Parity to 1
#define IOCTL_PT0     0x38 //Set Parity to 0
#define IOCTL_BKE     0x40 //Set Break Enable
#define IOCTL_BRT     0x80 //Set Bit Rate (Should pass the value in the arg parameter)

#define MAX_BUFF_SIZE 100

int main(int argc, char *argv[]) {
    if(argc < 2) {
        printf("Usage: %s <device_to_test>\n", argv[0]);
        return -1;
    }

    int fd = open(argv[1], O_RDWR | O_NONBLOCK);
    if(fd == -1) {
        printf("Error: cannot open file %s\n", argv[1]);
        return -1;
    }
    char *buff = (char *)malloc(MAX_BUFF_SIZE);
    ssize_t cwrite = 0, cread = 0;

    int bitrate = 2400;
    unsigned char cmd = 0 | IOCTL_PT0 | IOCTL_WL8 | IOCTL_SB2 | IOCTL_BKE | IOCTL_BRT;
    ioctl(fd, cmd, 2400);

    free(buff);
    close(fd);

    return 0;
}
