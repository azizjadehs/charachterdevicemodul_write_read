
#ifndef DYNMODUL_MAJOR
#define DYNMODUL_MAJOR 0   /* dynamic major by default */
#endif

#ifndef DYNMODUL_NR_DEVS
#define DYNMODUL_NR_DEVS 4
#endif

#ifndef DYNMODUL_QUANTUM
#define DYNMODUL_QUANTUM 4000
#endif

#ifndef DYNMODUL_QSET
#define DYNMODUL_QSET 1000
#endif

#ifndef DYNMODUL_P_BUFFER
#define DYNMODUL_P_BUFFER 4000
#endif



struct dynmodul_qset {
	void **data;
	struct dynmodul_qset *next;
};

struct dynmodul_dev {
	struct dynmodul_qset *data;
	int quantum;
	int qset;
	unsigned long size;
	unsigned int access_key;
	struct mutex lock;
	struct cdev cdev;
};


extern int dynmodul_major;
extern int dynmodul_nr_devs;
extern int dynmoudl_quantum;
extern int dynmodul_qset;

extern int dynmodul_p_buffer;


int	dynmodul_trim(struct dynmodul_dev *dev);

static ssize_t dynmodul_read(struct file *filp, char *buffer, size_t length, loff_t *offset);

static ssize_t dynmodul_write(struct file *filp, const char *buff, size_t len, loff_t *off);
