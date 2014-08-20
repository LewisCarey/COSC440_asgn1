
/**
 * File: asgn1.c
 * Date: 13/03/2011
 * Author: Your Name 
 * Version: 0.1
 *
 * This is a module which serves as a virtual ramdisk which disk size is
 * limited by the amount of memory available and serves as the requirement for
 * COSC440 assignment 1 in 2012.
 *
 * Note: multiple devices and concurrent modules are not supported in this
 *       version.
 */
 
/* This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/list.h>
#include <asm/uaccess.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/proc_fs.h>
#include <linux/device.h>

#define MYDEV_NAME "asgn1"
#define MYIOC_TYPE 'k'

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lewis Carey");
MODULE_DESCRIPTION("COSC440 asgn1");

static struct proc_dir_entry *my_proc;


/**
 * The node structure for the memory page linked list.
 */ 
typedef struct page_node_rec {
  struct list_head list;
  struct page *page;
} page_node;

typedef struct asgn1_dev_t {
  dev_t dev;            /* the device */
  struct cdev *cdev;
  struct list_head mem_list; 
  int num_pages;        /* number of memory pages this module currently holds */
  size_t data_size;     /* total data size in this module */
  atomic_t nprocs;      /* number of processes accessing this device */ 
  atomic_t max_nprocs;  /* max number of processes accessing this device */
  struct kmem_cache *cache;      /* cache memory */
  struct class *class;     /* the udev class */
  struct device *device;   /* the udev device node */
} asgn1_dev;

asgn1_dev asgn1_device;


int asgn1_major = 0;                      /* major number of module */  
int asgn1_minor = 0;                      /* minor number of module */
int asgn1_dev_count = 1;                  /* number of devices */


/**
 * This function frees all memory pages held by the module.
 */
void free_memory_pages(void) {
  page_node *curr;

  /* COMPLETE ME */
  /**
   * Loop through the entire page list {
   *   if (node has a page) {
   *     free the page
   *   }
   *   remove the node from the page list
   *   free the node
   * }
   * reset device data size, and num_pages
   */  
	// Define some temporary container variables
	struct list_head *ptr;
	struct list_head *tmp;	

	printk(KERN_WARNING "Freeing memory pages");
	
	// Iterate SAFELY over the list
	list_for_each_safe(ptr, tmp, &asgn1_device.mem_list) {
		
		// Free the pages if they exist
		curr = list_entry(ptr, page_node, list);
		if (curr->page) {
			__free_page(curr->page);
			list_del(&curr->list);
			kfree(curr);
		}	
		
	}		
	
	// Reset variable trackers
	asgn1_device.num_pages = 0;
	asgn1_device.data_size = 0;
}


/**
 * This function opens the virtual disk, if it is opened in the write-only
 * mode, all memory pages will be freed.
 */
int asgn1_open(struct inode *inode, struct file *filp) {
  /* COMPLETE ME */
  /**
   * Increment process count, if exceeds max_nprocs, return -EBUSY
   *
   * if opened in write-only mode, free all memory pages
   *
   */
	// All operations should be atomic due to the concurrent nature of processes
	atomic_inc(&asgn1_device.nprocs);
	// Reject device if too many processes are accessing it
	if (atomic_read(&asgn1_device.nprocs) > atomic_read(&asgn1_device.max_nprocs)) {
		printk(KERN_ERR "Too many processes accessing this device");
		return -EBUSY;
	}
	// Write only - free memory pages
	if(filp->f_flags == O_WRONLY) {
		free_memory_pages();
	}

	// Appending - set the pointer to end of the file
	if(filp->f_flags & O_APPEND) {
		printk(KERN_WARNING "Append flag set");
		filp->f_pos = asgn1_device.data_size;
	}
	
	// Truncation - Remove current writes and replace with new data
	if (filp->f_flags & O_TRUNC) {
		printk(KERN_WARNING "Truncate flag set");
		free_memory_pages();
	}

  return 0; /* success */
}


/**
 * This function releases the virtual disk, but nothing needs to be done
 * in this case. 
 */
int asgn1_release (struct inode *inode, struct file *filp) {
  /* COMPLETE ME */
  /**
   * decrement process count
   */
	// Decrement process count
	atomic_dec(&asgn1_device.nprocs);

  return 0;
}


