#include <linux/kernel.h>
#include <linux/uaccess.h>

asmlinkage unsigned int sys_newcall3(int *data){
    int tmp[3], i;
    char location = 0, number, counter, interval;

    copy_from_user(tmp, data, sizeof(tmp));
    
    for(i = 10; i <= 10000; i *= 10){
        if(tmp[2] % i != 0){           
            number = tmp[2] * 10 / i;
            break;
        }
        location++;
    }
    interval = tmp[0];
    counter = tmp[1];

    return location << 24 | number << 16 | counter << 8 | interval;
}
