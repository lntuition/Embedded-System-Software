#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/unistd.h>
#include <sys/syscall.h>
#include <asm/ioctl.h>

int main(int argc, char **argv)
{
    int fd;
    int data[3] = {0, 0, 0};
    unsigned int buffer;

    fd = open("/dev/dev_driver", O_WRONLY);
    if(fd < 0){
        printf("Open Failured!\n");
        return -1;
    }

    data[0] = atoi(argv[1]);
    data[1] = atoi(argv[2]);
    data[2] = atoi(argv[3]);
    buffer = syscall(378, data); // System call

    //write(fd, &buffer, sizeof(buffer));
    ioctl(fd, _IOW(242, 0, unsigned int), &buffer); // Write to device
    close(fd);

    return 0;
}