/**
 * This function reads contents of the virtual disk and writes to the user 
 */
ssize_t asgn1_read(struct file *filp, char __user *buf, size_t count,
		 loff_t *f_pos) {
  size_t size_read = 0;     /* size read from virtual disk in this function */
  size_t begin_offset;      /* the offset from the beginning of a page to
			       start reading */
  int begin_page_no = *f_pos / PAGE_SIZE; /* the first page which contains
					     the requested data */
  int curr_page_no = 0;     /* the current page number */
  size_t curr_size_read;    /* size read from the virtual disk in this round */
  size_t size_to_be_read;   /* size to be read in the current round in 
			       while loop */

  struct list_head *ptr = asgn1_device.mem_list.next;
  page_node *curr;

  /* COMPLETE ME */
  /**
   * check f_pos, if beyond data_size, return 0
   * 
   * Traverse the list, once the first requested page is reached,
   *   - use copy_to_user to copy the data to the user-space buf page by page
   *   - you also need to work out the start / end offset within a page
   *   - Also needs to handle the situation where copy_to_user copy less
   *       data than requested, and
   *       copy_to_user should be called again to copy the rest of the
   *       unprocessed data, and the second and subsequent calls still
   *       need to check whether copy_to_user copies all data requested.
   *       This is best done by a while / do-while loop.
   *
   * if end of data area of ramdisk reached before copying the requested
   *   return the size copied to the user space so far
   */

	int bytes_not_read;
	
	// Check to see if f_pos is beyond the size of data in memory
	if (filp->f_pos > asgn1_device.data_size) return 0;

	// Find the first node to read from based on the file position
	list_for_each(ptr, &asgn1_device.mem_list) {
		if (curr_page_no == begin_page_no) break;
		curr_page_no++;
	}
	curr = list_entry(ptr, page_node, list);	

	// Make sure the read does not go past our writes
	if (*f_pos + count > asgn1_device.data_size) count = asgn1_device.data_size - *f_pos;
	
	// While we still have things to be read
	while (size_read < count) {
		// Ensure we won't try to read past what we have written
		begin_offset = *f_pos % PAGE_SIZE;
		size_to_be_read = min((int) count - size_read, (int) PAGE_SIZE - begin_offset);
		
		printk(KERN_WARNING "Attempting to read %d bytes at %d offset", size_to_be_read, begin_offset);

		// Read data from the page
		bytes_not_read = copy_to_user(buf + size_read, page_address(curr->page) + begin_offset, size_to_be_read);
		
		// Update the variables and pointers, keeping track of what we have read
		curr_size_read = size_to_be_read - bytes_not_read;
		*f_pos += curr_size_read;
		size_read += curr_size_read;	
		
		// If our read fails, let the user know
		if (bytes_not_read) break;
		
		// Keep traversing
		ptr = ptr->next;
		curr = list_entry(ptr, page_node, list);
		curr_page_no++;	
	}

  return size_read;
}




static loff_t asgn1_lseek (struct file *file, loff_t offset, int cmd)
{
    loff_t testpos;

    size_t buffer_size = asgn1_device.num_pages * PAGE_SIZE;

    /* COMPLETE ME */
    /**
     * set testpos according to the command
     *
     * if testpos larger than buffer_size, set testpos to buffer_size
     * 
     * if testpos smaller than 0, set testpos to 0
     *
     * set file->f_pos to testpos
     */
	// Seeking normally
	if (cmd == SEEK_SET) {
		testpos = offset;
	} 
	// Seeking from the current position
	else if (cmd == SEEK_CUR) {
		testpos = file->f_pos + offset;
	} 
	// Seeking from the end
	else if (cmd == SEEK_END){
		testpos = asgn1_device.data_size + offset;
	} 
	// No meaningful value passed
	else {
		printk(KERN_ERR "No valid command given for device lseek");
		return -EINVAL;
	}
	// Checking against buffer size and 0 to prevent errors
	if (testpos > buffer_size) {
		testpos = buffer_size;
	}

	if (testpos < 0) {
		testpos = 0;
	}

	file->f_pos = testpos;


    printk (KERN_INFO "Seeking to pos=%ld\n", (long)testpos);
    return testpos;
}


