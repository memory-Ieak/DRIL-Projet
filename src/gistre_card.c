#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

#include "card.h"

/*
 * Metadata
 */

MODULE_AUTHOR("BELLIER Sacha; ESCAFFRE-FAURE Dimitri; SICARD Antoine");
MODULE_DESCRIPTION("gistre_card");
MODULE_LICENSE("GPL v2");

/*
 * Module static data
 */

int major;
struct card_dev *card_dev;

/*
 * Module Init and Exit functions
 */

static struct card_dev *card_create(void) {

	struct card_dev *dev = kmalloc(sizeof(*dev), GFP_KERNEL);
	if (! dev) {
		return NULL;
	}

	dev->cdev.owner = THIS_MODULE;
	cdev_init(&dev->cdev, &card_fops);

	dev->was_pinged = false;

	return dev;
}

static void card_destroy(struct card_dev *dev) {

	cdev_del(&dev->cdev);
	kfree(dev);
}

__exit
static void card_exit(void) {
	pr_info("Goodbye, GISTRE card !\n");
	dev_t dev;

	dev = MKDEV(major, 0);
	card_destroy(card_dev);
	unregister_chrdev_region(dev, 1);
}

__init
static int card_init(void) {
	pr_info("Hello, GISTRE card !\n");

	dev_t dev;
	int ret;
	const char devname[] = "card";

	Allocate major
	ret = alloc_chrdev_region(&dev, 0, 1, devname);
	if (ret < 0) {
		pr_info("Failed to allocate major\n");
		return 1;
	}
	else {
		major = MAJOR(dev);
		pr_info("Got major %d\n", major);
	}

	Register char device
	card_dev = card_create();
	if (! card_dev) {
		pr_err("Failed to init card_dev\n");
		return -ENOMEM;
	}

	if (cdev_add(&card_dev->cdev, dev, 1) < 0) {
		pr_err("Failed to register char device\n");
		return -ENOMEM;
	}
	return 0;
}

module_init(card_init);
module_exit(card_exit);
