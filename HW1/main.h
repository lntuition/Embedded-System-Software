#include "lib.h"

#ifndef MAIN_H
#define MAIN_H

#define ADDITIONAL

class Shared {
    // In Shared class, calculated proper shared memory address to each variable.

    private:
        int id;

    public:
        char *ev, *sw, *fnd, *lcd, *dot, *led;

        Shared(){ // make shared memory and map address to profit variable
            id = shmget(0, 1024, IPC_CREAT|0644);
            ev = (char *)shmat(id, (char *)NULL, 0);

            sw = ev + 1;
            fnd = ev + 10;
            lcd = ev + 14;
            dot = ev + 22;
            led = ev + 32;
        }

        ~Shared(){ // unmap address and delete shared mermory
            shmdt((char *)ev);
            shmctl(id, IPC_RMID, (struct shmid_ds *)NULL);
        }
        
        void reset(char type){ // reset profit shared memory
            if(type == 's') memset(sw, 0, 9); // not implemented except switch
        }
};

#define CLOCK_SIZE 4
class Clock {
    // In Clock class, initialize/calculate/flow time. 
    private:
        unsigned char clock[CLOCK_SIZE];
        struct tm *info;
        int size;

    public:
        time_t now;
        bool change_flag; // True : able to change

        Clock(){
            size = sizeof(clock);
            init();
        }   

        void init(){
            change_flag = false;
            reset();
        }

        void reset(){ // reset to now time
            now = time(NULL);
            info = localtime(&now);
            convert();
        }

        void convert(){ // convert time to char array
            clock[0] = info->tm_hour / 10;
            clock[1] = info->tm_hour % 10;
            clock[2] = info->tm_min / 10;
            clock[3] = info->tm_min % 10;
        }

        void set(char *addr){
            memcpy(addr, clock, size);
        }

        void add(char type){ // add time
            if(type == 'h') info->tm_hour += 1;
            else if(type == 'm') info->tm_min += 1;

            mktime(info);
            convert();
        }

        void flow(){ // flow time
            if(difftime(time(NULL), now) >= 60.0){
                now = time(NULL);
                add('m');
            }
        }

};

#define COUNTER_SIZE 4
class Counter {
    // In Counter class, calculate number and change digit.
    // In Addtion, make rand number and compare number.
    private:
        unsigned char counter[COUNTER_SIZE];
        int number;
        int radix[4];
        int idx;
        int size;

    public:
        Counter(){
            size = sizeof(counter);
            radix[0] = 10;
            radix[1] = 8;
            radix[2] = 4;
            radix[3] = 2;
            init();
        }   

        void init(){
            number = 0;
            idx = 0;
            convert_number(true);
        }

        int convert_radix(){
            idx = (idx + 1) % 4;
            convert_number(true);
            return radix[idx];
        }

        void convert_number(bool flag){ // convert number with radix[idx]
            int i;
            int tmp_num = number;

            for(i = 3; i >= 0; i--){
                counter[i] = tmp_num % radix[idx];
                tmp_num /= radix[idx];
            }
            if(flag == true && idx == 0) // true : counter[0] is always 0
                counter[0] = 0;
        }

        void set(char *addr){   
            memcpy(addr, counter, size);
        }

        void add(int digit, bool flag){ // add digit number to counter, for example digit 0 is units
            int adder = 1;
            while(digit--){
                adder *= radix[idx];
            }
            number += adder;
            convert_number(flag);
        }

#ifdef ADDITIONAL
        unsigned char rand_counter[COUNTER_SIZE];        
        bool rand_hint_flag;

        void rand_init(){ // make other 3 numbers with rand counter
            srand(time(NULL));
            rand_counter[0] = 0;
            rand_counter[1] = rand() % 10;
            do{
                rand_counter[2] = rand() % 10;
            }while(rand_counter[1] == rand_counter[2]);
            do{
                rand_counter[3] = rand() % 10;
            }while(rand_counter[1] == rand_counter[3] || rand_counter[2] == rand_counter[3]);

            rand_hint_flag = true;
            printf("[DEBUG] ANSWER IS %d%d%d\n", rand_counter[1], rand_counter[2], rand_counter[3]);
        }

        int rand_compare(){
            int i, j;
            int strike = 0, ball = 0;
            if(counter[1] == counter[2] || counter[2] == counter[3] || counter[1] == counter[3]){
                return -1;
            }
            
            for(i = 1; i < COUNTER_SIZE; i++){
                for(j = 1; j < COUNTER_SIZE; j++){
                    if(counter[i] == rand_counter[j]){
                        if(i == j) strike++;
                        else ball++;
                    }
                }
            }
            return strike * 10 + ball;
        }

        int rand_hint(){
            if(counter[1] > rand_counter[1]) return 3; // down
            else if(counter[1] < rand_counter[1]) return 2; // up

            if(counter[2] > rand_counter[2]) return 3;
            else if(counter[2] < rand_counter[2]) return 2;

            if(counter[3] > rand_counter[3]) return 3;
            else if(counter[3] < rand_counter[3]) return 2;

            return 4; // equal
        }

        void rand_answer(){
            memcpy(counter, rand_counter, size);
        }
#endif
};

unsigned char alphabet[9][4] = {".QZ", "ABC", "DEF", "GHI", "JKL", "MNO", "PRS", "TUV", "WXY"};
#define TEXT_SIZE 8
class Text {
    // In Text class, input/reset text.
    // In Addtion, make information text for random number from state.
    
    private:
        int idx; // before location for wrting character
        int size;
        unsigned char text[TEXT_SIZE];
    
    public:
        int mode;

        Text(){
            size = sizeof(text);
            init();
        }