/**
 * This function writes from the user buffer to the virtual disk of this
 * module
 */
ssize_t asgn1_write(struct file *filp, const char __user *buf, size_t count,
		  loff_t *f_pos) {
  size_t orig_f_pos = *f_pos;  /* the original file position */
  size_t size_written = 0;  /* size written to virtual disk in this function */
  size_t begin_offset;      /* the offset from the beginning of a page to
			       start writing */
  int begin_page_no = *f_pos / PAGE_SIZE;  /* the first page this finction
					      should start writing to */

  int curr_page_no = 0;     /* the current page number */
  size_t curr_size_written; /* size written to virtual disk in this round */
  size_t size_to_be_written;  /* size to be read in the current round in 
				 while loop */
  
  struct list_head *ptr = asgn1_device.mem_list.next;
  page_node *curr;

  /* COMPLETE ME */
  /**
   * Traverse the list until the first page reached, and add nodes if necessary
   *
   * Then write the data page by page, remember to handle the situation
   *   when copy_from_user() writes less than the amount you requested.
   *   a while loop / do-while loop is recommended to handle this situation. 
   */

	// First thing we need to do is calculate how many pages we are going to need,
	// and add these to the linked list
	
	// Variables we need
	int final_page_no = (*f_pos -1 + count) / PAGE_SIZE;
	size_t size_not_written;
	
	// While we still have pages to set up
	while (asgn1_device.num_pages < final_page_no + 1) {
		// Allocate a page in memory	
		curr = kmalloc(sizeof(page_node), GFP_KERNEL);
		curr->page = alloc_page(GFP_KERNEL);
		// If we didn't manage to allocate anything, return error
		if (curr->page == NULL) {
			printk(KERN_WARNING "Not enough memory left fro writing\n");
			return size_written;
		}
		// Add the allocated memory to the list
		list_add_tail(&(curr->list), &asgn1_device.mem_list);
		asgn1_device.num_pages++;
	}
	printk(KERN_INFO "%s: %d pages total\n", MYDEV_NAME, asgn1_device.num_pages);
	printk(KERN_INFO "%s: %d to be written", MYDEV_NAME, count);
	
	// Now that all our nodes and pages are allocated, traverse the list to find
	// the one to start on.
	list_for_each(ptr, &asgn1_device.mem_list) {
		if(curr_page_no == begin_page_no) break;
		curr_page_no++;
	}
	curr = list_entry(ptr, page_node, list);
	
	// printk(KERN_WARNING "PAGE OFFSET: %d", orig_f_pos);

	// Write the data
	while (size_written < count) {
		// Ensure the list has been initialised
		if (curr->page == NULL) {
			printk(KERN_ERR "No page to be written to in %s\n", MYDEV_NAME);
			return -1;
		}
		// Find the offset to begin writing
		begin_offset = *f_pos % PAGE_SIZE;
		// Make sure size we are writing fits in the page
		size_to_be_written = count - size_written;
		if ((PAGE_SIZE - begin_offset) < size_to_be_written) {
			size_to_be_written = PAGE_SIZE - begin_offset;
		}
		
		// Now that all checks have been made, write the data
		printk(KERN_INFO "%s: Writing %d bytes to page %d at offset %d\n", MYDEV_NAME, size_to_be_written, curr_page_no, begin_offset);

		// Perform the actual write

		// printk(KERN_WARNING "filp->f_pos: %d\n", filp->f_pos);
		//printk(KERN_WARNING "Begin offset: %u\n", *f_pos);
		size_not_written = copy_from_user(page_address(curr->page) + begin_offset, buf + size_written, size_to_be_written);
		
		// Record info in varaibles for checks	
		curr_size_written = size_to_be_written - size_not_written;
		*f_pos += curr_size_written;
		size_written += curr_size_written;

		if (size_not_written) break;
		
		// Keep writing
		ptr = ptr->next;
		curr = list_entry(ptr, page_node, list);
		curr_page_no++;
	}
	
	filp->f_pos = *f_pos;
	

  asgn1_device.data_size = max(asgn1_device.data_size,
                               orig_f_pos + size_written);
  return size_written;
}


#define SET_NPROC_OP 1
#define TEM_SET_NPROC _IOW(MYIOC_TYPE, SET_NPROC_OP, int) 

