#include <linux/cdev.h>

#include "card.h"

/*
 * Local definitions
 */

#define card_BUFSIZE sizeof("ping")

 /*
 * Character device file operations
 */

int card_open(struct inode *inode, struct file *file) {

	unsigned int i_major = imajor(inode);
	unsigned int i_minor = iminor(inode);

	pr_info("%s()\n", __func__);

	if (i_major != major) {
		pr_err("Invalid major %d, expected %d\n", i_major, major);
		return -EINVAL;
	}
	if (i_minor != 0) {
		pr_err("card driver only handles minor 0!\n");
		return -EINVAL;
	}

	/* Make file descriptor "remember" its mirror device object,
	 * for later use in open() and write(). */
	file->private_data = card_dev;

	return 0;
}

int card_release(struct inode *inode /* unused */,
        struct file *file /* unused */) {

	pr_info("%s()\n", __func__);

	/* Nothing in particular to do */
	return 0;
}

ssize_t card_read(struct file *file, char __user *buf,
        size_t len, loff_t *off /* unused */) {

	struct card_dev *dev;
	/* Don't forget trailing NUL */
	char pongbuf[card_BUFSIZE+1];

	pr_info("%s(len=%zu)\n", __func__, len);

	dev = (struct card_dev *)file->private_data;
	if (dev->was_pinged) {
		/* Answer with a pong. Don't handle offsets */
		if (len < card_BUFSIZE) {
			return -EINVAL;
		}
		len = min(len, card_BUFSIZE+1);
		memset(pongbuf, 0, card_BUFSIZE+1);
		memcpy(pongbuf, "pong", card_BUFSIZE);
		if (copy_to_user(buf, pongbuf, len)) {
			pr_err("Failed to copy data to user\n");
			return -EFAULT;
		}
		dev->was_pinged = false;
	}
	else {
		/* Nothing to say */
		len = 0;
	}

	return len;
}

ssize_t card_write(struct file *file, const char __user *buf,
        size_t len, loff_t *off /* unused */) {

	struct card_dev *dev;
	/* Don't forget trailing NUL */
	char pingbuf[card_BUFSIZE+1];

	pr_info("%s(len=%zu)\n", __func__, len);

	dev = (struct card_dev *)file->private_data;
	/* copy_(from|to)_user() do not append a trailing NUL, so memset()
	 * our buffer to avoid risking corrupting the stack. */
	memset(pingbuf, 0, card_BUFSIZE+1);

	pr_info("Cutting data length to %zu\n", card_BUFSIZE);
	len = min(len, card_BUFSIZE);
	if (copy_from_user(pingbuf, buf, len)) {
		pr_err("Failed to copy user data\n");
		return -EFAULT;
	}

	if (strcmp(pingbuf, "ping") == 0) {
		pr_info("I was pinged!\n");
		dev->was_pinged = true;
	}
	else {
		pr_info("I was not pinged; I received %s\n", pingbuf);
	}

	/* Even if the message is not a ping, silently eat user data */
	return len;
}

/*
 *  Init & Exit
 */

static struct file_operations card_fops = {
	.owner   = THIS_MODULE,
	.open    = card_open,
	.release = card_release,
	.read    = card_read,
	.write   = card_write,
	/* Others functions are using the kernel's defaults */
};