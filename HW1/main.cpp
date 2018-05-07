#include "lib.h"
#include "input.h"
#include "output.h"
#include "main.h"

void input_process(Shared);
void output_process(Shared);
void main_process(Shared);

#define CLOCK 0
#define COUNTER 1
#define TEXT 2
#define DRAW 3
#define BASEBALL 4

int main(int argc, char *argv[]){
    Shared shm;
    int status;

    // fork input and output process
    pid_t pid = fork();
    if(pid == 0){ // input process
        input_process(shm);
    }
    else {
        pid = fork();
        if(pid == 0){ // output process
            output_process(shm);
        }
        else{
            main_process(shm);
            wait(&status);
        }
        wait(&status);
    }
    return 0;
}

void input_process(Shared shm){
    // 1. receive input from embedded machine
    // 2. send inputs to main process with IPC(shared memory)
    
    Event ev;
    Switch sw;

    while((shm.ev)[0] != 'Q'){
        ev.get(shm.ev);
        sw.get(shm.sw);
    }
}

void output_process(Shared shm){
    // 1. receive outputs from main process with IPC(shared memory)
    // 2. print output to embedded machine
    
    Fnd fnd;
    Lcd lcd;
    Dot dot;
    Led led;

    while((shm.ev)[0] != 'Q'){
        fnd.set(shm.fnd);
        lcd.set(shm.lcd);
        dot.set(shm.dot);
        led.set(shm.led);    
    }
}

