
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

#define DEV_NAME	"gp0"		

struct gpiopin_dev
{
	struct 	cdev 	dev;
	dev_t			dev_id;
	struct class 	*dev_class;
	struct device_node *nd;
	int 			gpio;
};

struct gpiopin_dev  gp0;


static int gp0_open(struct inode *inode, struct file *pfile) 
{ 
	int ret = 0;

	pfile->private_data = &gp0;
	
	return ret;
} 

static ssize_t gp0_read(struct file *pfile, char __user *buf, size_t size, loff_t *offset) 
{ 
	int ret = 0;
	struct gpiopin_dev *p;
	char level = 0;
	
	p = pfile->private_data;

	level = gpio_get_value(p->gpio);
	ret = copy_to_user(buf, &level, sizeof(level));
	
    return ret; 
} 


static ssize_t gp0_write(struct file *pfile, const char __user *buf, size_t size, loff_t *offset) 
{ 
	int ret = 0;
	struct gpiopin_dev *p;
	char level = 0;
	
	p = pfile->private_data;

	ret = copy_from_user(&level, buf, size);
	gpio_set_value(p->gpio, level);
	
    return ret; 
} 

static int gp0_release(struct inode * inode , struct file * pfile) 
{ 
     return 0; 
} 

static struct file_operations gp0_fops = { 
     .owner = THIS_MODULE, 
     .open  = gp0_open,
     .read  = gp0_read,
     .write = gp0_write, 
     .release = gp0_release,
}; 
   
static int gp0_probe(struct platform_device *pdev)  
{     
    struct device *dev; 
	int ret = -1;
	dev_t	id = 0;

	//gp0.nd = of_find_node_by_path("/gpiopin");
	gp0.nd =  pdev->dev.of_node;
	if(gp0.nd == NULL)
	{
		printk("get node faileed\n");
		return -1;
	}
	gp0.gpio = of_get_named_gpio(gp0.nd, "gpios", 0);
	if(gp0.gpio < 0)
	{
		printk("get gpio failed\n");
		return -1;
	}
	ret = gpio_request(gp0.gpio, "gp0");		/* 申请GPIO */
	if(ret < 0)
	{
		printk("gpio request failed\n");
		return ret;
	}
	
	ret = gpio_direction_output(gp0.gpio, 0);	/* output，low default */
	if(ret<0)
	{
		printk("gpio set failed\n");
		gpio_free(gp0.gpio);
		return ret;
	}
	ret = alloc_chrdev_region(&id, 0, 1, DEV_NAME); 
	if (ret)
	{
		printk("alloc dev-id failed.\n");
		return ret;
	}

	gp0.dev_id = id; 
    cdev_init(&gp0.dev, &gp0_fops);
	ret = cdev_add(&gp0.dev, gp0.dev_id, 1);
    if (ret)
    {
    	unregister_chrdev_region(gp0.dev_id, 1);
        return ret;
    }

	gp0.dev_class = class_create(THIS_MODULE, "gp0_class");
	if (IS_ERR(gp0.dev_class)) 
	{
		printk("class_create failed.\n");
		cdev_del(&gp0.dev);
		ret = -EIO;
		return ret;
	}

	dev = device_create(gp0.dev_class, NULL, gp0.dev_id, NULL, DEV_NAME);
	if (IS_ERR(dev))   
    {   
         return PTR_ERR(dev);    
    }
	
   	return 0; 
} 
  
static int gp0_remove(struct platform_device *pdev) 
{ 
	gpio_free(gp0.gpio);
	device_destroy(gp0.dev_class, gp0.dev_id);
    class_destroy(gp0.dev_class);
    cdev_del(&gp0.dev);
    unregister_chrdev_region(gp0.dev_id, 1);
	
    return 0; 
} 
  
  
static struct of_device_id of_gp0_ids[] = {
   {.compatible = "gpiopin"},
   { }   
 };
 
static struct platform_driver gp0_driver = { 
	.driver   = { 
	.owner    = THIS_MODULE, 
	.name     = DEV_NAME, 
	.of_match_table = of_match_ptr(of_gp0_ids),
	}, 
	.probe 	  = gp0_probe, 
	.remove   = gp0_remove, 
};

module_platform_driver(gp0_driver);

MODULE_LICENSE("GPL"); 
