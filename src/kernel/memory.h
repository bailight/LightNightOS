#ifndef MEMORY_H
#define MEMORY_H

#include "../lib/stdint.h"

#define E820_BUF        0x5000
#define E820_COUNT_ADDR (E820_BUF - 4)
#define E820_MAX_ENTRIES 32

#define PTRS_PER_PAGE	512

#define PAGE_OFFSET	((unsigned long)0xffff800000000000)

#define PAGE_GDT_SHIFT	39
#define PAGE_1G_SHIFT	30
#define PAGE_2M_SHIFT	21
#define PAGE_4K_SHIFT	12

#define PAGE_2M_SIZE	(1UL << PAGE_2M_SHIFT)
#define PAGE_4K_SIZE	(1UL << PAGE_4K_SHIFT)

#define PAGE_2M_MASK	(~ (PAGE_2M_SIZE - 1))
#define PAGE_4K_MASK	(~ (PAGE_4K_SIZE - 1))

#define PAGE_2M_ALIGN(addr)	(((unsigned long)(addr) + PAGE_2M_SIZE - 1) & PAGE_2M_MASK)
#define PAGE_4K_ALIGN(addr)	(((unsigned long)(addr) + PAGE_4K_SIZE - 1) & PAGE_4K_MASK)

#define Virt_To_Phy(addr)	((unsigned long)(addr) - PAGE_OFFSET)
#define Phy_To_Virt(addr)	((unsigned long *)((unsigned long)(addr) + PAGE_OFFSET))

#define Virt_To_2M_Page(kaddr)	(memory_management_struct.pages_struct + (Virt_To_Phy(kaddr) >> PAGE_2M_SHIFT))
#define Phy_to_2M_Page(kaddr)	(memory_management_struct.pages_struct + ((unsigned long)(kaddr) >> PAGE_2M_SHIFT))

#define PAGE_XD		(unsigned long)0x1000000000000000
#define	PAGE_PAT	(unsigned long)0x1000
#define	PAGE_Global	(unsigned long)0x0100
#define	PAGE_PS		(unsigned long)0x0080
#define	PAGE_Dirty	(unsigned long)0x0040
#define	PAGE_Accessed	(unsigned long)0x0020
#define PAGE_PCD	(unsigned long)0x0010
#define PAGE_PWT	(unsigned long)0x0008
#define	PAGE_U_S	(unsigned long)0x0004
#define	PAGE_R_W	(unsigned long)0x0002
#define	PAGE_Present	(unsigned long)0x0001
#define PAGE_KERNEL_GDT		(PAGE_R_W | PAGE_Present)	
#define PAGE_KERNEL_Dir		(PAGE_R_W | PAGE_Present)
#define	PAGE_KERNEL_Page	(PAGE_PS  | PAGE_R_W | PAGE_Present)
#define PAGE_USER_Dir		(PAGE_U_S | PAGE_R_W | PAGE_Present)
#define	PAGE_USER_Page		(PAGE_PS  | PAGE_U_S | PAGE_R_W | PAGE_Present)

typedef struct {unsigned long pml4t;} pml4t_t;
#define	mk_pml4t(addr,attr)	((unsigned long)(addr) | (unsigned long)(attr))
#define set_pml4t(mpl4tptr,mpl4tval)	(*(mpl4tptr) = (mpl4tval))


typedef struct {unsigned long pdpt;} pdpt_t;
#define mk_pdpt(addr,attr)	((unsigned long)(addr) | (unsigned long)(attr))
#define set_pdpt(pdptptr,pdptval)	(*(pdptptr) = (pdptval))


typedef struct {unsigned long pdt;} pdt_t;
#define mk_pdt(addr,attr)	((unsigned long)(addr) | (unsigned long)(attr))
#define set_pdt(pdtptr,pdtval)		(*(pdtptr) = (pdtval))


typedef struct {unsigned long pt;} pt_t;
#define mk_pt(addr,attr)	((unsigned long)(addr) | (unsigned long)(attr))
#define set_pt(ptptr,ptval)		(*(ptptr) = (ptval))

struct Page {
    unsigned long phy_addr;
    unsigned int ref_count;
    struct Page *next;
};

struct E820{
	unsigned long address;
	unsigned long length;
	unsigned int	type;
}__attribute__((packed));

extern struct Global_Memory_Descriptor memory_management_struct;

struct Global_Memory_Descriptor {
    volatile struct E820 e820[32];
    volatile unsigned long e820_length;

	volatile unsigned long 	start_code , end_code , end_data , end_brk;

	volatile struct Page *pages_struct;
};

void init_memory(void);
void init_memory_map(void);

struct Page * alloc_page(void);
void free_page(struct Page *page);
void *malloc(unsigned long size);
void free(void *ptr);

#endif