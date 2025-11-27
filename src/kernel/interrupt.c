#include "interrupt.h"
#include "kernel.h"

extern void keyboard_handler(void);
extern void timer_handler(void);

#define PIC_MASTER_CMD   0x20
#define PIC_MASTER_DATA  0x21
#define PIC_SLAVE_CMD    0xA0
#define PIC_SLAVE_DATA   0xA1

typedef struct {
    uint16_t offset_low;
    uint16_t segment;
    uint8_t  ist;
    uint8_t  flags;
    uint16_t offset_mid;
    uint32_t offset_high;
    uint32_t reserved;
} __attribute__((packed)) idt_entry_t;

static idt_entry_t idt[256] = {0};

static void idt_set_gate(int vector, void* handler) {
    uint64_t addr = (uint64_t)handler;

    idt[vector].offset_low  = (uint16_t)(addr & 0xFFFF);
    idt[vector].offset_mid  = (uint16_t)((addr >> 16) & 0xFFFF);
    idt[vector].offset_high = (uint32_t)((addr >> 32) & 0xFFFFFFFF);


    idt[vector].segment  = 0x18;
    idt[vector].ist      = 0;
    idt[vector].flags    = 0x8E;   // present, ring0, interrupt gate
    idt[vector].reserved = 0;
}

static void idt_init(void) {
    // IRQ0 → вектор 32, IRQ1 → вектор 33
    idt_set_gate(32, timer_handler);
    idt_set_gate(33, keyboard_handler);

    struct {
        uint16_t limit;
        uint64_t base;
    } __attribute__((packed)) idtr = {
        .limit = sizeof(idt) - 1,
        .base  = (uint64_t)idt
    };

    lidt(&idtr);
}

static void pic_init(void) {

    outb(PIC_MASTER_DATA, 0xFF);
    outb(PIC_SLAVE_DATA,  0xFF);

    // ICW1
    outb(PIC_MASTER_CMD, 0x11);
    outb(PIC_SLAVE_CMD,  0x11);

    // ICW2
    outb(PIC_MASTER_DATA, 0x20);   // master 0x20–0x27
    outb(PIC_SLAVE_DATA,  0x28);   // slave  0x28–0x2F

    // ICW3: master/slave
    outb(PIC_MASTER_DATA, 0x04);   // slave on IRQ2
    outb(PIC_SLAVE_DATA,  0x02);

    // ICW4: 8086 mode
    outb(PIC_MASTER_DATA, 0x01);
    outb(PIC_SLAVE_DATA,  0x01);

    // OCW1: IRQ0 (bit0=0) и IRQ1 (bit1=0)
    outb(PIC_MASTER_DATA, 0xFC);   // 11111100
    outb(PIC_SLAVE_DATA,  0xFF);
}

void idt_register_handler(int vector, void* handler) {
    if (vector >= 0 && vector < 256) {
        idt_set_gate(vector, handler);
    }
}

void interrupt_init(void) {
    idt_init();
    pic_init();
}