#define GET_MAJ_NUMBER_OP 2
#define TEM_GET_MAJ_NUMBER _IOR(MYIOC_TYPE, GET_MAJ_NUMBER_OP, int)

#define GET_PROC_NUMBER_OP 3
#define TEM_GET_PROC_NUMBER _IOR(MYIOC_TYPE, GET_PROC_NUMBER_OP, int)
/**
 * The ioctl function - various cases.
 */
long asgn1_ioctl (struct file *filp, unsigned cmd, unsigned long arg) {
  int nr;
  int new_nprocs;
  int result;

  /* COMPLETE ME */
  /** 
   * check whether cmd is for our device, if not for us, return -EINVAL 
   *
   * get command, and if command is SET_NPROC_OP, then get the data, and
     set max_nprocs accordingly, don't forget to check validity of the 
     value before setting max_nprocs
   */
	// Ensure the device cmd is correct
	printk(KERN_WARNING "ioctl function reached for /dev/asgn1/\n");
	if (_IOC_TYPE(cmd) != MYIOC_TYPE) {
		printk(KERN_WARNING "IOCTL CMD Not for this device");
		return -EINVAL;
	}
	// Parse the cmd
	nr = _IOC_NR(cmd);	

	switch (nr) {
		// Sets the maximum number of processes
		case SET_NPROC_OP:
			printk(KERN_WARNING "Attempting to set nprocs\n");
			result = copy_from_user((int *) &new_nprocs, (int *) arg, sizeof (arg));
			if (result != 0) {
				printk(KERN_WARNING "Copy to user failed");
				return -EINVAL;
			}
			if (new_nprocs > 1 && new_nprocs > atomic_read(&asgn1_device.nprocs)) {
				atomic_set(&asgn1_device.max_nprocs, new_nprocs);	
				return 0;
			}
			break;	
		// Returns the device major number
		case GET_MAJ_NUMBER_OP:
			printk(KERN_WARNING "Reading maj number\n");
			return MAJOR(asgn1_device.dev);
			break;
		// Get the number of processes currently accessing the device
		case GET_PROC_NUMBER_OP:
			printk(KERN_WARNING "Reading proc numbers\n");
			return atomic_read(&asgn1_device.nprocs);
			break;
		
		default:
			printk(KERN_WARNING "Command not found");
			return -EINVAL;
			break;
	}

  return -ENOTTY;
}


/**
 * Displays information about current status of the module,
 * which helps debugging.
 */
int asgn1_read_procmem(char *buf, char **start, off_t offset, int count,
		     int *eof, void *data) {
  /* stub */
  int result;

  /* COMPLETE ME */
  /**
   * use snprintf to print some info to buf, up to size count
   * set eof
   */

	// If not enough buffer space - should add check and remove 40 if time
	if (count < 40) {
		return -EINVAL;
	}
	// Print the information
	result = sprintf(buf, "Number pages: %u Ramdisk Size %lu\n", asgn1_device.num_pages, (PAGE_SIZE * asgn1_device.num_pages));
	
	*eof = 1;

  return result;
}

/**
 * Maps the memory to physical address - increasing efficiency.
 */
static int asgn1_mmap (struct file *filp, struct vm_area_struct *vma)
{
    // unsigned long pfn;
    unsigned long offset = vma->vm_pgoff << PAGE_SHIFT;
    unsigned long len = vma->vm_end - vma->vm_start;
    unsigned long ramdisk_size = asgn1_device.num_pages * PAGE_SIZE;
    page_node *curr;
    unsigned long index = 0;

    /* COMPLETE ME */
    /**
     * check offset and len
     *
     * loop through the entire page list, once the first requested page
     *   reached, add each page with remap_pfn_range one by one
     *   up to the last requested page
     */
	printk(KERN_WARNING "Entered the mmap function");
	// Check the arguments are OK	
	if (offset + len > ramdisk_size) return -1;
	
	list_for_each_entry(curr, &asgn1_device.mem_list, list) {
		
		if (index >= offset) {
			// Check to see if the page exists
			if (curr->page == NULL) {
				printk(KERN_WARNING "No page to mmap");
				return -1;
			}

			// Mmap the page
			if (remap_pfn_range (vma, vma->vm_start + PAGE_SIZE * (index - vma->vm_pgoff), page_to_pfn(curr->page), PAGE_SIZE, vma->vm_page_prot)) {
				return -EAGAIN;
			}	 
		}
		index++;
	}
	
    return 0;
}


