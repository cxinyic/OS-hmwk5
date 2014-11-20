#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <errno.h>
#include <string.h>
#include <sys/fcntl.h>
#include <sys/mman.h>

#define PAGE_TABLE_SIZE (4*1024*1024) /* 4mb */
#define PGD_SIZE (2048*4)

static int pgnum2index(int num)
{
	return (((num / 512 * 4096) / 4) + (num % 512));
}

#define young_bit(pte)  ((pte & (1<<1))  >> 1)
#define file_bit(pte)   ((pte & (1<<2))  >> 2)
#define dirty_bit(pte)  ((pte & (1<<6))  >> 6)
#define rdonly_bit(pte) ((pte & (1<<7))  >> 7)
#define user_bit(pte)   ((pte & (1<<8))  >> 8)
#define phys(pte)   (pte >> 12)

static unsigned long * expose(int pid)
{
	int fd = open("/dev/zero", O_RDONLY);
	void *addr = mmap(NULL, PAGE_TABLE_SIZE * 2, PROT_READ,
				 MAP_SHARED, fd, 0);
	void *pgd_addr = mmap(NULL, PGD_SIZE * 2, PROT_READ,
				 MAP_SHARED, fd, 0);
	close(fd);
	if (addr == MAP_FAILED) {
		printf("Error: mmap");
		return NULL;
	}
	if (pgd_addr == MAP_FAILED) {
		printf("Error: mmap");
		return NULL;
	}
	if (syscall(378, pid, (unsigned long)pgd_addr,
			(unsigned long)addr) < 0) {
		printf("Error: expose_page_table syscall");
		return NULL;
	}
	return addr;
}

int main(int argc, char **argv)
{
	int i;
	unsigned long *page_table = NULL, *page = NULL;
	int pid;
	int verbose = 0;

	if(argc != 3 && argc != 2)
		return -1;

	if(argv[1][0] == '-' && argv[1][1] == 'v')
		verbose = 1;

	/* second argument is pid*/
	pid = atoi(argv[argc-1]);

	page_table = expose(pid);

	if (page_table == NULL) {
		return -1;
	}

	for (i = 0; i < PAGE_TABLE_SIZE / sizeof(int); i++) {
		page = &page_table[pgnum2index(i)];

		if (page == NULL)
			continue;
		if (*page == 0) {
			if (verbose)
				printf("0x400 0x10000000 0 0 0 0 0 0\n");
			continue;
		}
		printf("%d ", i);
		printf("%p ", page);
		printf("%p ", (void *)phys(*page));
		printf("%lu ", young_bit(*page));
		printf("%lu ", file_bit(*page));
		printf("%lu ", dirty_bit(*page));
		printf("%lu ", rdonly_bit(*page));
		printf("%lu ", user_bit(*page));
		printf("\n");
	}
	munmap(page_table, PAGE_TABLE_SIZE * 2);
        //free(page_table);
        //free(page);
        page_table = NULL;
        page = NULL;
        return 0;
}

