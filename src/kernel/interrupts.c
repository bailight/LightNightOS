#include "interrupts.h"

#define IDT_ENTRIES 256
#define KERNEL_CS   0x18

extern void lidt(void *);

// ----- IDT entry and pointer -----
struct idt_entry {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t  ist;
    uint8_t  type_attr;
    uint16_t offset_mid;
    uint32_t offset_high;
    uint32_t zero;
} __attribute__((packed));

struct idt_ptr {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

static struct idt_entry idt[IDT_ENTRIES];
static struct idt_ptr   idtr;

// ----- ISR stubs (asm) -----
extern void isr0(void);
extern void isr1(void);
extern void isr2(void);
extern void isr3(void);
extern void isr4(void);
extern void isr5(void);
extern void isr6(void);
extern void isr7(void);
extern void isr8(void);
extern void isr9(void);
extern void isr10(void);
extern void isr11(void);
extern void isr12(void);
extern void isr13(void);
extern void isr14(void);
extern void isr15(void);
extern void isr16(void);
extern void isr17(void);
extern void isr18(void);
extern void isr19(void);
extern void isr20(void);
extern void isr21(void);
extern void isr22(void);
extern void isr23(void);
extern void isr24(void);
extern void isr25(void);
extern void isr26(void);
extern void isr27(void);
extern void isr28(void);
extern void isr29(void);
extern void isr30(void);
extern void isr31(void);

// ----- helpers for IDT -----
static void idt_set_gate(int n, void (*handler)(void), uint8_t type_attr)
{
    uint64_t addr = (uint64_t)handler;

    idt[n].offset_low  = (uint16_t)(addr & 0xFFFF);
    idt[n].selector    = KERNEL_CS;
    idt[n].ist         = 0;
    idt[n].type_attr   = type_attr;   // 0x8E = interrupt gate, ring0
    idt[n].offset_mid  = (uint16_t)((addr >> 16) & 0xFFFF);
    idt[n].offset_high = (uint32_t)((addr >> 32) & 0xFFFFFFFF);
    idt[n].zero        = 0;
}

// ----- exception messages -----
static const char *exception_messages[] = {
    "0: Divide-by-zero",
    "1: Debug",
    "2: NMI",
    "3: Breakpoint",
    "4: Overflow",
    "5: Bound-Range",
    "6: Invalid Opcode",
    "7: Device Not Available",
    "8: Double Fault",
    "9: Coprocessor Segment Overrun",
    "10: Invalid TSS",
    "11: Segment Not Present",
    "12: Stack-Segment Fault",
    "13: General Protection Fault",
    "14: Page Fault",
    "15: Reserved",
    "16: x87 Floating-Point",
    "17: Alignment Check",
    "18: Machine Check",
    "19: SIMD Floating-Point",
    "20: Virtualization",
    "21: Control Protection",
    "22: Reserved",
    "23: Reserved",
    "24: Reserved",
    "25: Reserved",
    "26: Reserved",
    "27: Reserved",
    "28: Hypervisor Injection",
    "29: VMM Communication",
    "30: Security Exception",
    "31: Reserved"
};

// ----- simple VGA panic screen -----
#define VGA_MEM   ((volatile uint16_t*)0xB8000)
#define VGA_W     80
#define VGA_H     25

static void vga_clear_panic(void)
{
    uint16_t attr = (0x4 << 4) | 0xF;
    uint16_t cell = ((uint16_t)attr << 8) | ' ';
    for (int i = 0; i < VGA_W * VGA_H; ++i)
        VGA_MEM[i] = cell;
}

static void vga_puts_at(int row, const char *s)
{
    uint16_t attr = (0x4 << 4) | 0xF;
    int col = 0;
    while (*s && row < VGA_H) {
        if (*s == '\n') {
            row++;
            col = 0;
        } else if (col < VGA_W) {
            VGA_MEM[row*VGA_W + col] =
                ((uint16_t)attr << 8) | (uint8_t)(*s);
            col++;
        }
        s++;
    }
}

static void vga_print_hex64(int row, int col, uint64_t val)
{
    uint16_t attr = (0x4 << 4) | 0xF;
    const char *hex = "0123456789ABCDEF";
    for (int i = 0; i < 16; ++i) {
        int shift = (15 - i) * 4;
        char c = hex[(val >> shift) & 0xF];
        VGA_MEM[row*VGA_W + (col + i)] =
            ((uint16_t)attr << 8) | (uint8_t)c;
    }
}

// ----- common exception handler (called from isr.asm) -----
void isr_handler(regs_t *r)
{
    vga_clear_panic();

    vga_puts_at(0,  "KERNEL PANIC");
    vga_puts_at(1,  "CPU exception occurred.");
    vga_puts_at(3,  "Exception:");
    if (r->int_no < 32)
        vga_puts_at(4, exception_messages[r->int_no]);
    else
        vga_puts_at(4, "Unknown");

    vga_puts_at(6,  "RIP:");
    vga_print_hex64(6, 5, r->rip);

    vga_puts_at(7,  "RSP:");
    vga_print_hex64(7, 5, r->rsp);

    vga_puts_at(8,  "RFLAGS:");
    vga_print_hex64(8, 9, r->rflags);

    vga_puts_at(9,  "ERR:");
    vga_print_hex64(9, 5, r->err_code);

    vga_puts_at(11, "System halted.");

    for (;;)
        __asm__ __volatile__("hlt");
}

// ----- IDT initialization -----
void idt_init(void)
{
    // clear IDT
    for (int i = 0; i < IDT_ENTRIES; ++i) {
        idt[i].offset_low  = 0;
        idt[i].selector    = 0;
        idt[i].ist         = 0;
        idt[i].type_attr   = 0;
        idt[i].offset_mid  = 0;
        idt[i].offset_high = 0;
        idt[i].zero        = 0;
    }

    uint8_t flags = 0x8E; // present, ring0, interrupt gate

    idt_set_gate(0,  isr0,  flags);
    idt_set_gate(1,  isr1,  flags);
    idt_set_gate(2,  isr2,  flags);
    idt_set_gate(3,  isr3,  flags);
    idt_set_gate(4,  isr4,  flags);
    idt_set_gate(5,  isr5,  flags);
    idt_set_gate(6,  isr6,  flags);
    idt_set_gate(7,  isr7,  flags);
    idt_set_gate(8,  isr8,  flags);
    idt_set_gate(9,  isr9,  flags);
    idt_set_gate(10, isr10, flags);
    idt_set_gate(11, isr11, flags);
    idt_set_gate(12, isr12, flags);
    idt_set_gate(13, isr13, flags);
    idt_set_gate(14, isr14, flags);
    idt_set_gate(15, isr15, flags);
    idt_set_gate(16, isr16, flags);
    idt_set_gate(17, isr17, flags);
    idt_set_gate(18, isr18, flags);
    idt_set_gate(19, isr19, flags);
    idt_set_gate(20, isr20, flags);
    idt_set_gate(21, isr21, flags);
    idt_set_gate(22, isr22, flags);
    idt_set_gate(23, isr23, flags);
    idt_set_gate(24, isr24, flags);
    idt_set_gate(25, isr25, flags);
    idt_set_gate(26, isr26, flags);
    idt_set_gate(27, isr27, flags);
    idt_set_gate(28, isr28, flags);
    idt_set_gate(29, isr29, flags);
    idt_set_gate(30, isr30, flags);
    idt_set_gate(31, isr31, flags);

    idtr.limit = (uint16_t)(sizeof(idt) - 1);
    idtr.base  = (uint64_t)&idt[0];

    lidt(&idtr);
}