// Operations the file can perform
struct file_operations asgn1_fops = {
  .owner = THIS_MODULE,
  .read = asgn1_read,
  .write = asgn1_write,
  .unlocked_ioctl = asgn1_ioctl,
  .open = asgn1_open,
  .mmap = asgn1_mmap,
  .release = asgn1_release,
  .llseek = asgn1_lseek
};

/**
 * Initialise the module and create the master device
 */
int __init asgn1_init_module(void){
  int result; 
	int majCheck;

  /* COMPLETE ME */
  /*
   * set nprocs and max_nprocs of the device
   *
   * allocate major number
   * allocate cdev, and set ops and owner field 
   * add cdev
   * initialize the page list
   * create proc entries
   */
	// Accessing variables should be atomic to avoid data race
	atomic_set(&asgn1_device.nprocs, 0);
	atomic_set(&asgn1_device.max_nprocs, 10);

	// Allocate a major device number from kernel	
	majCheck = alloc_chrdev_region(&asgn1_device.dev, asgn1_minor, asgn1_dev_count, MYDEV_NAME);
	if (majCheck < 0) {
		// Error with assigning major number
		printk(KERN_ERR "Error allocating a major number");
		goto fail_device;
	}
	// Allocate, Initialise, and add the char device
	asgn1_device.cdev = cdev_alloc();	
	cdev_init(asgn1_device.cdev, &asgn1_fops);
	cdev_add(asgn1_device.cdev, asgn1_device.dev, asgn1_dev_count);
	// Initialise the list for page nodes
	INIT_LIST_HEAD(&asgn1_device.mem_list);
	// Create proc entry, assign the info method to it
	my_proc = create_proc_entry("asgn1", S_IRUGO | S_IWUSR, NULL);
	if (!my_proc) {
		printk(KERN_ERR "Error creating proc entry");
		goto fail_device;
	}
	my_proc->read_proc = asgn1_read_procmem;	
	

  asgn1_device.class = class_create(THIS_MODULE, MYDEV_NAME);
  if (IS_ERR(asgn1_device.class)) {
  }

  asgn1_device.device = device_create(asgn1_device.class, NULL, 
                                      asgn1_device.dev, "%s", MYDEV_NAME);
  if (IS_ERR(asgn1_device.device)) {
    printk(KERN_WARNING "%s: can't create udev device\n", MYDEV_NAME);
    result = -ENOMEM;
    goto fail_device;
  }
  
  printk(KERN_WARNING "set up udev entry\n");
  printk(KERN_WARNING "Hello world from %s\n", MYDEV_NAME);
  return 0;

  /* cleanup code called when any of the initialization steps fail */
fail_device:
   class_destroy(asgn1_device.class);

  /* COMPLETE ME */
  /* PLEASE PUT YOUR CLEANUP CODE HERE, IN REVERSE ORDER OF ALLOCATION */
	
	// Free everything in reverse order
	// Remove proc
	 if(my_proc) remove_proc_entry("asgn1", NULL);		
	// Remove list head
	list_del(&asgn1_device.mem_list);
	// Remove cdev
	cdev_del(asgn1_device.cdev);
	// Unregister device	
	unregister_chrdev_region(asgn1_device.dev, asgn1_dev_count); // This goes last	

  return result;
}


/**
 * Finalise the module
 */
void __exit asgn1_exit_module(void){
  device_destroy(asgn1_device.class, asgn1_device.dev);
  class_destroy(asgn1_device.class);
  printk(KERN_WARNING "cleaned up udev entry\n");
  
  /* COMPLETE ME */
  /**
   * free all pages in the page list 
   * cleanup in reverse order
   */
	// Clean everything up in reverse order
	free_memory_pages();
	list_del_init(&asgn1_device.mem_list);
	cdev_del(asgn1_device.cdev);
	unregister_chrdev_region(asgn1_device.dev, asgn1_dev_count); // Must go last	

  printk(KERN_WARNING "Good bye from %s\n", MYDEV_NAME);
}


module_init(asgn1_init_module);
module_exit(asgn1_exit_module);


