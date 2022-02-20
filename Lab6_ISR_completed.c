/* ece4220lab1_isr.c
 * ECE4220/7220
 * Author: Luis Alberto Rivera
 
 Basic steps needed to configure GPIO interrupt and attach a handler.
 Check chapter 6 of the BCM2837 ARM Peripherals manual for details about
 the GPIO registers, how to configure, set, clear, etc.
 
 Note: this code is not functional. It shows some of the actual code that you need,
 but some parts are only descriptions, hints or recommendations to help you
 implement your module.
 
 You can compile your module using the same Makefile provided. Just add
 obj-m += YourModuleName.o
 */
 
#ifndef MODULE
#define MODULE
#endif
#ifndef __KERNEL__
#define __KERNEL__
#endif
 
#include <linux/module.h>
#include <linux/kernel.h>
#include <asm/io.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
 
MODULE_LICENSE("GPL");
 
/* Declare your pointers for mapping the necessary GPIO registers.
   You need to map:
    
   - Pin Event detect status register(s)
   - Rising Edge detect register(s) (either synchronous or asynchronous should work)
   - Function selection register(s)
   - Pin Pull-up/pull-down configuration registers
    
   Important: remember that the GPIO base register address is 0x3F200000, not the
   one shown in the BCM2835 ARM Peripherals manual.
*/
    //gpselect pointers
    unsigned long * gpsel0;
    unsigned long * gpsel1;
    unsigned long * gpsel2;
 
    //pin event
    unsigned long * gpeds0;
 
    //rising edge
    unsigned long * gparen0;
 
    //pin pullup/pulldown
    unsigned long * gppud;
    unsigned long * gppudclk0;
 
    //pin set and clear
    unsigned long * gpset0;
    unsigned long * gpclr0;
 
    
 
 
 
int mydev_id;    // variable needed to identify the handler
 
// Interrupt handler function. Tha name "button_isr" can be different.
// You may use printk statements for debugging purposes. You may want to remove
// them in your final code.
static irqreturn_t button_isr(int irq, void *dev_id)
{

    
	return IRQ_HANDLED;
}
 
int init_module()
{
    int dummy = 0;
 
    // Map GPIO registers
    // Remember to map the base address (beginning of a memory page)
    // Then you can offset to the addresses of the other registers
 
    gpsel0 = (unsigned long *) ioremap(0x3f200000, 4096);
    gpsel1 = gpsel0 + 1;
    gpsel2 = gpsel1 + 1;
 
    iowrite32(0x00000000, gpsel0);
    iowrite32(0x00000000, gpsel1);
    iowrite32(0x00000000, gpsel2);
 
    gpset0 = (unsigned long *) ioremap(0x3f20001c, 4096);
    gpclr0 = (unsigned long *) ioremap(0x3f200028, 4096);
 
    // Don't forget to configure all ports connected to the push buttons as inputs.
    
    // You need to configure the pull-downs for all those ports. There is
    // a specific sequence that needs to be followed to configure those pull-downs.
    // The sequence is described on page 101 of the BCM2837-ARM-Peripherals manual.
    // You can use  udelay(100);  for those 150 cycles mentioned in the manual.
    // It's not exactly 150 cycles, but it gives the necessary delay.
    // WiringPi makes it a lot simpler in user space, but it's good to know
    // how to do this "by hand".
        
   
    
 
    // Enable (Async) Rising Edge detection for all 5 GPIO ports.
 
    gparen0 = (unsigned long * )ioremap(0x3f20007c, 4096);
    iowrite32(0x001f0000, gparen0);
            
    // Request the interrupt / attach handler (line 79, doesn't match the manual...)
    // The third argument string can be different (you give the name)
    dummy = request_irq(79, button_isr, IRQF_SHARED, "Button_handler", &mydev_id);
 
    printk("Button Detection enabled.\n");
    return 0;
}
 
// Cleanup - undo whatever init_module did
void cleanup_module()
{
    // Good idea to clear the Event Detect status register here, just in case.
    
    // Disable (Async) Rising Edge detection for all 5 GPIO ports.
    gparen0 = (unsigned long *) ioremap(0x3f20007c, 4096);
    iowrite32(0x00000000, gparen0);
 
    // Remove the interrupt handler; you need to provide the same identifier
    free_irq(79, &mydev_id);
    
    printk("Button Detection disabled.\n");
}
