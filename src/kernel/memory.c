#include "memory.h"
#include "console.h"
#include "../lib/stdint.h"
#include "lib.h"

extern unsigned long __start_code, __end_code, __end_data, __end_brk;

static struct Page *free_pages_list = NULL;
static unsigned long total_free_pages = 0;
static unsigned long kernel_heap_start = 0;
static unsigned long kernel_heap_end = 0;

#define MALLOC_DEBUG 1
#ifdef MALLOC_DEBUG
#define malloc_debug(fmt, ...) printk_color(CYAN, BLACK, "malloc: " fmt, ##__VA_ARGS__)
#endif

struct malloc_header {
    size_t size;
    struct malloc_header *next;
    int is_free;
};

static struct malloc_header *malloc_head = NULL;
static unsigned long malloc_total_allocated = 0;
static unsigned long malloc_total_freed = 0;

void init_page(void) {
    printk_color(BLUE, BLACK, "=== Page Management Initialization Start ===\n");
    
    unsigned long i, j;
    struct Page *pages = NULL;
    unsigned long page_count = 0;
    
    for(i = 0; i < memory_management_struct.e820_length; i++) {
        if(memory_management_struct.e820[i].type != 1)
            continue;
        
        unsigned long start = PAGE_2M_ALIGN(memory_management_struct.e820[i].address);
        unsigned long end = ((memory_management_struct.e820[i].address + 
                             memory_management_struct.e820[i].length) >> PAGE_2M_SHIFT) << PAGE_2M_SHIFT;
        
        if(end <= start)
            continue;
        
        page_count += (end - start) >> PAGE_2M_SHIFT;
    }
    
    for(i = 0; i < memory_management_struct.e820_length; i++) {
        if(memory_management_struct.e820[i].type != 1)
            continue;
        
        unsigned long start = memory_management_struct.e820[i].address;
        unsigned long end = start + memory_management_struct.e820[i].length;
        
        if((end - start) >= (page_count * sizeof(struct Page))) {
            pages = (struct Page *)Phy_To_Virt(start);
            memset(pages, 0, page_count * sizeof(struct Page));
            memory_management_struct.pages_struct = pages;
            malloc_debug("Page structures allocated at 0x%016lx, size: %lu bytes\n",
                        (unsigned long)pages, page_count * sizeof(struct Page));
            break;
        }
    }
    
    if(!pages) {
        printk_color(RED, BLACK, "Failed to allocate memory for page structures!\n");
        return;
    }
    
    struct Page *current = NULL;
    unsigned long page_index = 0;
    
    for(i = 0; i < memory_management_struct.e820_length; i++) {
        if(memory_management_struct.e820[i].type != 1)
            continue;
        
        unsigned long start = PAGE_2M_ALIGN(memory_management_struct.e820[i].address);
        unsigned long end = ((memory_management_struct.e820[i].address + 
                             memory_management_struct.e820[i].length) >> PAGE_2M_SHIFT) << PAGE_2M_SHIFT;
        
        if(end <= start)
            continue;
        
        for(j = start; j < end; j += PAGE_2M_SIZE) {
            struct Page *page = &pages[page_index++];
            page->phy_addr = j;
            page->ref_count = 0;
            page->next = NULL;
            
            if(!free_pages_list) {
                free_pages_list = page;
                current = page;
            } else {
                current->next = page;
                current = page;
            }
            
            total_free_pages++;
        }
    }
    struct Page *first_page = free_pages_list;
    if(!first_page) {
        printk_color(RED, BLACK, "No free pages available for heap!\n");
        return;
    }

    kernel_heap_start = first_page->phy_addr;
    
    unsigned long heap_size = 128 * 1024 * 1024; // 128MB
    
    kernel_heap_end = 0;
    for(i = 0; i < memory_management_struct.e820_length; i++) {
        if(memory_management_struct.e820[i].type == 1) {
            unsigned long end = memory_management_struct.e820[i].address + 
                               memory_management_struct.e820[i].length;
            if(end > kernel_heap_end) {
                kernel_heap_end = end;
            }
        }
    }
    
    if(kernel_heap_end < heap_size) {
        printk_color(RED, BLACK, "Error: Not enough memory for kernel heap!\n");
        heap_size = 64 * 1024 * 1024; // 64MB
        if(kernel_heap_end < heap_size) {
            printk_color(RED, BLACK, "Fatal: Still not enough memory!\n");
            return;
        }
    }
    
    kernel_heap_start = PAGE_2M_ALIGN(kernel_heap_start);
    
    if(kernel_heap_start >= kernel_heap_end) {
        printk_color(RED, BLACK, "Error: Invalid kernel heap range!\n");
        return;
    }
    
    printk_color(GREEN, BLACK, "Total free 2M pages: %lu\n", total_free_pages);
    printk_color(GREEN, BLACK, "Kernel heap size: %lu MB\n", heap_size / (1024 * 1024));
    printk_color(GREEN, BLACK, "Kernel heap (phys): 0x%016lx - 0x%016lx\n", 
                kernel_heap_start, kernel_heap_end);
    printk_color(GREEN, BLACK, "Kernel heap (virt): 0x%016lx - 0x%016lx\n", 
                Phy_To_Virt(kernel_heap_start), Phy_To_Virt(kernel_heap_end));
    
    printk_color(BLUE, BLACK, "=== Page Management Initialization Complete ===\n");
}

