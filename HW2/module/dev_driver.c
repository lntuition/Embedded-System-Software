#include <linux/module.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/delay.h>

#include <asm/io.h>
#include <asm/uaccess.h>
#include <linux/kernel.h>
#include <linux/ioport.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/version.h>

#include "./dev_driver.h"
#include <asm/ioctl.h>

#define MAJOR_NUMBER 242
#define DEV_DRIVER_NAME "dev_driver"

/* Physical address */
#define DOT_ADDRESS 0x08000210
#define FND_ADDRESS 0x08000004
#define LED_ADDRESS 0x08000016
#define LCD_ADDRESS 0x08000090
 
static int port_usage = 0;

/* Logical address */
static unsigned char *dot_addr;
static unsigned char *fnd_addr;
static unsigned char *led_addr;
static unsigned char *lcd_addr;

ssize_t dev_write(struct file *, const char *, size_t, loff_t *);
static void dev_timer_function(unsigned long);
int dev_open(struct inode *, struct file *);
int dev_release(struct inode *, struct file *);
long dev_ioctl(struct file *, unsigned int, unsigned long);

struct timer_list timer;
static struct struct_data {
    int location;
    unsigned char number;
    int count;
    int interval;

    int id_start;
    int name_start;
    int id_to;
    int name_to;
};
struct struct_data dev_data;

struct file_operations dev_fops = // file operations
{
	owner:		        THIS_MODULE,
	open:		        dev_open,
	write:		        dev_write,	
	release:	        dev_release,
    unlocked_ioctl:     dev_ioctl,
};

int dev_open(struct inode *minode, struct file *mfile) 
{	
	if(port_usage != 0) 
        return -EBUSY;
	port_usage = 1;
	return 0;
}

int dev_release(struct inode *minode, struct file *mfile) 
{
	port_usage = 0;
	return 0;
}

ssize_t dev_write(struct file *inode, const char *gdata, size_t length, loff_t *off_what) 
{
	unsigned int data;
	const char *tmp = gdata;

    // get data from application program
	if (copy_from_user(&data, tmp, sizeof(data)))
		return -EFAULT;

    /* Initialize by gdata */
    dev_data.location = (data >> 24) & 0xFF;
    dev_data.number = (data >> 16) & 0xFF;
    dev_data.count = (data >> 8) & 0xFF;
    dev_data.interval = data & 0xFF;

    dev_data.id_start = 0;
    dev_data.name_start = 0;
    dev_data.id_to = 1;
    dev_data.name_to = 1;

    dev_timer_function((unsigned long)(&dev_data));

	return 0;
}

static void dev_timer_function(unsigned long data)
{
    int i;
    unsigned short int value = 0;
    unsigned char text[16];

    // end timer and clear device
    if(--dev_data.count < 0){
        outw(0, (unsigned int)fnd_addr);
        outw(0, (unsigned int)led_addr);
        for(i=0; i<10; i++){
		    outw(0, (unsigned int)dot_addr + i*2 );
        }
        for(i=0; i<32; i+=2){
            outw(0, (unsigned int)lcd_addr + i);
        }
        return;
    }

    /* Write Value to device */
    value = dev_data.number << (4 * dev_data.location);
    outw(value, (unsigned int)fnd_addr); // fnd

    value = 0x01 << (8 - dev_data.number);
    outw(value, (unsigned int)led_addr); // led

    for(i=0; i<10; i++){
        value = dot_number[dev_data.number][i] & 0x7F;
		outw(value, (unsigned int)dot_addr + i*2 ); // dot
    }

    memset(text, 0, sizeof(text));
    memcpy(text + dev_data.id_start, lcd_id, sizeof(lcd_id));
    for(i=0; i<16; i+=2){
        value = (text[i] & 0xFF) << 8 | text[i+1] & 0xFF;
        outw(value, (unsigned int)lcd_addr + i); // lcd
    }

    memset(text, 0, sizeof(text));
    memcpy(text + dev_data.name_start, lcd_name, sizeof(lcd_name));
    for(i=0; i<16; i+=2){
        value = (text[i] & 0xFF) << 8 | text[i+1] & 0xFF;
        outw(value, (unsigned int)lcd_addr + 16 + i); // lcd
    }

    /* Changed to next value */
    if(++dev_data.number > 8){
        dev_data.number = 1;
        if(--dev_data.location < 0){
            dev_data.location = 3; 
        }
    }

    dev_data.id_start += dev_data.id_to;
    if(dev_data.id_start > 8){
        dev_data.id_start = 7;
        dev_data.id_to *= -1;
    }
    else if(dev_data.id_start < 0){
        dev_data.id_start = 1;
        dev_data.id_to *= -1;
    }

    dev_data.name_start += dev_data.name_to;
    if(dev_data.name_start > 4){
        dev_data.name_start = 3;
        dev_data.name_to *= -1;
    }
    else if(dev_data.name_start < 0){
        dev_data.name_start = 1;
        dev_data.name_to *= -1;
    }

    /* Add timer */
    timer.expires = get_jiffies_64() + (dev_data.interval * (HZ / 10));
    timer.function = dev_timer_function;
    add_timer(&timer);
}

long dev_ioctl(struct file *inode, unsigned int cmd, unsigned long arg)
{
    if(_IOC_NR(cmd) == 0){
        unsigned int data;
	    unsigned int *tmp = (unsigned int *)arg;

        // get data from application program
	    if (copy_from_user(&data, tmp, sizeof(data)))
		    return -EFAULT;

        /* Initialize by data */
        dev_data.location = (data >> 24) & 0xFF;
        dev_data.number = (data >> 16) & 0xFF;
        dev_data.count = (data >> 8) & 0xFF;
        dev_data.interval = data & 0xFF;

        dev_data.id_start = 0;
        dev_data.name_start = 0;
        dev_data.id_to = 1;
        dev_data.name_to = 1;

        /* Add timer function */
        dev_timer_function((unsigned long)(&dev_data));
    }
    return 0;
}

int __init dev_init(void)
{
	int result;

    // register this device driver with file operation
	result = register_chrdev(MAJOR_NUMBER, DEV_DRIVER_NAME, &dev_fops);
	if(result < 0) {
		printk(KERN_WARNING"Can't get any major\n");
		return result;
	}

    // mapping physical address to kernel address
    fnd_addr = ioremap(FND_ADDRESS, 0x04);
	dot_addr = ioremap(DOT_ADDRESS, 0x10);
    led_addr = ioremap(LED_ADDRESS, 0x01);
    lcd_addr = ioremap(LCD_ADDRESS, 0x32);

    // initialize timer
    init_timer(&timer);

	printk("init module, %s major number : %d\n", DEV_DRIVER_NAME, MAJOR_NUMBER);
	return 0;
}

void __exit dev_exit(void) 
{
    // remove timer
    del_timer_sync(&timer);

    // unmapping physical address to kernel address
    iounmap(fnd_addr);
	iounmap(dot_addr);
    iounmap(led_addr);
    iounmap(lcd_addr);
    
    // unregister(delete) this device driver
    unregister_chrdev(MAJOR_NUMBER, DEV_DRIVER_NAME);
}

module_init(dev_init);
module_exit(dev_exit);
MODULE_LICENSE("GPL");