        void init(){
            mode = 1; // text
            reset();
        }

        void reset(){
            memset(text, 0, size);
            idx = -1;
        }

        void set(char *addr){
            memcpy(addr, text, size);
        }

        void add(int input){
            char ch = '\0';
            int i;
            
            if(input == 10) ch = ' ';
            else if(mode){
                if(idx == -1) ch = alphabet[input][0];
                else {
                    if(alphabet[input][0] == text[idx]) text[idx] = alphabet[input][1];
                    else if(alphabet[input][1] == text[idx]) text[idx] = alphabet[input][2];
                    else if(alphabet[input][2] == text[idx]) text[idx] = alphabet[input][0];
                    else ch = alphabet[input][0];
                }
            }
            else ch = '1' + input;

            if(ch != '\0'){ // Add new character
                if(idx == TEXT_SIZE - 1) { // max length
                    memcpy(text, text + 1, TEXT_SIZE - 1);
                    idx--;
                }   
                text[++idx] = ch;
            }
        }

#ifdef ADDITIONAL
        int rand_result(int state){
            memset(text, 0, size);
            
            if(state == -1){
                memcpy(text, "RESUBMIT", 8);
                return 0; // 0 chance use
            }
            else if(state == 0){
                memcpy(text, "OUT", 3);
            }
            else if(state >= 30){
                memcpy(text, "HOMERUN", 8);
                return -1; // end -> reset
            }
            else {
                text[0] = 'S';
                text[1] = '0' + state / 10;
                text[2] = 'B';
                text[3] = '0' + state % 10;
            }
            return 1; // 1 chance use
        }
#endif
};

#define DRAW_SIZE 10
unsigned char image[2][DRAW_SIZE] = {
    {0x0c, 0x1c, 0x1c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x3f, 0x3f},
    {0x1c, 0x36, 0x63, 0x63, 0x63, 0x7f, 0x7f, 0x63, 0x63, 0x63}
};
#ifdef ADDITIONAL
unsigned char rand_image[5][DRAW_SIZE] = {
    {0x00, 0x3E, 0x3E, 0x3E, 0x3E, 0x3E, 0x38, 0x38, 0x38, 0x00}, // fail
    {0x00, 0x38, 0x38, 0x38, 0x3E, 0x3E, 0x3E, 0x3E, 0x3E, 0x00}, // success
    {0x00, 0x08, 0x1C, 0x3E, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x00}, // up
    {0x00, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x3E, 0x1C, 0x08, 0x00}, // down
    {0x00, 0x00, 0x3E, 0x3E, 0x00, 0x00, 0x3E, 0x3E, 0x00, 0x00}  // equal
};
#endif

class Draw {
    // In Draw class, draw board and move location(x, y).
    // In Addtion, print picture information from rand number.

    private:
        unsigned char draw[DRAW_SIZE];
        int x, y;
        int size;

    public:
        bool blink_flag;
        time_t now;

        Draw(){
            size = sizeof(draw);
            init();
        }

        void init(){
            now = time(NULL);
            reset();
        }

        void reset(){
            blink_flag = true;
            x = 6;
            y = 0;
            clear();
        }

        void clear(){
            memset(draw, 0, size);
        }

        void set(char *addr){
            memcpy(addr, draw, size);
        }

        void move(char type){
            if(type == 'u' && y > 0) y--;
            else if(type == 'd' && y < 9) y++;
            else if(type == 'r' && x > 0) x--;
            else if(type == 'l' && x < 6) x++;
        }

        void reverse(){ // reverse now location(y, x)
            unsigned char num = 1;
            draw[y] ^= (num << x);
        }

        void reverse_all(){ // reverse all draw
            int i;
            for(i = 0; i < DRAW_SIZE; i++){
                draw[i] ^= 0x7f;
            }
        }

        void blink(char *addr){ // blink draw
            unsigned char num = 1;
            
            if(difftime(time(NULL), now) >= 1.0){
                now = time(NULL);
                addr[y] ^= (num << x);
            }
        }

        void picture(int mode){ // print picture
            memcpy(draw, image[mode], size);
        }

#ifdef ADDITIONAL
        bool rand_end(int mode){
            if(mode > -1){ // game end
                rand_picture(mode);
                return true;
            }
            return false;
        }

        void rand_picture(int mode){
            memcpy(draw, rand_image[mode], size);
        }
#endif
};

class State {
    // In State class, change/blink state.
    // In addition, calculate change with state.
    private:
        unsigned char state;
        unsigned char blink_state;

    public:
        bool blink_flag; // true : able to blink
        time_t now;

        State(){
            blink_flag = false;
        }

        void init(){
            state = 0;
            blink_state = 0x20;
        }

        void set(char *addr){
            *addr = state;
        }

        unsigned char convert_radix_to_state(int radix){ // convert radix to state
            if(radix == 10) return 0x40;
            else if(radix == 8) return 0x20;
            else if(radix == 4) return 0x10;
            else if(radix == 2) return 0x80;
        }

        void change(unsigned char state){ // change state
            this->state = state;
        }

        void blink(){
            if(difftime(time(NULL), now) >= 1.0){
                now = time(NULL);
                change(blink_state);
                blink_state ^= 0x30;
            }
        }

#ifdef ADDITIONAL
        int chance;
        
        void rand_init(){
            state = 0xFF;
            chance = 8;
        }

        int rand_chance(int state){
            unsigned char num = 1;

            if(state == 1){ // 1 chance use
                chance--;
                this->state ^= (num << chance);
                if(this->state == 0){
                    return 0; // end with fail
                }
            }
            else if(state == -1){
                return 1; // end with success
            }
            return -1; // not yet end
        }
#endif
};

#endif
