#include "interrupt.h"
#include "kernel.h"
#include "keyboard.h"

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
    uint64_t handler_addr = (uint64_t)handler;
    idt[vector].offset_low  = (uint16_t)(handler_addr & 0xFFFF);
    idt[vector].offset_mid  = (uint16_t)((handler_addr >> 16) & 0xFFFF);
    idt[vector].offset_high = (uint32_t)((handler_addr >> 32) & 0xFFFFFFFF);
    idt[vector].segment     = 0x18;
    idt[vector].ist         = 0;
    idt[vector].flags       = 0x8E;
    idt[vector].reserved    = 0;
}

static void idt_init(void) {
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

// 辅助函数：初始化 PIC（模块内私有）
static void pic_init(void) {
    // 1. 关闭所有 IRQ
    outb(PIC_MASTER_DATA, 0xFF);
    outb(PIC_SLAVE_DATA,  0xFF);

    // 2. 发送 ICW1：边缘触发、需要 ICW4
    outb(PIC_MASTER_CMD, 0x11);
    outb(PIC_SLAVE_CMD,  0x11);

    // 3. 发送 ICW2：设置中断向量偏移
    outb(PIC_MASTER_DATA, 0x20);  // 主 PIC 偏移 0x20
    outb(PIC_SLAVE_DATA,  0x28);  // 从 PIC 偏移 0x28

    // 4. 发送 ICW3：主从级联
    outb(PIC_MASTER_DATA, 0x04);
    outb(PIC_SLAVE_DATA,  0x02);

    // 5. 发送 ICW4：8086 模式
    outb(PIC_MASTER_DATA, 0x01);
    outb(PIC_SLAVE_DATA,  0x01);

    // 6. 开启键盘 IRQ1，屏蔽其他
    outb(PIC_MASTER_DATA, 0xFD);
    outb(PIC_SLAVE_DATA,  0xFF);
}

// Register
void idt_register_handler(int vector, void* handler) {
    if (vector >= 0 && vector < 256) {
        idt_set_gate(vector, handler);
    }
}

// initialization
void interrupt_init(void) {
    idt_init();  // 先初始化 IDT
    pic_init();  // 再初始化 PIC
}