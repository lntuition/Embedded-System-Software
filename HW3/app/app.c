#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

int main(int argc, char **argv)
{
    int fd;
    char buffer = NULL;

    fd = open("/dev/stopwatch", O_WRONLY);
    if(fd < 0){
        printf("Open Failured!\n");
        return -1;
    }
    
    write(fd, &buffer, sizeof(buffer));
    close(fd);

    return 0;
}