struct Page *alloc_page(void) {
    if(!free_pages_list) {
        printk_color(RED, BLACK, "Out of memory! No free pages available.\n");
        return NULL;
    }
    
    struct Page *page = free_pages_list;
    free_pages_list = free_pages_list->next;
    page->next = NULL;
    page->ref_count = 1;
    total_free_pages--;
    
    if((unsigned long)page->phy_addr & (PAGE_2M_SIZE - 1)) {
        printk_color(YELLOW, BLACK, "Warning: Page not 2M aligned: 0x%lx\n", page->phy_addr);
    }
    
    memset(Phy_To_Virt(page->phy_addr), 0, PAGE_2M_SIZE);
    
    malloc_debug("Allocated 2M page at phys 0x%lx, virt 0x%lx\n",
                page->phy_addr, Phy_To_Virt(page->phy_addr));
    
    return page;
}

void free_page(struct Page *page) {
    if(!page) {
        malloc_debug("free_page: null pointer\n");
        return;
    }
    
    page->ref_count--;
    
    if(page->ref_count <= 0) {
        page->next = free_pages_list;
        free_pages_list = page;
        total_free_pages++;
        
        malloc_debug("Freed 2M page at phys 0x%lx\n", page->phy_addr);
    }
}

void *malloc(unsigned long size) {
    if(size == 0) {
        malloc_debug("zero size request\n");
        return NULL;
    }
    
    unsigned long max_allowed = PAGE_2M_SIZE - sizeof(struct malloc_header) - 128;
    if(size > max_allowed) {
        printk_color(RED, BLACK, "malloc: Request too large: %lu bytes (max: %lu)\n", 
                    size, max_allowed);
        return NULL;
    }
    
    size = (size + 7) & ~7;
    
    unsigned long total_size = size + sizeof(struct malloc_header);
    
    struct malloc_header *current, *prev = NULL;
    
    if(!malloc_head) {
        struct Page *page = alloc_page();
        if(!page) {
            printk_color(RED, BLACK, "malloc: Failed to allocate first page\n");
            return NULL;
        }
        
        malloc_head = (struct malloc_header *)Phy_To_Virt(page->phy_addr);
        malloc_head->size = PAGE_2M_SIZE - sizeof(struct malloc_header);
        malloc_head->next = NULL;
        malloc_head->is_free = 1;
        
        malloc_debug("Initialized heap with first page at %p\n", malloc_head);
    }
    
    current = malloc_head;
    while(current) {
        if(current->is_free && current->size >= total_size) {
            if(current->size > total_size + sizeof(struct malloc_header) + 8) {
                struct malloc_header *new_block = (struct malloc_header *)
                    ((char *)current + total_size);
                new_block->size = current->size - total_size;
                new_block->next = current->next;
                new_block->is_free = 1;
                
                current->size = size;
                current->next = new_block;
                
                malloc_debug("Split block: %lu bytes, remaining: %lu bytes\n",
                           size, new_block->size);
            }
            
            current->is_free = 0;
            malloc_total_allocated += size;
            
            void *result = (void *)((char *)current + sizeof(struct malloc_header));
            malloc_debug("Allocated %lu bytes at %p\n", size, result);
            return result;
        }
        
        prev = current;
        current = current->next;
    }
    
    struct Page *page = alloc_page();
    if(!page) {
        printk_color(RED, BLACK, "malloc: Out of pages!\n");
        return NULL;
    }
    
    struct malloc_header *new_block = (struct malloc_header *)Phy_To_Virt(page->phy_addr);
    new_block->size = PAGE_2M_SIZE - sizeof(struct malloc_header);
    new_block->next = NULL;
    new_block->is_free = 0;
    
    if(prev) {
        prev->next = new_block;
    }
    
    if(new_block->size > total_size + sizeof(struct malloc_header) + 8) {
        struct malloc_header *split_block = (struct malloc_header *)
            ((char *)new_block + total_size);
        split_block->size = new_block->size - total_size;
        split_block->next = NULL;
        split_block->is_free = 1;
        
        new_block->size = size;
        new_block->next = split_block;
        
        malloc_debug("Split new page: %lu bytes, remaining: %lu bytes\n",
                   size, split_block->size);
    }
    
    malloc_total_allocated += size;
    
    void *result = (void *)((char *)new_block + sizeof(struct malloc_header));
    malloc_debug("Allocated %lu bytes at %p (new page)\n", size, result);
    return result;
}

