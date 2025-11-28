#ifndef MEMORY_H
#define MEMORY_H

#include "../lib/stdint.h"

#define E820_BUF        0x5000
#define E820_COUNT_ADDR (E820_BUF - 4)
#define E820_MAX_ENTRIES 32

#define PAGE_2M_SHIFT    21
#define PAGE_2M_SIZE     (1UL << PAGE_2M_SHIFT)
#define PAGE_2M_MASK     (~(PAGE_2M_SIZE - 1))
#define PAGE_2M_ALIGN(addr)  ((addr + PAGE_2M_SIZE - 1) & PAGE_2M_MASK)

#define PHY_TO_VIRT(phy) (0xFFFF800000000000UL | (phy))

#define PAGE_FREE        0x00
#define PAGE_USED        0x01
#define PAGE_RESERVED    0x02

#define ZONE_NORMAL      0x01

#define PGD_BASE 0xFFFF800000100000
#define PUD_BASE 0xFFFF800000101000
#define PMD_BASE 0xFFFF800000102000

#define PAGE_PRESENT  (1 << 0)
#define PAGE_WRITE    (1 << 1)
#define PAGE_USER     (1 << 2)
#define PAGE_PSE      (1 << 7)
#define PAGE_FLAGS    (PAGE_PRESENT | PAGE_WRITE | PAGE_PSE)

static int bits_map_inited = 0;

static volatile int page_lock = 0;

#define LOCK()   while(__sync_lock_test_and_set(&page_lock, 1)) {}
#define UNLOCK() __sync_lock_release(&page_lock, 0);

struct E820{
	unsigned long address;
	unsigned long length;
	unsigned int	type;
}__attribute__((packed));

extern struct Global_Memory_Descriptor memory_management_struct;

struct Global_Memory_Descriptor {
    volatile struct E820 e820[32];
    volatile unsigned long e820_length;

	unsigned long * bits_map;
	volatile unsigned long 	bits_size;
	volatile unsigned long   bits_length;

	struct Page *	pages_struct;
	volatile unsigned long	pages_size;
	volatile unsigned long 	pages_length;

	struct Zone * 	zones_struct;
	volatile unsigned long	zones_size;
	volatile unsigned long 	zones_length;

	volatile unsigned long 	start_code , end_code , end_data , end_brk;

	unsigned long	end_of_struct;
};

struct Page
{
	struct Zone *	zone_struct;
	volatile unsigned long	PHY_address;
	volatile unsigned long	attribute;

	volatile unsigned long	reference_count;
	
	volatile unsigned long	age;
};

struct Zone
{
	struct Page * 	pages_group;
	volatile unsigned long	pages_length;
	
	volatile unsigned long	zone_start_address;
	volatile unsigned long	zone_end_address;
	volatile unsigned long	zone_length;
	volatile unsigned long	attribute;

	struct Global_Memory_Descriptor * GMD_struct;

	volatile unsigned long	page_using_count;
	volatile unsigned long	page_free_count;

	volatile unsigned long	total_pages_link;
};

void init_memory(void);
void init_memory_map(void);

struct Page * alloc_page(void);
void free_page(struct Page *page);
int map_page(unsigned long virt_addr, unsigned long phy_addr);
void unmap_page(unsigned long virt_addr);
void *malloc(unsigned long size);
void free(void *ptr);

#endif