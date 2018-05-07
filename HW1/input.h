#include "lib.h"

#ifndef INPUT_H
#define INPUT_H

class Event {
    // In event class, get event from embedded board and save information to address.
    private:
        struct input_event event;
        int fd;
        int size;

    public:
        Event(){ // open device driver and initialize
            fd = open("/dev/input/event0", O_RDONLY|O_NONBLOCK);
            size = sizeof(event);
            memset(&event, 0, size);
        }   

        ~Event(){ // close device driver
            close(fd);
        }

        void get(char *addr){ // read key and set address 
            if(read(fd, &event, size) > 0 && event.value == KEY_PRESS) { // only key pressed
                if(event.code == 158) *addr = 'Q';
                else if(event.code == 115) *addr = (*addr + 1) % 5;
                else if(event.code == 114) *addr = (*addr + 4) % 5;    
            }
        }
};

#define SWITCH_SIZE 9

class Switch {
    // In switch class, get switch event from embedded board and save information to address.
    private:
        unsigned char sw[SWITCH_SIZE];
        unsigned char prev_sw[SWITCH_SIZE];
        unsigned char output_sw[SWITCH_SIZE];
        int fd;
        int size;

    public:
        Switch(){ // open device driver and initialize
            fd = open("/dev/fpga_push_switch", O_RDONLY|O_NONBLOCK);
            size = sizeof(sw);
            memset(sw, 0, size);
        }   

        ~Switch(){ // close device driver
            close(fd);
        }

        void get(char *addr){ // read key and set address
            int i;
            int cnt = 0;
            int repeat = 2500;
            
            memset(output_sw, 0, size);
            while(repeat--){ // repeat reading to get dual input
                memcpy(prev_sw, sw, size);
                read(fd, sw, size);

                for(i = 0; i < SWITCH_SIZE; i++){
                    if(sw[i] - prev_sw[i] == KEY_PRESS) { // change from 0 to 1
                        output_sw[i] |= KEY_PRESS;
                    }
                }
            }
            memcpy(addr, output_sw, size);
        }
};

#endif
