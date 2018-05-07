#include "lib.h"

#ifndef OUTPUT_H
#define OUTPUT_H

class Output {
    // Super class for every output classes.
    // All output class have file descriptor and size(except led).
    // All output class print information from address to embedded board.
    protected:
        int fd;
        int size;
    
    public:
        ~Output(){ // close file descriptor
            close(fd);
        }
        
        void set(char *addr){ // write to file
            write(fd, addr, size);
        }
};

class Fnd : public Output {
    public:
        Fnd(){ // open device driver and initialize
            fd = open("/dev/fpga_fnd", O_RDWR);
            size = 4;
        }
};

class Lcd : public Output {
    public:
        Lcd(){ // open device driver and initialize
            fd = open("/dev/fpga_text_lcd", O_RDWR);
            size = 8;
        }
};

class Dot : public Output {
    public:
        Dot(){ // open device driver and initialize
            fd = open("/dev/fpga_dot", O_WRONLY);
            size = 10;
        }
};

class Led : public Output {
    private:
        unsigned char *addr;
    
    public:
        Led(){ // calculate memory map
            unsigned long *base = 0;
            fd = open("/dev/mem", O_RDWR|O_SYNC);
            base = (unsigned long *)mmap(NULL, 4096, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x8000000);
            addr = (unsigned char *)((void *)base + 0x16);
        }

        ~Led(){ // delete memory map
            munmap(addr, 4096);
        }

        void set(char* addr){ // write to memory map (override)
           *(this->addr) = *addr;
        }
};

#endif
