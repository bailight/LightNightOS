#include "memory.h"
#include "console.h"
#include "../lib/stdint.h"
#include "lib.h"

extern unsigned long __start_code, __end_code, __end_data, __end_brk;

static int test_bit(int idx);
static void set_bit(int idx, int val);

static inline unsigned long get_cr3() {
    unsigned long cr3;
    asm volatile("mov %%cr3, %0" : "=r"(cr3));
    return cr3;
}

static inline void set_cr3(unsigned long cr3) {
    asm volatile("mov %0, %%cr3" : : "r"(cr3));
}

static inline void flush_tlb(unsigned long addr) {
    asm volatile("invlpg (%0)" : : "r"(addr));
}

static void init_page_tables() {

    memset((void*)PGD_BASE, 0, 0x3000);

    unsigned long *pgd = (unsigned long*)PGD_BASE;
    unsigned long *pud = (unsigned long*)PUD_BASE;
    unsigned long *pmd = (unsigned long*)PMD_BASE;

    unsigned long virt_addr = PGD_BASE;
    unsigned long phy_addr  = 0;
    unsigned long end_addr  = (unsigned long)&__end_brk;

    while (virt_addr < end_addr) {
        int pgd_idx = (virt_addr >> 39) & 0x1FF;
        int pud_idx = (virt_addr >> 30) & 0x1FF;
        int pmd_idx = (virt_addr >> 21) & 0x1FF;

        if (!(pgd[pgd_idx] & PAGE_PRESENT)) {
            pgd[pgd_idx] = (unsigned long)pud | PAGE_FLAGS;
        }

        if (!(pud[pud_idx] & PAGE_PRESENT)) {
            pud[pud_idx] = (unsigned long)pmd | PAGE_FLAGS;
        }

        if (!(pmd[pmd_idx] & PAGE_PRESENT)) {
            pmd[pmd_idx] = phy_addr | PAGE_FLAGS;
        }

        virt_addr += PAGE_2M_SIZE;
        phy_addr  += PAGE_2M_SIZE;
    }

    set_cr3((unsigned long)pgd);
    printk_color(GREEN, BLACK, "Page tables init done, CR3 = %#lx\n", get_cr3());
}

// ========= Physict page 

static void init_page_bits_map() {
    unsigned long total_2m_pages = 0;
    unsigned long bits_map_size  = 0;

    for (int i = 0; i < memory_management_struct.e820_length; i++) {
        if (memory_management_struct.e820[i].type != 1) continue;

        unsigned long start = PAGE_2M_ALIGN(memory_management_struct.e820[i].address);
        unsigned long end   = (memory_management_struct.e820[i].address + memory_management_struct.e820[i].length) & PAGE_2M_MASK;
        if (end <= start) continue;

        total_2m_pages += (end - start) >> PAGE_2M_SHIFT;
    }

    bits_map_size = (total_2m_pages + 7) / 8;
    memory_management_struct.bits_length = total_2m_pages;

    unsigned long bits_phy = 0;
    for (int i = 0; i < memory_management_struct.e820_length; i++) {
        if (memory_management_struct.e820[i].type != 1) continue;

        unsigned long start = PAGE_2M_ALIGN(memory_management_struct.e820[i].address);
        unsigned long end   = (memory_management_struct.e820[i].address + memory_management_struct.e820[i].length) & PAGE_2M_MASK;
        if (start + bits_map_size <= end) {
            bits_phy = start;
            for (unsigned long addr = start; addr < start + bits_map_size; addr += PAGE_2M_SIZE) {
                int bit_idx = addr >> PAGE_2M_SHIFT;
                set_bit(bit_idx, PAGE_RESERVED);
            }
            break;
        }
    }

    memory_management_struct.bits_map = (unsigned long*)PHY_TO_VIRT(bits_phy);
    memory_management_struct.bits_size = bits_map_size;
    memset(memory_management_struct.bits_map, PAGE_FREE, bits_map_size);

    unsigned long kernel_start = PAGE_2M_ALIGN((unsigned long)&__start_code);
    unsigned long kernel_end   = PAGE_2M_ALIGN((unsigned long)&__end_brk);
    for (unsigned long addr = kernel_start; addr < kernel_end; addr += PAGE_2M_SIZE) {

        unsigned long phy_addr = addr - 0xFFFF800000000000;
        int bit_idx = phy_addr >> PAGE_2M_SHIFT;
        set_bit(bit_idx, PAGE_RESERVED);
    }

    for (int i = 0; i < memory_management_struct.e820_length; i++) {
        if (memory_management_struct.e820[i].type != 1) {
            unsigned long start = PAGE_2M_ALIGN(memory_management_struct.e820[i].address);
            unsigned long end   = (memory_management_struct.e820[i].address + memory_management_struct.e820[i].length) & PAGE_2M_MASK;
            for (unsigned long addr = start; addr < end; addr += PAGE_2M_SIZE) {
                int bit_idx = addr >> PAGE_2M_SHIFT;
                set_bit(bit_idx, PAGE_RESERVED);
            }
        }
    }

    bits_map_inited = 1;
    printk_color(GREEN, BLACK, "Page bits map init: %lu pages, %lu bytes\n", total_2m_pages, bits_map_size);
}

