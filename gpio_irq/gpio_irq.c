
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/ioctl.h>
#include <linux/uaccess.h>		
#include <asm/io.h>
#include <linux/platform_device.h>
#include <linux/version.h>
#include <linux/delay.h> 
#include <linux/gpio.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/of_address.h>
#include <linux/poll.h>  
#include <linux/interrupt.h>
#include "gpio_irq.h"

#define DEV_NAME	"gq0"		

struct gpioirq_dev
{
	struct 	cdev 	dev;
	dev_t			dev_id;
	struct class 	*dev_class;
	int 			gpio;
	int				irq;
	enum of_gpio_flags				irq_mode;
	wait_queue_head_t 	r_queue;
	bool				r_en;
	struct fasync_struct *r_sync;
};

struct gpioirq_dev  gq0;

static int gq0_open(struct inode *inode, struct file *pfile) 
{ 
	int ret = 0;

	pfile->private_data = &gq0;
	
	return ret;
} 

static ssize_t gq0_read(struct file *pfile, char __user *buf, size_t size, loff_t * offset) 
{ 
	int ret = 0;
	struct gpioirq_dev *p;
	char level = 0;
	
	p = pfile->private_data;

	wait_event_interruptible(p->r_queue, p->r_en);
	level = gpio_get_value(p->gpio);
	ret = copy_to_user(buf, &level, 1);
	
    return ret; 
} 

static int gq0_release(struct inode * inode , struct file * pfile) 
{ 
     return 0; 
} 

static struct file_operations gq0_fops = { 
     .owner = THIS_MODULE, 
     .open  = gq0_open,  
     .read  = gq0_read, 
     .release = gq0_release,
}; 

static irqreturn_t gq0_irq_handle(int irq, void *dev_id)
{
	struct gpioirq_dev *p;

	p = (struct gpioirq_dev *)dev_id;
	p->r_en = true;
    wake_up_interruptible(&(p->r_queue));	/* 唤醒休眠进程(读) */

	/* 通知进程数据可读
	 * SIGIO:信号类型
	 * POLL_IN:普通数据可读
	 */
	kill_fasync(&p->r_sync, SIGIO, POLL_IN);
	
	return IRQ_HANDLED;
}

static int gq0_probe(struct platform_device *pdev)  
{     
    struct device *dev; 
	int ret = -1;
	dev_t	id = 0;
	struct device_node *nd;
	
	nd = pdev->dev.of_node;
	printk("gq0 init\n");
	if(nd == NULL)
	{
		printk("get node faileed\n");
		return -1;
	}
	//gq0.gpio = of_get_named_gpio_flags(nd, "gpios", 0, &(gq0.irq_mode));	
	gq0.gpio = of_get_named_gpio(nd, "gpios", 0);	
	if(gq0.gpio < 0)
	{
		printk("get gpio failed\n");
		return -1;
	}
	
	if (!gpio_is_valid(gq0.gpio)) 
	{
		printk("gpio [%d] is invalid\n", gq0.gpio);
		return -1;
	}
	ret = gpio_request(gq0.gpio, "gq0");		/* 申请GPIO */
	if(ret < 0)
	{
		printk("gpio request failed\n");
		return ret;
	}

	gq0.irq = gpio_to_irq(gq0.gpio);	/* 中断号映射 */
	
	//ret = request_irq(gq0.irq, gq0_irq_handle, gq0.irq_mode, "gq0", &gq0);/* 注册中断 */
	ret = request_irq(gq0.irq, gq0_irq_handle, IRQF_TRIGGER_FALLING, "gq0", &gq0);/* 注册中断 */
	if(ret<0)
	{
		printk("request gq0 irq failed\n");
		free_irq(gq0.irq, &gq0);
		gpio_free(gq0.gpio);
		return ret;
	}
	ret = alloc_chrdev_region(&id, 0, 1, DEV_NAME); 
	if (ret)
	{
		printk("alloc dev-id failed.\n");
		return ret;
	}

	gq0.dev_id = id; 
    cdev_init(&gq0.dev, &gq0_fops);
	ret = cdev_add(&gq0.dev, gq0.dev_id, 1);
    if (ret)
    {
    	unregister_chrdev_region(gq0.dev_id, 1);
        return ret;
    }

	gq0.dev_class = class_create(THIS_MODULE, "gq0_class");
	if (IS_ERR(gq0.dev_class)) 
	{
		printk("class_create failed.\n");
		cdev_del(&gq0.dev);
		ret = -EIO;
		return ret;
	}

	dev = device_create(gq0.dev_class, NULL, gq0.dev_id, NULL, DEV_NAME);
	if (IS_ERR(dev))   
    {   
         return PTR_ERR(dev);    
    }
	
   	return 0; 
} 
  
static int gq0_remove(struct platform_device *pdev) 
{ 
	gpio_free(gq0.gpio);
	device_destroy(gq0.dev_class, gq0.dev_id);
    class_destroy(gq0.dev_class);
    cdev_del(&gq0.dev);
    unregister_chrdev_region(gq0.dev_id, 1);
	
    return 0; 
} 
  
  
static struct of_device_id of_gq0_ids[] = {
   {.compatible = "gpioirq"},
   { }   		   
 };
 
static struct platform_driver gq0_driver = { 
	.driver   = { 
	.owner    = THIS_MODULE, 
	.name     = DEV_NAME, 
	.of_match_table = of_gq0_ids,
	}, 
	.probe 	  = gq0_probe, 
	.remove   = gq0_remove, 
};

module_platform_driver(gq0_driver);

MODULE_LICENSE("GPL"); 