void main_process(Shared shm){
    // 1. receive inputs from input process with IPC(shared memory)
    // 2. do profit operation
    // 3. send result to output process with IPC(shared memory)
    
    int i;
    char prev_ev = 255; // initialize
    (shm.ev)[0] = 0;

    Clock clock;
    Counter counter;
    Text text;
    Draw draw;
    State state;

    while((shm.ev)[0] != 'Q'){
        if(prev_ev != (shm.ev)[0]){ // mode changed -> make iniital state
            if((shm.ev)[0] >= CLOCK && (shm.ev)[0] <= BASEBALL){
                shm.reset('s');
                counter.init();
                text.init();
                draw.init();
                state.init();

                if((shm.ev)[0] == CLOCK){
                    state.change(0x80);
                }
                else if((shm.ev)[0] == COUNTER){
                    state.change(0x40);
                }
                else if((shm.ev)[0] == TEXT){
                    draw.picture(text.mode);
                }
                else if((shm.ev)[0] == BASEBALL){
                    counter.rand_init();
                    state.rand_init();
                }
                
                counter.set(shm.fnd);
                text.set(shm.lcd);
                draw.set(shm.dot);
                state.set(shm.led);
            }
        }
        prev_ev = (shm.ev)[0];

        switch((shm.ev)[0]){
            case CLOCK:
                if(clock.change_flag){
                    state.blink();
                    
                    if((shm.sw)[0] == KEY_PRESS){ // save
                        clock.change_flag = false;
                        clock.now = time(NULL);
                        state.change(0x80);
                        (shm.sw)[0] = KEY_RELEASE;
                    }
                    else if((shm.sw)[1] == KEY_PRESS){ // reset
                        clock.reset();
                        (shm.sw)[1] = KEY_RELEASE;
                    }
                    else if((shm.sw)[2] == KEY_PRESS){ // add hour
                        clock.add('h');
                        (shm.sw)[2] = KEY_RELEASE;
                    }
                    else if((shm.sw)[3] == KEY_PRESS){ // add minute
                        clock.add('m');
                        (shm.sw)[3] = KEY_RELEASE;
                    }
                }
                else {
                    clock.flow();
                    
                    if((shm.sw)[0] == KEY_PRESS){ // able to change
                        clock.change_flag = true;
                        state.change(0x30);
                        state.now = time(NULL);
                        shm.reset('s');
                    }
                }
                clock.set(shm.fnd);
                state.set(shm.led);
                break;
            case COUNTER:
                if((shm.sw)[0] == KEY_PRESS){  // convert radix
                    state.change(state.convert_radix_to_state(counter.convert_radix()));
                    (shm.sw)[0] = KEY_RELEASE;
                }
                else if((shm.sw)[1] == KEY_PRESS){ // add units
                    counter.add(0, true);
                    (shm.sw)[1] = KEY_RELEASE;
                }
                else if((shm.sw)[2] == KEY_PRESS){ // add tens
                    counter.add(1, true);
                    (shm.sw)[2] = KEY_RELEASE;
                }
                else if((shm.sw)[3] == KEY_PRESS){ // add hundreds
                    counter.add(2, true);
                    (shm.sw)[3] = KEY_RELEASE;
                }
                counter.set(shm.fnd);
                state.set(shm.led);
                break;
            case TEXT:
                if((shm.sw)[1] == KEY_PRESS && (shm.sw)[2] == KEY_PRESS){ // reset
                    text.reset();
                    counter.add(0, false);
                    (shm.sw)[1] = KEY_RELEASE;
                    (shm.sw)[2] = KEY_RELEASE;
                }
                else if((shm.sw)[4] == KEY_PRESS && (shm.sw)[5] == KEY_PRESS){ // change alphabet/number 
                    text.mode = 1 - text.mode;
                    draw.picture(text.mode);
                    counter.add(0, false);
                    (shm.sw)[4] = KEY_RELEASE;
                    (shm.sw)[5] = KEY_RELEASE;
                }
                else if((shm.sw)[7] == KEY_PRESS && (shm.sw)[8] == KEY_PRESS){ // add space
                    text.add(10);
                    counter.add(0, false);
                    (shm.sw)[7] = KEY_RELEASE;
                    (shm.sw)[8] = KEY_RELEASE;
                }
                else { // add alphabet/number
                    for(i = 0; i < 9; i++){
                        if((shm.sw)[i] == KEY_PRESS){
                            text.add(i);
                            counter.add(0, false);
                            (shm.sw)[i] = KEY_RELEASE;
                            break;
                        }
                    }    
                }
                text.set(shm.lcd);
                counter.set(shm.fnd);
                draw.set(shm.dot);
                break;
            case DRAW:
                if((shm.sw)[0] == KEY_PRESS){ // reset
                    draw.reset();
                    draw.set(shm.dot);
                    counter.add(0, false);
                    shm.sw[0] = KEY_RELEASE;
                }
                else if((shm.sw)[2] == KEY_PRESS){ // blink on/off
                    draw.blink_flag = !draw.blink_flag;
                    draw.set(shm.dot);
                    draw.now = time(NULL);
                    counter.add(0, false);
                    shm.sw[2] = KEY_RELEASE;
                }
                else if((shm.sw)[6] == KEY_PRESS){ // clear
                    draw.clear();
                    draw.set(shm.dot);
                    counter.add(0, false);
                    shm.sw[6] = KEY_RELEASE;
                }
                else if((shm.sw)[8] == KEY_PRESS){ // reverse
                    draw.reverse_all();
                    draw.set(shm.dot);
                    counter.add(0, false);
                    shm.sw[8] = KEY_RELEASE;
                }
                else if((shm.sw)[4] == KEY_PRESS){ // check/uncheck
                    draw.reverse();
                    draw.set(shm.dot);
                    counter.add(0, false);
                    shm.sw[4] = KEY_RELEASE;
                }
                else if((shm.sw)[1] == KEY_PRESS){ // move up
                    draw.move('u');
                    draw.set(shm.dot);
                    counter.add(0, false);
                    shm.sw[1] = KEY_RELEASE;
                }
                else if((shm.sw)[3] == KEY_PRESS){ // move left
                    draw.move('l');
                    draw.set(shm.dot);
                    counter.add(0, false);
                    shm.sw[3] = KEY_RELEASE;
                }
                else if((shm.sw)[5] == KEY_PRESS){ // move right
                    draw.move('r');
                    draw.set(shm.dot);
                    counter.add(0, false);
                    shm.sw[5] = KEY_RELEASE;
                }
                else if((shm.sw)[7] == KEY_PRESS){ // move down
                    draw.move('d');
                    draw.set(shm.dot);
                    counter.add(0, false);
                    (shm.sw)[7] = KEY_RELEASE;
                }

                counter.set(shm.fnd);
                if(draw.blink_flag){
                    draw.blink(shm.dot);
                }
                break;
            case BASEBALL:
                if((shm.sw)[0] == KEY_PRESS){ // reset
                    counter.init();
                    (shm.sw)[0] = KEY_RELEASE;
                }
                else if((shm.sw)[1] == KEY_PRESS){ // add units
                    counter.add(0, true);
                    (shm.sw)[1] = KEY_RELEASE;
                }
                else if((shm.sw)[2] == KEY_PRESS){ // add tens
                    counter.add(1, true);
                    (shm.sw)[2] = KEY_RELEASE;
                }
                else if((shm.sw)[3] == KEY_PRESS){ // add hundreds
                    counter.add(2, true);
                    (shm.sw)[3] = KEY_RELEASE;
                }
                else if((shm.sw)[4] == KEY_PRESS){ // submit
                    if(draw.rand_end(state.rand_chance(text.rand_result(counter.rand_compare())))){
                        counter.init();
                        counter.rand_answer();
                        counter.rand_init();
                        state.rand_init();
                    }
                    else {
                        counter.init();
                        draw.init();
                    }
                    (shm.sw)[4] = KEY_RELEASE;
                }
                else if((shm.sw)[5] == KEY_PRESS){ // hint
                    if(counter.rand_hint_flag){
                        counter.rand_hint_flag = false;
                        draw.rand_picture(counter.rand_hint());
                    }
                    counter.init();
                    (shm.sw)[5] = KEY_RELEASE;
                }

                counter.set(shm.fnd);
                text.set(shm.lcd);
                state.set(shm.led);
                draw.set(shm.dot);
                break;
            default:
                break;
        }
    }
}