void free(void *ptr) {
    if(!ptr) {
        malloc_debug("null pointer\n");
        return;
    }
    
    unsigned long addr = (unsigned long)ptr;
    unsigned long heap_virt_start = (unsigned long)Phy_To_Virt(kernel_heap_start);
    unsigned long heap_virt_end = (unsigned long)Phy_To_Virt(kernel_heap_end);
    
    malloc_debug("Checking pointer %p\n", ptr);
    malloc_debug("Heap virt range: 0x%016lx - 0x%016lx\n", 
                heap_virt_start, heap_virt_end);
    malloc_debug("Pointer addr: 0x%016lx\n", addr);
    malloc_debug("Kernel heap phys: 0x%016lx - 0x%016lx\n",
                kernel_heap_start, kernel_heap_end);
    
    if(addr < heap_virt_start || addr >= heap_virt_end) {
        printk_color(RED, BLACK, "free: Invalid pointer %p\n", ptr);
        printk_color(RED, BLACK, "  Pointer: 0x%016lx\n", addr);
        printk_color(RED, BLACK, "  Heap virt: 0x%016lx - 0x%016lx\n",
                    heap_virt_start, heap_virt_end);
        printk_color(RED, BLACK, "  Diff to start: %ld\n", addr - heap_virt_start);
        return;
    }
    
    struct malloc_header *header = (struct malloc_header *)
        ((char *)ptr - sizeof(struct malloc_header));
    
    if(header->size > PAGE_2M_SIZE) {
        printk_color(RED, BLACK, "free: Invalid header at %p\n", header);
        return;
    }
    
    header->is_free = 1;
    malloc_total_freed += header->size;
    
    struct malloc_header *current = malloc_head;
    while(current && current->next) {
        if(current->is_free && current->next->is_free) {
            // 合并current和current->next
            unsigned long merged_size = current->size + sizeof(struct malloc_header) + current->next->size;
            malloc_debug("Merging blocks: %lu + %lu = %lu bytes\n",
                       current->size, current->next->size, merged_size);
            
            current->size = merged_size;
            current->next = current->next->next;
        } else {
            current = current->next;
        }
    }
    
    malloc_debug("Freed memory at %p, size: %lu bytes\n", ptr, header->size);
    malloc_debug("Stats: allocated=%lu, freed=%lu\n", 
                malloc_total_allocated, malloc_total_freed);
}

