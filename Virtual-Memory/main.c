/*
Main program for the virtual memory project.
Make all of your modifications to this file.
You may add or rearrange any code or data as you need.
The header files page_table.h and disk.h explain
how to use the page table and disk interfaces.

Group Members:
Dylan Breslaw
John Carr
Carter Goldman
Nicholas Newton
*/

#include "page_table.h"
#include "disk.h"
#include "program.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

// Globals
char *policy;
int *pages;
struct disk *disk;
char *physmem;
int oldest = 0;
int alternating = 0;
int page_faults = 0;
int disk_reads = 0;
int disk_writes = 0;

// Replace the page in replace_frame with the given page in the page table
void replace(struct page_table *pt, int page, int replace_frame)
{
	int npages = page_table_get_npages(pt);
	int old_page = pages[replace_frame];

	disk_write(disk, old_page, &physmem[replace_frame*npages]); 
	disk_writes++;
	
	disk_read(disk, page, &physmem[replace_frame*npages]);
	disk_reads++;

	page_table_set_entry(pt,page,replace_frame, PROT_READ);
	pages[replace_frame] = page;
	page_table_set_entry(pt, old_page, 0, 0);
}

// Replace using a random number
void rand_policy_handler(struct page_table *pt, int page){
	int replace_frame = rand() % (page_table_get_nframes(pt));
	replace(pt, page, replace_frame);
}

// Replace using FIFO
void fifo_policy_handler(struct page_table *pt, int page)
{
	replace(pt, page, oldest);

	// Increment oldest
	oldest++;
	if (oldest >= page_table_get_nframes(pt))
	{
		oldest = 0;
	}
}

// Replace using our custom algorithm
void custom_policy_handler(struct page_table *pt, int page, int nframes)
{	
	// LIFO Replacement, with a buffer
	// Replace either (n)th or (n-1)th page
	replace(pt, page, nframes-alternating-1);

	// Alternate between 1 and 0
	alternating = 1 - alternating;
}

// Handle any page fault
void page_fault_handler( struct page_table *pt, int page )
{
	page_faults++;

	int npages= page_table_get_npages(pt);
	int nframes= page_table_get_nframes(pt);

	//check if pages exceeds frames
	if (npages <= nframes) {
		page_table_set_entry(pt,page,page,PROT_READ|PROT_WRITE);
		return;
	}
	//else bring page into memory do replacement policy if
	else{
		int frame, bits, i;
		page_table_get_entry(pt, page, &frame, &bits);

		// Check if already in frame table
		for (i = 0; i < nframes; i++) {
			if (pages[i] == page) {
				if (bits == 1) {
					page_table_set_entry(pt, page, frame, PROT_READ|PROT_WRITE);
				}
				else if (bits == 3) {
					page_table_set_entry(pt, page, frame, PROT_READ|PROT_WRITE|PROT_EXEC);
				}
				return;
			}
		}

		//check if pt is full, if found frame, send page here
		for(i=0; i<nframes;i++){
			if(pages[i]==-1){
				page_table_set_entry(pt, page, i, PROT_READ);
				pages[i]=page; // no longer free
				disk_read(disk, page, &physmem[i*nframes]);
				disk_reads++;
				return;
			}
		}
		
		

		//if not found, handle page replacement based on passed policy
		if(!strcmp(policy, "rand")) {
			rand_policy_handler(pt, page);
		}
		else if(!strcmp(policy, "fifo")) {
			fifo_policy_handler(pt, page);
		}
		else if(!strcmp(policy, "custom")) {
			custom_policy_handler(pt, page, nframes);
		}
		else {
			printf("unknown algorithm\n");
			printf("use: virtmem <npages> <nframes> <rand|fifo|custom> <alpha|beta|gamma|delta>\n");
			exit(1);
		}
	}
}

int main( int argc, char *argv[] )
{
	if(argc!=5) {
		printf("use: virtmem <npages> <nframes> <rand|fifo|custom> <alpha|beta|gamma|delta>\n");
		return 1;
	}

	int npages = atoi(argv[1]);
	int nframes = atoi(argv[2]);
	if (npages <= 0 || nframes <= 0)
	{
		printf("Error: npages and nframes must be positive integers\n");
		return 1;
	}

	policy = argv[3];
	const char *program = argv[4];
	pages=malloc(npages*sizeof(int));
	int i;
	// Initialize table to -1 for "empty"
	for(i=0; i<npages; i++){
		pages[i]=-1;
	}

	disk = disk_open("myvirtualdisk",npages);
	if(!disk) {
		fprintf(stderr,"couldn't create virtual disk: %s\n",strerror(errno));
		return 1;
	}

	struct page_table *pt = page_table_create( npages, nframes, page_fault_handler );
	if(!pt) {
		fprintf(stderr,"couldn't create page table: %s\n",strerror(errno));
		return 1;
	}

	char *virtmem = page_table_get_virtmem(pt);

	physmem = page_table_get_physmem(pt);

	if(!strcmp(program,"alpha")) {
		alpha_program(virtmem,npages*PAGE_SIZE);

	} else if(!strcmp(program,"beta")) {
		beta_program(virtmem,npages*PAGE_SIZE);

	} else if(!strcmp(program,"gamma")) {
		gamma_program(virtmem,npages*PAGE_SIZE);

	} else if(!strcmp(program,"delta")) {
		delta_program(virtmem,npages*PAGE_SIZE);

	} else {
		fprintf(stderr,"unknown program: %s\n",argv[4]);
		return 1;
	}

	// Output statistics
	printf("Page Faults: %d\n", page_faults);
	printf("Disk Reads: %d\n", disk_reads);
	printf("Disk Writes: %d\n", disk_writes);

	page_table_delete(pt);
	disk_close(disk);
	free(pages);

	return 0;
}
