
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
#include <linux/i2c.h>
#include "ssd1306.h"

#define SSD1306_SLAVE_ADDR		0x3C 

#define SSD1306_WR_CMD   		0x00	/* 写命令 */
#define SSD1306_WR_DATA  		0x01	/* 写数据 */


#define SSD1306_DEV_NAME		"ssd1306"		

struct ssd1306_dev
{
	struct 	cdev 	sd_cdev;
	dev_t			sd_id;
	struct class 	*sd_class;
	struct i2c_client *sd_i2c_client;
};

struct ssd1306_dev  ssd1306_device;

static int ssd1306_write_cmd(struct ssd1306_dev *pdev, uint8_t cmd)
{
	int ret = 0;
	uint8_t	 buf[2];
	struct i2c_msg msg[1];
	struct i2c_client *client = NULL;

	if (NULL == *pdev)
	{
		return -1;
	}
	
	client = (struct i2c_client*)pdev->sd_i2c_client;
	buf[0] = 0x00;
	buf[1] = cmd;
	msg[0].addr  = client->addr;
	msg[0].buff  = &buf[0];
	msg[0].size  = 2;
	msg[0].flags = 0;
	
	if(i2c_transfer(client->adapter, msg, 1) != 1)
	{
		ret =-1;
	}

	return ret;	
}

static int ssd1306_write_data(struct ssd1306_dev *pdev, uint8_t data)
{
	int ret = 0;
	uint8_t	 buf[2];
	struct i2c_msg msg[1];
	struct i2c_client *client = NULL;

	if (NULL == *pdev)
	{
		return -1;
	}
	
	client = (struct i2c_client*)pdev->sd_i2c_client;
	buf[0] = 0x40;
	buf[1] = data;
	msg[0].addr  = client->addr;
	msg[0].buff  = &buf[0];
	msg[0].size  = 2;
	msg[0].flags = 0;

	if(i2c_transfer(client->adapter, msg, 1) != 1)
	{
		ret =-1;
	}

	return ret;	
}


static int ssd1306_write_byte(struct ssd1306_dev *pdev, uint8_t cmd, uint8_t data)
{
	if (SSD1306_WR_CMD == cmd)
	{
		return ssd1306_write_cmd(pdev, data);
	}
	else if (SSD1306_WR_DATA == cmd)
	{
		return ssd1306_write_data(pdev, data);
	}
	else
	{
		return -1;
	}
}

static int ssd1306_init(struct ssd1306_dev *pdev)
{
	ssd1306_write_byte(pdev, SSD1306_WR_CMD, 0xAE);	/* display off */
	ssd1306_write_byte(pdev, SSD1306_WR_CMD, 0x00);	/* set low column address */
	ssd1306_write_byte(pdev, SSD1306_WR_CMD, 0x10);	/* set high column address */
	ssd1306_write_byte(pdev, SSD1306_WR_CMD, 0x40);	/* set start line address */
	ssd1306_write_byte(pdev, SSD1306_WR_CMD, 0xB0);	/* set page address */
	ssd1306_write_byte(pdev, SSD1306_WR_CMD, 0x81); /* contract control */
	ssd1306_write_byte(pdev, SSD1306_WR_CMD, 0xFF);	 
	ssd1306_write_byte(pdev, SSD1306_WR_CMD, 0xA1); /* set segment remap */
	ssd1306_write_byte(pdev, SSD1306_WR_CMD, 0xA6); /* normal / reverse */
	ssd1306_write_byte(pdev, SSD1306_WR_CMD, 0xA8); /* set multiplex ratio(1 to 64) */
	ssd1306_write_byte(pdev, SSD1306_WR_CMD, 0x3F); /* 1/32 duty */
	ssd1306_write_byte(pdev, SSD1306_WR_CMD, 0xC8); /* Com scan direction */
	ssd1306_write_byte(pdev, SSD1306_WR_CMD, 0xD3); /* set display offset */
	ssd1306_write_byte(pdev, SSD1306_WR_CMD, 0x00);
	
	ssd1306_write_byte(pdev, SSD1306_WR_CMD, 0xD5); /* set osc division */
	ssd1306_write_byte(pdev, SSD1306_WR_CMD, 0x80);
	
	ssd1306_write_byte(pdev, SSD1306_WR_CMD, 0xD8); /* set area color mode off */
	ssd1306_write_byte(pdev, SSD1306_WR_CMD, 0x05);
	
	ssd1306_write_byte(pdev, SSD1306_WR_CMD, 0xD9); /* Set Pre-Charge Period */
	ssd1306_write_byte(pdev, SSD1306_WR_CMD, 0xF1);
	
	ssd1306_write_byte(pdev, SSD1306_WR_CMD, 0xDA); /* set com pin configuartion */
	ssd1306_write_byte(pdev, SSD1306_WR_CMD, 0x12);
	
	ssd1306_write_byte(pdev, SSD1306_WR_CMD, 0xDB); /* set vcomh */
	ssd1306_write_byte(pdev, SSD1306_WR_CMD, 0x30);
	
	ssd1306_write_byte(pdev, SSD1306_WR_CMD, 0x8D); /* set charge pump enable */
	ssd1306_write_byte(pdev, SSD1306_WR_CMD, 0x14);
	
	ssd1306_write_byte(pdev, SSD1306_WR_CMD, 0xAF); /* turn on oled panel */
}