void init_memory_map(){
    unsigned long i;
    unsigned long TotalMem = 0;
    struct E820 *p = NULL;
    memory_management_struct.e820_length = 0;
    
    p = (struct E820 *)0xffff800000005000;
    
    for(i = 0;i < 32;i++){
        printk_color(ORANGE,BLACK,"Address:0x%016lx\tLength:0x%016lx\tType:0x%x\n",
                    p->address,p->length,p->type);
        if(p->type == 1){
            TotalMem +=  p->length;
        }
        
        memory_management_struct.e820[i].address = p->address;
        memory_management_struct.e820[i].length = p->length;
        memory_management_struct.e820[i].type = p->type;
        memory_management_struct.e820_length = i + 1;
        
        p++;
        if(p->type > 4 || p->length == 0){
            break;
        }   
    }
    
    printk_color(ORANGE,BLACK, "OS Can Used Total RAM: 0x%016lx = %lu MB\n", 
                TotalMem, TotalMem / (1024 * 1024));
    
    TotalMem = 0;
    
    for(i = 0;i < memory_management_struct.e820_length;i++)
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
    
    printk_color(YELLOW,BLACK,"OS Can Used Total 2M PAGEs: 0x%08lx = %lu\n",TotalMem,TotalMem);
}

void memory_test(void) {
    printk_color(GREEN, BLACK, "=== Memory Allocation Test Start ===\n");

    void *ptr1 = malloc(512);
    void *ptr2 = malloc(1024);
    void *ptr3 = malloc(2048);
    
    if(ptr1 && ptr2 && ptr3) {
        printk_color(CYAN, BLACK, "Test 1: Multiple allocations:\n");
        printk_color(CYAN, BLACK, "  malloc(512)  = %p\n", ptr1);
        printk_color(CYAN, BLACK, "  malloc(1024) = %p\n", ptr2);
        printk_color(CYAN, BLACK, "  malloc(2048) = %p\n", ptr3);
        
        memset(ptr1, 0xBB, 512);
        memset(ptr2, 0xCC, 1024);
        memset(ptr3, 0xDD, 2048);
        printk_color(CYAN, BLACK, "  Memory filled with different patterns\n");
    } else {
        printk_color(RED, BLACK, "Test 1: Multiple allocations failed!\n");
    }
    
    void *ptr4 = malloc(1024 * 1024);  // 1MB
    if(ptr4) {
        printk_color(CYAN, BLACK, "Test 2: malloc(1MB) = %p\n", ptr4);
        memset(ptr4, 0x55, 1024 * 1024);
        printk_color(CYAN, BLACK, "  1MB memory filled\n");
    } else {
        printk_color(RED, BLACK, "Test 2: malloc(1MB) failed!\n");
    }
    
    if(ptr2) {
        char *test_ptr = (char *)ptr2;
        test_ptr[0] = 'T';
        test_ptr[1] = 'E';
        test_ptr[2] = 'S';
        test_ptr[3] = 'T';
        test_ptr[4] = '\0';
        printk_color(CYAN, BLACK, "Test 3: Write/read test: '%s'\n", (char *)ptr2);
    }
    
    if(ptr1) free(ptr1);
    if(ptr2) free(ptr2);
    if(ptr3) free(ptr3);
    if(ptr4) free(ptr4);
    
    printk_color(CYAN, BLACK, "  All memory freed\n");
}

void init_memory() {
    printk_color(BLUE, BLACK, "=== Memory Initialization Start ===\n");

    memory_management_struct.start_code = (unsigned long)&__start_code;
    memory_management_struct.end_code   = (unsigned long)&__end_code;
    memory_management_struct.end_data   = (unsigned long)&__end_data;
    memory_management_struct.end_brk    = (unsigned long)&__end_brk;

    init_memory_map();
    
    printk_color(BLUE, BLACK, "=== Memory Initialization Complete ===\n");
    
    init_page();
    
    memory_test();
}