static int test_bit(int idx) {
    if (!bits_map_inited || idx >= memory_management_struct.bits_length) return PAGE_RESERVED;
    int byte_idx = idx / 8;
    int bit_idx  = idx % 8;
    return (memory_management_struct.bits_map[byte_idx] >> bit_idx) & 1;
}

static void set_bit(int idx, int val) {
    if (!bits_map_inited || idx >= memory_management_struct.bits_length) return;
    LOCK();
    int byte_idx = idx / 8;
    int bit_idx  = idx % 8;
    if (val) {
        memory_management_struct.bits_map[byte_idx] |= (1 << bit_idx);
    } else {
        memory_management_struct.bits_map[byte_idx] &= ~(1 << bit_idx);
    }
    UNLOCK();
}

struct Page *alloc_page(void) {
    if (!bits_map_inited) {
        printk_color(RED, BLACK, "Bits map not initialized!\n");
        return NULL;
    }

    LOCK();

    for (int i = 0; i < memory_management_struct.bits_length; i++) {
        if (test_bit(i) == PAGE_FREE) {
            set_bit(i, PAGE_USED);
            
            static struct Page page_buf;
            page_buf.PHY_address = (unsigned long)i << PAGE_2M_SHIFT;
            page_buf.attribute   = PAGE_USED;
            page_buf.reference_count = 1;

            UNLOCK();
            printk_color(GREEN, BLACK, "Alloc page: PHY %#lx\n", page_buf.PHY_address);
            return &page_buf;
        }
    }
    UNLOCK();

    printk_color(RED, BLACK, "No free 2M page available!\n");
    return NULL;
}

void free_page(struct Page *page) {
    if (!page || !bits_map_inited) {
        printk_color(RED, BLACK, "Invalid page to free!\n");
        return;
    }

    LOCK();

    int bit_idx = page->PHY_address >> PAGE_2M_SHIFT;
    if (bit_idx >= memory_management_struct.bits_length) {
        UNLOCK();
        return;
    }

    if (page->reference_count > 0) {
        page->reference_count--;
        if (page->reference_count > 0) {
            UNLOCK();
            return;
        }
    }

    set_bit(bit_idx, PAGE_FREE);
    page->attribute = PAGE_FREE;
    UNLOCK();

    printk_color(GREEN, BLACK, "Free page: PHY %#lx\n", page->PHY_address);
}

int map_page(unsigned long virt_addr, unsigned long phy_addr) {
    if (virt_addr & (PAGE_2M_SIZE - 1) || phy_addr & (PAGE_2M_SIZE - 1)) {
        printk_color(RED, BLACK, "Address not 2M aligned!\n");
        return -1;
    }

    unsigned long *pgd = (unsigned long*)PGD_BASE;
    unsigned long *pud = (unsigned long*)PUD_BASE;
    unsigned long *pmd = (unsigned long*)PMD_BASE;

    int pgd_idx = (virt_addr >> 39) & 0x1FF;
    int pud_idx = (virt_addr >> 30) & 0x1FF;
    int pmd_idx = (virt_addr >> 21) & 0x1FF;

    if (!(pgd[pgd_idx] & PAGE_PRESENT)) {
        pgd[pgd_idx] = (unsigned long)pud | PAGE_FLAGS;
    }
    if (!(pud[pud_idx] & PAGE_PRESENT)) {
        pud[pud_idx] = (unsigned long)pmd | PAGE_FLAGS;
    }
    pmd[pmd_idx] = phy_addr | PAGE_FLAGS;

    flush_tlb(virt_addr);
    printk_color(GREEN, BLACK, "Map: VIRT %#lx -> PHY %#lx\n", virt_addr, phy_addr);
    return 0;
}

void unmap_page(unsigned long virt_addr) {
    if (virt_addr & (PAGE_2M_SIZE - 1)) {
        printk_color(RED, BLACK, "Address not 2M aligned!\n");
        return;
    }

    unsigned long *pgd = (unsigned long*)PGD_BASE;
    unsigned long *pud = (unsigned long*)PUD_BASE;
    unsigned long *pmd = (unsigned long*)PMD_BASE;

    int pgd_idx = (virt_addr >> 39) & 0x1FF;
    int pud_idx = (virt_addr >> 30) & 0x1FF;
    int pmd_idx = (virt_addr >> 21) & 0x1FF;

    if (pgd[pgd_idx] & PAGE_PRESENT && pud[pud_idx] & PAGE_PRESENT) {
        pmd[pmd_idx] = 0;
        flush_tlb(virt_addr);
        printk_color(GREEN, BLACK, "Unmap: VIRT %#lx\n", virt_addr);
    }
}

// ===== malloc/free

typedef struct mem_block {
    unsigned long size;
    struct mem_block *next;
    int is_free;
} mem_block_t;

static mem_block_t *heap_start = NULL;

