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

#include "dev_driver.h"

static dev_t dev_dev;
static struct cdev dev_cdev;

static int dev_open(struct inode *, struct file *);
static int dev_release(struct inode *, struct file *);
static int dev_ioctl(struct file *, unsigned int, unsigned long);

irqreturn_t home_handler(int, void *, struct pt_regs *);
irqreturn_t volup_handler(int, void *, struct pt_regs *);
irqreturn_t voldown_handler(int, void *, struct pt_regs *);

static unsigned char *fnd_addr;
static unsigned char *dot_addr;
struct timer_list timer;

static unsigned int mode; // 1 : counter, 2 : timer
static unsigned int count;
static int time;
static bool istimerset; // true : timer set, false : timer doing

void print_blank(void);
void print_mode1(void);
void print_mode2(void);
static void timer_on(unsigned long);

static struct file_operations dev_fops =
{
	.open = dev_open,
	.unlocked_ioctl = dev_ioctl,
	.release = dev_release,
};

irqreturn_t home_handler(int irq, void* dev_id, struct pt_regs* reg) {
	// change mode and if we need to set variables then set variable
	if (mode == 1) {
		istimerset = true;
		time = 0;
		mode = 2;
		print_blank();
	}
	else if (mode == 2) {
		mode = 1;
		print_mode1();
	}

	return IRQ_HANDLED;
}

irqreturn_t volup_handler(int irq, void* dev_id, struct pt_regs* reg) {
	// if mode is timer and timer set -> add time 1 second
	if (mode == 2 && istimerset) {
		if (time < 60)
			time++;
		print_mode2();
	}

	return IRQ_HANDLED;
}

irqreturn_t voldown_handler(int irq, void* dev_id, struct pt_regs* reg) {
	// if mode is timer and timer set -> start timer
	if (mode == 2 && istimerset) {
		istimerset = false;
		print_mode2();

		timer.expires = get_jiffies_64() + HZ;
		timer.function = timer_on;
		add_timer(&timer);
	}
	return IRQ_HANDLED;
}

static int dev_open(struct inode *minode, struct file *mfile) {
	int ret;
	int irq;

	// initialize variables and clear board
	count = 0;
	mode = 1;
	print_blank();

	// home
	gpio_direction_input(IMX_GPIO_NR(1, 11));
	irq = gpio_to_irq(IMX_GPIO_NR(1, 11));
	printk(KERN_ALERT "IRQ Number : %d\n", irq);
	ret = request_irq(irq, home_handler, IRQF_TRIGGER_FALLING, "home", 0);

	// vol+
	gpio_direction_input(IMX_GPIO_NR(2, 15));
	irq = gpio_to_irq(IMX_GPIO_NR(2, 15));
	printk(KERN_ALERT "IRQ Number : %d\n", irq);
	ret = request_irq(irq, volup_handler, IRQF_TRIGGER_FALLING, "volup", 0);

	// vol-
	gpio_direction_input(IMX_GPIO_NR(5, 14));
	irq = gpio_to_irq(IMX_GPIO_NR(5, 14));
	printk(KERN_ALERT "IRQ Number : %d\n", irq);
	ret = request_irq(irq, voldown_handler, IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING, "voldown", 0);

	return 0;
}

static int dev_release(struct inode *minode, struct file *mfile) {
	// clear board
	print_blank();

	// unregister interrrupt
	free_irq(gpio_to_irq(IMX_GPIO_NR(1, 11)), NULL);
	free_irq(gpio_to_irq(IMX_GPIO_NR(2, 15)), NULL);
	free_irq(gpio_to_irq(IMX_GPIO_NR(5, 14)), NULL);

	return 0;
}

static int dev_ioctl(struct file *inode, unsigned int cmd, unsigned long arg)
{	// if cmd is 1, it means puzzle moved -> if mode is counter, then add count
	if (_IOC_NR(cmd) == 1 && mode == 1) {
		count++;
		print_mode1();
	}
	// if cmd is 2, it means get state from kernel -> if mode is timer and timer finished, then return 1 -> (finish activity)
	else if (_IOC_NR(cmd) == 2 && mode == 2) {
		if (!istimerset && time == 0) {
			return 1;
		}
	}
	return 0;
}

void print_blank() {
	// clear board
	int i;
	unsigned short int value = 0;
	outw(value, (unsigned int)fnd_addr);

	for (i = 0; i < 10; i++) {
		value = dot_set_blank[i] & 0x7F;
		outw(value, (unsigned int)dot_addr + i * 2);
	}
}

void print_mode1() {
	// print to board with counter variables
	int i, temp;
	unsigned short int value = 0;
	value += ((count % 10000) / 1000) << 12;
	value += ((count % 1000) / 100) << 8;
	value += ((count % 100) / 10) << 4;
	value += (count % 10);
	outw(value, (unsigned int)fnd_addr);

	for (i = 0; i < 10; i++) {
		temp = count % 10;
		value = dot_number[temp][i] & 0x7F;
		outw(value, (unsigned int)dot_addr + i * 2);
	}
}

void print_mode2() {
	// print to board with timer variables, it also check timer mode(set, doing)
	int i;
	unsigned short int value = 0;
	value += (time / 10) << 4;
	value += (time % 10);
	outw(value, (unsigned int)fnd_addr);

	if (!istimerset && time < 10) {
		for(i = 0; i < 10; i++){
			value = dot_number[time][i] & 0x7F;
			outw(value, (unsigned int)dot_addr + i * 2);
		}
	}
}

static void timer_on(unsigned long data) {
	// add timer with 1 second interval when time is going to 0
	if (time > 0) {
		time--;
		print_mode2();

		timer.expires = get_jiffies_64() + HZ;
		timer.function = timer_on;
		add_timer(&timer);
	}
}

static int __init dev_init(void) {
	int error;

	// register module
	dev_dev = MKDEV(242, 0);
	error = register_chrdev_region(dev_dev, 1, "dev");
	if (error < 0) {
		printk(KERN_WARNING "Can't get major\n");
		return error;
	}
	printk(KERN_ALERT "Major number : 242\n");

	cdev_init(&dev_cdev, &dev_fops);
	dev_cdev.owner = THIS_MODULE;
	dev_cdev.ops = &dev_fops;
	error = cdev_add(&dev_cdev, dev_dev, 1);
	if (error) {
		printk(KERN_NOTICE "Register Error %d\n", error);
	}

	// mapping logical address to physical address
	fnd_addr = ioremap(0x08000004, 0x04);
	dot_addr = ioremap(0x08000210, 0x10);

	init_timer(&timer);

	return 0;
}

static void __exit dev_exit(void) {
	del_timer_sync(&timer);

	// unmapping physical address to logical address
	iounmap(fnd_addr);
	iounmap(dot_addr);

	// unregister module
	cdev_del(&dev_cdev);
	unregister_chrdev_region(dev_dev, 1);
}

module_init(dev_init);
module_exit(dev_exit);
MODULE_LICENSE("GPL");