void ssd1306_clear(struct ssd1306_dev *pdev)  
{  
	uint8_t i;
	uint8_t j;
	
	for(i=0; i<8; i++)  
	{  
		ssd1306_write_byte (pdev, SSD1306_WR_CMD, 0xb0+i);   /* 设置页地址（0~7）*/
		ssd1306_write_byte (pdev, SSD1306_WR_CMD, 0x00);     /* 设置显示位置—列低地址 */
		ssd1306_write_byte (pdev, SSD1306_WR_CMD, 0x10);     /* 设置显示位置—列高地址 */
		for(j=0; j<128; j++)
		{
			ssd1306_write_byte(pdev, SSD1306_WR_DATA, 0x00); 
		}
	} 
}

void ssd1306_full(struct ssd1306_dev *pdev)  
{  
	uint8_t i;
	uint8_t j;
	
	for(i=0; i<8; i++)  
	{  
		ssd1306_write_byte (pdev, SSD1306_WR_CMD, 0xb0+i);    /* 设置页地址（0~7）*/
		ssd1306_write_byte (pdev, SSD1306_WR_CMD, 0x00);      /* 设置显示位置—列低地址 */
		ssd1306_write_byte (pdev, SSD1306_WR_CMD, 0x10);      /* 设置显示位置—列高地址 */  
		for(j=0; j<128; j++)
		{
			ssd1306_write_byte(pdev, SSD1306_WR_DATA, 0x01); 
		}
	} 
}

static int ssd1306_open(struct inode *inode, struct file *pfile) 
{ 
	int ret = 0;
	

	printk("ssd1306 opened\n");
	return ret;
} 

static ssize_t ssd1306_read(struct file *pfile, char __user *buf, size_t size, loff_t * offset) 
{ 
	int ret = 0;
	
	return ret;
}

static ssize_t ssd1306_write(struct file *pfile, const char __user *buf, size_t size, loff_t *offset) 
{ 
     return 0; 
} 

static long ssd1306_ioctl(struct file *pfile, unsigned int cmd, unsigned long arg)
{
	int ret = 0;

	return ret;
}

static int ssd1306_release(struct inode * inode , struct file * pfile) 
{ 
     return 0; 
} 

static struct file_operations ssd1306_fops = { 
     .owner = THIS_MODULE, 
     .open  = ssd1306_open, 
     .read  = ssd1306_read, 
     .write = ssd1306_write, 
     .release = ssd1306_release,
     .unlocked_ioctl = ssd1306_ioctl,
}; 
   
static int ssd1306_probe(struct i2c_client *client, const struct i2c_device_id *dev_id)  
{     
    struct device *dev; 
	int ret = -1;
	dev_t	id = 0;
	
	ret = alloc_chrdev_region(&id, 0, 1, SSD1306_DEV_NAME); 
	if (ret)
	{
		printk("alloc dev-id failed.\n");
		return ret;
	}

	ssd1306_device.sd_id = id; 
    cdev_init(&ssd1306_device.sd_cdev, &ssd1306_fops);
	ret = cdev_add(&ssd1306_device.sd_cdev, ssd1306_device.sd_id, 1);
    if (ret)
    {
    	unregister_chrdev_region(ssd1306_device.sd_id, 1);
        return ret;
    }

	ssd1306_device.sd_class = class_create(THIS_MODULE, "ssd1306_class");
	if (IS_ERR(ssd1306_device.sd_class)) 
	{
		printk("class_create failed.\n");
		cdev_del(&ssd1306_device.sd_cdev);
		ret = -EIO;
		return ret;
	}

	dev = device_create(ssd1306_device.sd_class, NULL, ssd1306_device.sd_id, NULL, SSD1306_DEV_NAME);
	if (IS_ERR(dev))   
    {   
         return PTR_ERR(dev);    
    }
	ssd1306_device.sd_i2c_client = client;

	ssd1306_init(&ssd1306_device);
	
   	return 0; 
} 
  
static int ssd1306_remove(struct i2c_client *client)
{ 
	device_destroy(ssd1306_device.sd_class, ssd1306_device.sd_id);
    class_destroy(ssd1306_device.sd_class);
    cdev_del(&ssd1306_device.sd_cdev);
    unregister_chrdev_region(ssd1306_device.sd_id, 1);
	
    return 0; 
} 
  
static const struct i2c_device_id ssd1306_id[] = { 
     {"ssd1306", SSD1306_SLAVE_ADDR}, 
     {} 
}; 
MODULE_DEVICE_TABLE(i2c, ssd1306_id); 
  
static struct of_device_id of_ssd1306_ids[] = {
   {.compatible = "none,ssd1306"},
   { }   
 };
 
static struct i2c_driver ssd1306_driver = { 
	.driver   = { 
	.owner    = THIS_MODULE, 
	.name     = SSD1306_DEV_NAME, 
	.of_match_table = of_ssd1306_ids,
	}, 
	.id_table = ssd1306_id, 
	.probe 	  = ssd1306_probe, 
	.remove   = ssd1306_remove, 
};

static int __init ssd1306_init(void) 
{ 
     i2c_add_driver(&ssd1306_driver);
     return 0; 
} 
  
static void __exit ssd1306_exit(void) 
{ 
     i2c_del_driver(&ssd1306_driver);
} 
  
module_init(ssd1306_init); 
module_exit(ssd1306_exit); 
  
MODULE_LICENSE("GPL"); 
