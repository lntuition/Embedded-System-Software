#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <asm/irq.h>
#include <mach/gpio.h>
#include <linux/platform_device.h>
#include <asm/gpio.h>
#include <linux/wait.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <linux/ioport.h>
#include <linux/version.h>
#include <linux/cdev.h>

static dev_t stopwatch_dev;
static struct cdev stopwatch_cdev;

static int stopwatch_open(struct inode *, struct file *);
static int stopwatch_release(struct inode *, struct file *);
static int stopwatch_write(struct file *, const char *, size_t, loff_t *);

irqreturn_t home_handler(int, void *, struct pt_regs *);
irqreturn_t back_handler(int, void *, struct pt_regs *);
irqreturn_t volup_handler(int, void *, struct pt_regs *);
irqreturn_t voldown_handler(int, void *, struct pt_regs *);

wait_queue_head_t wq_write;
DECLARE_WAIT_QUEUE_HEAD(wq_write);

struct timer_list do_timer;
struct timer_list end_timer;
static void timer_function(unsigned long);
bool is_doing = false;
bool is_ending = false;

static unsigned char *fnd_addr;
unsigned int fnd_time = 0;
static unsigned short int encode(unsigned int);

static struct file_operations stopwatch_fops =
{
	.open = stopwatch_open,
	.write = stopwatch_write,
	.release = stopwatch_release,
};

irqreturn_t home_handler(int irq, void* dev_id, struct pt_regs* reg) {
    timer_function(0);
    return IRQ_HANDLED;
}

irqreturn_t back_handler(int irq, void* dev_id, struct pt_regs* reg) {
    timer_function(1);
    return IRQ_HANDLED;
}

irqreturn_t volup_handler(int irq, void* dev_id,struct pt_regs* reg) {
    timer_function(2);
    return IRQ_HANDLED;
}

irqreturn_t voldown_handler(int irq, void* dev_id, struct pt_regs* reg) {
    printk(KERN_ALERT "ready to end : %x\n", gpio_get_value(IMX_GPIO_NR(5, 14)));
    timer_function(3);
    
    return IRQ_HANDLED;
}

static void timer_function(unsigned long mode){
    switch(mode){
        case 4: // end
            __wake_up(&wq_write, 1, 1, NULL);
        case 2: // reset
            fnd_time = 0;
            outw(encode(fnd_time), (unsigned int)fnd_addr);        
        case 1: // pause
            if(is_doing == true){
                del_timer_sync(&do_timer);
                is_doing = false;
            }
            break;
        
        case 0: // start
            outw(encode(fnd_time), (unsigned int)fnd_addr);
            if(is_doing == false){
                do_timer.expires = get_jiffies_64() + HZ;
                do_timer.data = 5;
                do_timer.function = timer_function;
                add_timer(&do_timer);
                is_doing = true;
            }
            break;
        
        case 5: // flow
            fnd_time = (fnd_time + 1) % 3600;
            outw(encode(fnd_time), (unsigned int)fnd_addr);
            do_timer.expires = get_jiffies_64() + HZ;
            do_timer.data = 5;
            do_timer.function = timer_function;
            add_timer(&do_timer);
            break;

        case 3: // ready to end
            if(is_ending == false){
                printk("added..");
                end_timer.expires = get_jiffies_64() + 3 * HZ;
                end_timer.data = 4;
                end_timer.function = timer_function;
                add_timer(&end_timer);
                is_ending = true;
            }
            else {
                printk("deleted..");
                del_timer_sync(&end_timer);
                is_ending = false;
            }

        default:
            break;
    }
}

static unsigned short int encode(unsigned int time){
    unsigned short int value = 0;
    unsigned int tmp;
    
    // hour
    tmp = fnd_time / 60;
    value |= (tmp / 10) << 12;
    value |= (tmp % 10) << 8;
    
    // minute
    tmp = fnd_time % 60;
    value |= (tmp / 10) << 4;
    value |= (tmp % 10);

    return value;
}

static int stopwatch_open(struct inode *minode, struct file *mfile){
	int ret;
	int irq;

	printk("Open...\n");

	// home
	gpio_direction_input(IMX_GPIO_NR(1,11));
	irq = gpio_to_irq(IMX_GPIO_NR(1,11));
	printk(KERN_ALERT "IRQ Number : %d\n",irq);
	ret=request_irq(irq, home_handler, IRQF_TRIGGER_FALLING, "home", 0);

	// back
	gpio_direction_input(IMX_GPIO_NR(1,12));
	irq = gpio_to_irq(IMX_GPIO_NR(1,12));
	printk(KERN_ALERT "IRQ Number : %d\n",irq);
	ret=request_irq(irq, back_handler, IRQF_TRIGGER_FALLING, "back", 0);

	// vol+
	gpio_direction_input(IMX_GPIO_NR(2,15));
	irq = gpio_to_irq(IMX_GPIO_NR(2,15));
	printk(KERN_ALERT "IRQ Number : %d\n",irq);
	ret=request_irq(irq, volup_handler, IRQF_TRIGGER_FALLING, "volup", 0);

	// vol-
	gpio_direction_input(IMX_GPIO_NR(5,14));
	irq = gpio_to_irq(IMX_GPIO_NR(5,14));
	printk(KERN_ALERT "IRQ Number : %d\n",irq);
	ret=request_irq(irq, voldown_handler, IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING, "voldown", 0);

	return 0;
}

static int stopwatch_release(struct inode *minode, struct file *mfile){
	printk("Release...\n");
    free_irq(gpio_to_irq(IMX_GPIO_NR(1, 11)), NULL);
	free_irq(gpio_to_irq(IMX_GPIO_NR(1, 12)), NULL);
	free_irq(gpio_to_irq(IMX_GPIO_NR(2, 15)), NULL);
	free_irq(gpio_to_irq(IMX_GPIO_NR(5, 14)), NULL);
	
	return 0;
}

static int stopwatch_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos ){
    printk("Sleep...\n");
    interruptible_sleep_on(&wq_write);
	
    return 0;
}

static int __init stopwatch_init(void) {
    int error;
    
    stopwatch_dev = MKDEV(242, 0);
    error = register_chrdev_region(stopwatch_dev, 1, "stopwatch");
    if(error < 0){
        printk(KERN_WARNING "Can't get major\n");
        return error;
    }
    printk(KERN_ALERT "Major number : 242\n");

    cdev_init(&stopwatch_cdev, &stopwatch_fops);
    stopwatch_cdev.owner = THIS_MODULE;
    stopwatch_cdev.ops = &stopwatch_fops;
    error = cdev_add(&stopwatch_cdev, stopwatch_dev, 1); 
    if(error){
        printk(KERN_NOTICE "Register Error %d\n", error);
    }

    fnd_addr = ioremap(0x08000004, 0x04);
    init_timer(&do_timer);
    init_timer(&end_timer);

    return 0;
}

static void __exit stopwatch_exit(void) {
	del_timer_sync(&end_timer);
    del_timer_sync(&do_timer);
    iounmap(fnd_addr);

    cdev_del(&stopwatch_cdev);

	unregister_chrdev_region(stopwatch_dev, 1);
}

module_init(stopwatch_init);
module_exit(stopwatch_exit);
MODULE_LICENSE("GPL");
