#ifndef MODULE
#define MODULE
#endif
#ifndef __KERNEL__
#define __KERNEL__
#endif

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <asm/io.h>

#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/uaccess.h>

#define MSG_SIZE 50
#define CDEV_NAME "Lab6"

MODULE_LICENSE("GPL");

unsigned long timer_interval_ns = 2272727;
static struct hrtimer hr_timer;
static int count = 0;
static int major;
static char msg[MSG_SIZE];
unsigned long *gpset0, *gpclr0, *gpsel0, *gpeds0;
char currentNote = 'A';

enum hrtimer_restart timer_callback(struct hrtimer *timer_for_restart){
		
	ktime_t currtime, interval;
	unsigned long overruns = 0;

	unsigned long readpin = ioread32(gpeds0);

	if((readpin & 0x00010000) != 0) timer_interval_ns = 2272727;
	else if((readpin & 0x00020000) != 0) timer_interval_ns = 2024783;
	else if((readpin & 0x00040000) != 0) timer_interval_ns = 1911132;
	else if((readpin & 0x00080000) != 0) timer_interval_ns = 1702620;
	else if((readpin & 0x00100000) != 0) timer_interval_ns = 1516875;

	iowrite32(0x001f0000, gpeds0);	

	currtime = ktime_get();
	interval = ktime_set(0, timer_interval_ns);
	
	overruns = hrtimer_forward(timer_for_restart, currtime, interval);

	if(count == 0) iowrite32(0x00000040, gpset0);
	else if (count == 1) iowrite32(0x00000040, gpclr0);
	count = (count + 1) % 2;

	return HRTIMER_RESTART;

}

static ssize_t device_write (struct file *filp, const char __user *buff, size_t len, loff_t *off){

	ssize_t dummy;

	if(len > MSG_SIZE)
		return -EINVAL;

	dummy = copy_from_user(msg, buff, len);
	if(len == MSG_SIZE)
		msg[len-1] = '\0';
	else
		msg[len] = '\0';

	printk("received %s\n", msg);

	printk("first char in received msg is %c\n", msg[0]);

	if(msg[0] == 'A')	{timer_interval_ns = 2272727; currentNote = 'A';}
	else if(msg[0] == 'B')	{timer_interval_ns = 2024783; currentNote = 'B';}
	else if(msg[0] == 'C') 	{timer_interval_ns = 1911132; currentNote = 'C';}
	else if(msg[0] == 'D') 	{timer_interval_ns = 1702620; currentNote = 'D';}
	else if(msg[0] == 'E') 	{timer_interval_ns = 1516875; currentNote = 'E';}
	else printk("msg didnt match a note\n");

	return len;
}

static ssize_t device_read(struct file *filp, char __user *buffer, size_t length, loff_t *offset){

	msg[0] = currentNote;
	ssize_t dummy = copy_to_user(buffer, msg, length);
	msg[0] = '\0';
	return length;

}

static struct file_operations fops = {
	.read = device_read,
	.write = device_write,
};

int timer_init(void){

	major = register_chrdev(0, CDEV_NAME, &fops);
	printk("sudo mknod /dev/%s c %d 0\n", CDEV_NAME, major);

	gpsel0 = (unsigned long *)ioremap(0x3f200000, 4096);
	iowrite32(0x00040000, gpsel0);

	gpset0 = (unsigned long *) ioremap(0x3f20001c, 4096);
	gpclr0 = (unsigned long *) ioremap(0x3f200028, 4096);
	gpeds0 = (unsigned long *) ioremap(0x3f200040, 4096);

	ktime_t ktime = ktime_set(0, timer_interval_ns);

	hrtimer_init(&hr_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);

	hr_timer.function = &timer_callback;

	hrtimer_start(&hr_timer, ktime, HRTIMER_MODE_REL);

	return 0;

}

void timer_exit(void){
	
	printk("trying to remove kmod\n");

	int ret;
	ret = hrtimer_cancel(&hr_timer);
	printk("got past timer cancel\n");
	if(ret)
		printk("the timer was still in use\n");
	else 
		printk("the timer was not running\n");
	printk("HR timer module uninstalling\n");
	
	unregister_chrdev(major, CDEV_NAME);
	printk("chr_dev unregistered\n");
}

module_init(timer_init);
module_exit(timer_exit);
