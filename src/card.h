struct card_dev {
	struct cdev cdev;
};

/* Major will always be dynamically allocated */
extern int major;
extern struct card_dev *card_dev;
