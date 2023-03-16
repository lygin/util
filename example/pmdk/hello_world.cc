#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <libpmemobj.h>

// Name of our layout in the pool(not file name)
#define LAYOUT "hello_layout"

// Maximum length of our buffer
#define MAX_BUF_LEN 30


// Root structure
struct my_root {
	size_t len;
	char buf[MAX_BUF_LEN];
};

/****************************
 * This function writes the "Hello..." string to persistent-memory.
 *****************************/
void write_hello_string (char *buf, char *path)
{
	PMEMobjpool *pop;
	
	// 1.Create the pmemobj pool (or open it if it already exists)
	pop = pmemobj_create(path, LAYOUT, PMEMOBJ_MIN_POOL, 0666);

	// Check if create failed		
	if (pop == NULL) 
	{
		perror(path);
		exit(1);
	}
					
	// 2.Get the PMEMObj root(mem struct->oid)
	PMEMoid root = pmemobj_root(pop, sizeof (struct my_root));

	// 3.get mem Pointer for structure at the root
	struct my_root *rootp = (struct my_root *)pmemobj_direct(root);
	if (rootp == NULL)
		exit(1);

	// Write the string to persistent memory

	// Copy string and persist it
	pmemobj_memcpy_persist(pop, rootp->buf, buf, strlen(buf));

	// Assign the string length, not persist now
	rootp->len = strlen(buf);
	// persist now (mem_addr, size)
	pmemobj_persist(pop, &rootp->len, sizeof (rootp->len));	

	// Write the string from persistent memory 	to console
	printf("\nWrite the (%s) string to persistent-memory.\n", rootp->buf);
	
	// Close PMEM object pool
	pmemobj_close(pop);	
		
	return;
}

/****************************
 * This function reads the "Hello..." string from persistent-memory.
 *****************************/
void read_hello_string(char *path)
{
	PMEMobjpool *pop;
	
	//Attempt open instead
	pop = pmemobj_open(path, LAYOUT);
	
	// Check if open failed
	if (pop == NULL) {
		perror(path);
		exit(1);
	} 
	
	// Get the PMEMObj root
	PMEMoid root = pmemobj_root(pop, sizeof (struct my_root));
	
	// Pointer for structure at the root
	struct my_root *rootp = (struct my_root *)pmemobj_direct(root);
	
	// Read the string from persistent memory and write to the console
	printf("\nRead the (%s) string from persistent-memory.\n", rootp->buf);
	
	// Close PMEM object pool
	pmemobj_close(pop);

	return;
}

int main(int argc, char *argv[])
{
	char *path = "/mnt/pmem0/hello";
	
	// Create the string to save to persistent memory
	char buf[MAX_BUF_LEN] = "Hello Persistent Memory!!!";
	

	//write_hello_string(buf, path);
		
	read_hello_string(path);

	return 0;
}