static void init_heap() {
    if (heap_start) return;

    struct Page *page = alloc_page();
    if (!page) {
        printk_color(RED, BLACK, "Heap init failed!\n");
        return;
    }

    heap_start = (mem_block_t*)PHY_TO_VIRT(page->PHY_address);
    heap_start->size = PAGE_2M_SIZE - sizeof(mem_block_t);
    heap_start->next = NULL;
    heap_start->is_free = 1;

    printk_color(GREEN, BLACK, "Heap init: start %#lx, size %lu\n", (unsigned long)heap_start, heap_start->size);
}

void *malloc(unsigned long size) {
    if (size == 0) return NULL;

    size = (size + 7) & ~7;

    if (!heap_start) init_heap();
    if (!heap_start) return NULL;

    LOCK();
    mem_block_t *curr = heap_start;
    while (curr) {
        if (curr->is_free && curr->size >= size) {
            if (curr->size - size > sizeof(mem_block_t) + 8) {
                mem_block_t *new_block = (mem_block_t*)((unsigned long)curr + sizeof(mem_block_t) + size);
                new_block->size = curr->size - size - sizeof(mem_block_t);
                new_block->next = curr->next;
                new_block->is_free = 1;

                curr->size = size;
                curr->next = new_block;
            }
            curr->is_free = 0;
            UNLOCK();
            return (void*)((unsigned long)curr + sizeof(mem_block_t));
        }
        curr = curr->next;
    }
    UNLOCK();

    struct Page *new_page = alloc_page();
    if (!new_page) return NULL;

    mem_block_t *new_block = (mem_block_t*)PHY_TO_VIRT(new_page->PHY_address);
    new_block->size = PAGE_2M_SIZE - sizeof(mem_block_t);
    new_block->next = NULL;
    new_block->is_free = 1;

    LOCK();
    curr = heap_start;
    while (curr->next) curr = curr->next;
    curr->next = new_block;
    UNLOCK();

    return malloc(size);
}

void free(void *ptr) {
    if (!ptr || !heap_start) return;

    LOCK();
    mem_block_t *block = (mem_block_t*)((unsigned long)ptr - sizeof(mem_block_t));

    if ((unsigned long)block < (unsigned long)heap_start || block->is_free) {
        UNLOCK();
        printk_color(RED, BLACK, "Invalid free pointer!\n");
        return;
    }

    block->is_free = 1;

    if (block->next && block->next->is_free) {
        block->size += sizeof(mem_block_t) + block->next->size;
        block->next = block->next->next;
    }

    mem_block_t *prev = heap_start;
    while (prev && prev->next != block) prev = prev->next;
    if (prev && prev->is_free) {
        prev->size += sizeof(mem_block_t) + block->size;
        prev->next = block->next;
    }

    UNLOCK();
    printk_color(GREEN, BLACK, "Free memory: %#lx\n", (unsigned long)ptr);
}

void init_memory() {
	printk_color(BLUE, BLACK, "=== Memory Initialization Start ===\n");
	init_memory_map();

	memory_management_struct.start_code = (unsigned long)&__start_code;
    memory_management_struct.end_code   = (unsigned long)&__end_code;
    memory_management_struct.end_data   = (unsigned long)&__end_data;
    memory_management_struct.end_brk    = (unsigned long)&__end_brk;

    init_page_bits_map();

    // init_page_tables();

	printk_color(BLUE, BLACK, "=== Memory Initialization Complete ===\n");
}

void init_memory_map(){
	unsigned int i;
	unsigned long TotalMem = 0;
	struct E820 *p = NULL;
	memory_management_struct.e820_length = 0;
	
	p = (struct E820 *)0xffff800000005000;

	for(i = 0;i < 32;i++){
		printk_color(ORANGE,BLACK,"Address:%#018lx\tLength:%#018lx\tType:%#010x\n",p->address,p->length,p->type);
		if(p->type == 1){
			TotalMem +=  p->length;
		}

		memory_management_struct.e820[i].address = p->address;
		memory_management_struct.e820[i].length = p->length;
		memory_management_struct.e820[i].type = p->type;
		memory_management_struct.e820_length = i + 1;

		p++;
		if(p->type > 4 || p->length == 0 || p->type < 1){
			break;
		}	
	}

	printk_color(ORANGE,BLACK, "OS Can Used Total RAM:%#018lx=%018d\n", TotalMem, TotalMem);

	TotalMem = 0;

	for(i = 0;i <= memory_management_struct.e820_length;i++)
	{
		unsigned long start,end;
		if(memory_management_struct.e820[i].type != 1)
			continue;
		start = PAGE_2M_ALIGN(memory_management_struct.e820[i].address);
		end   = ((memory_management_struct.e820[i].address + memory_management_struct.e820[i].length) >> PAGE_2M_SHIFT) << PAGE_2M_SHIFT;
		if(end <= start)
			continue;
		TotalMem += (end - start) >> PAGE_2M_SHIFT;
	}
	
	printk_color(YELLOW,BLACK,"OS Can Used Total 2M PAGEs:%#010x=%010d\n",TotalMem,TotalMem);

}