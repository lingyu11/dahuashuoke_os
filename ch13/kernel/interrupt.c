#include "global.h" 
#include "stdint.h" 
#include "io.h"
#include "print.h"
#include "interrupt.h"

#define IDT_DESC_CNT 0x81      // 支持中断数扩展至 0x80

/* 主片和从片的端口定义 */
#define PIC_M_CTRL 0x20           // 主片的控制端口是0x20
#define PIC_M_DATA 0x21           // 主片的数据端口是0x21
#define PIC_S_CTRL 0xa0           // 从片的控制端口是0xa0
#define PIC_S_DATA 0xa1           // 从片的数据端口是0xa1

/* 定义eflags寄存器中的IF位 */
#define EFLAGS_IF 0x00000200

/* 获取eflags寄存器值 */
//#define GET_EFLAGS(EFLAG_VAR) asm volatile("pushfl;popl %0":"=r"(EFLAG_VAR))

/* 中断门描述符结构体 */
struct gate_desc {                     
    uint16_t   func_offset_low_word;   // 中断处理程序在目标代码段内的偏移量低16位
    uint16_t   selector;               // 中断处理程序目标代码段选择子
    uint8_t    dcount;                 // 双字计数字段，固定值
    uint8_t    attribute;              // 门描述符属性             
    uint16_t   func_offset_high_word;  // 中断处理程序在目标代码段内的偏移量高16位
};

extern uint32_t syscall_handler(void); // 声明外部中断处理例程

/* 静态函数声明 */
static void make_idt_desc(struct gate_desc* p_gdesc, uint8_t attr, intr_handler function);
static void idt_desc_init(void);
static void pic_init(void);


/* 全局变量声明 */
static struct gate_desc idt[IDT_DESC_CNT];           // idt是中断描述符表，本质上是中断门描述符数组
extern intr_handler intr_entry_table[IDT_DESC_CNT];  //声明引用kernel.s的中断处理函数入口数组

char* intr_name[IDT_DESC_CNT];          //用于保存异常的名字
intr_handler idt_table[IDT_DESC_CNT];   //定义中断处理程序数组,最终调用的是ide_table中的程序


/* 通用的中断处理函数，一般用在异常出现时的处理 */  
static void general_intr_handler(uint8_t vec_nr) {  
    if (vec_nr == 0x27 || vec_nr == 0x2f) {  
        return; // IRQ7 和 IRQ15 会产生伪中断（spurious interrupt），无需处理  
    }  
    /* 将光标置为 0，从屏幕左上角清出一片打印异常信息的区域，方便阅读 */  
    set_cursor(0);  
    int cursor_pos = 0;  
    while(cursor_pos < 320) {  
        put_char(' ');  
        cursor_pos++;  
    }  

    set_cursor(0);            // 重置光标为屏幕左上角  
    put_str("!!!!!!     excetion message begin  !!!!!!!!!\n");  
    set_cursor(88);           // 从第2行第8个字符开始打印  
    put_str(intr_name[vec_nr]);  
    if (vec_nr == 14) {       // 若为Pagefault，将缺失的地址打印出来并暂停  
        int page_fault_vaddr = 0;  
        asm volatile ("movl %%cr2, %0" : "=r" (page_fault_vaddr));
        // cr2 是存放造成 page_fault 的地址  
        put_str("\npage fault addr is "); 
        put_int(page_fault_vaddr);  
    }  
  
    put_str("\n!!!!!!     excetion message end    !!!!!!!!!\n");  
    // 进入中断处理程序就表示已经处在关中断情况下，不会出现调度进程的情况。故下面的死循环不会再被中断  
    while(1);  
}


/* 注册通用中断处理函数及异常名称 */
static void exception_init(void) {
    int i;
    for (i=0;i<IDT_DESC_CNT;i++) {
        idt_table[i] = general_intr_handler;// 默认为 general_intr_handler
        intr_name[i] = "unknown"; // 先统一赋值为 unknown
    }
    intr_name[0] = "#DE Divide Error";
    intr_name[1] = "#DB Debug Exception";
    intr_name[2] = "NMI Interrupt";
    intr_name[3] = "#BP Breakpoint Exception";
    intr_name[4] = "#OF Overflow Exception";
    intr_name[5] = "#BR BOUND Range Exceeded Exception";
    intr_name[6] = "#UD Invalid Opcode Exception";
    intr_name[7] = "#NM Device Not Available Exception";
    intr_name[8] = "#DF Double Fault Exception";
    intr_name[9] = "Coprocessor Segment Overrun";
    intr_name[10] = "#TS Invalid TSS Exception";
    intr_name[11] = "#NP Segment Not Present";
    intr_name[12] = "#SS Stack Fault Exception";
    intr_name[13] = "#GP General Protection Exception";
    intr_name[14] = "#PF Page-Fault Exception";
    // intr_name[15] 第15项是intel保留项，未使用
    intr_name[16] = "#MF x87 FPU Floating-Point Error";
    intr_name[17] = "#AC Alignment Check Exception";
    intr_name[18] = "#MC Machine-Check Exception";
    intr_name[19] = "#XF SIMD Floating-Point Exception";
}

/* 完成有关中断的所有初始化工作 - 主函数（调用者） */
void intr_init() {
    put_str("idt_init start\n");
    idt_desc_init();  // 初始化中断描述符表
    exception_init();  //异常名初始化并注册通常的中断处理函数
    pic_init();       // 初始化8259A
    
    /* 加载idt */
    uint64_t idt_operand = ((sizeof(idt) - 1) | ((uint64_t)((uint32_t)idt) << 16));
    asm volatile("lidt %0" : : "m"(idt_operand));
    put_str("idt_init done\n");
}


static void idt_desc_init(void) {
    int i, lastindex = IDT_DESC_CNT - 1;
    for (i = 0; i < IDT_DESC_CNT; i++) {
        make_idt_desc(&idt[i], IDT_DESC_ATTR_DPL0, intr_entry_table[i]);
    }
    // 单独处理系统调用：DPL=3 允许用户态访问，处理函数为 syscall_handler
    make_idt_desc(&idt[lastindex], IDT_DESC_ATTR_DPL3, (intr_handler)syscall_handler);
    put_str("idt desc init done\n");
}


/* 创建中断门描述符 - 被idt_desc_init调用 */
static void make_idt_desc(struct gate_desc* p_gdesc, uint8_t attr, intr_handler function) {
    p_gdesc->func_offset_low_word = (uint32_t)function & 0x0000FFFF;
    p_gdesc->selector = SELECTOR_K_CODE;  // 修正：应该是SELECTOR_K_CODE
    p_gdesc->dcount = 0;                  // 固定值
    p_gdesc->attribute = attr;            // 门描述符属性
    p_gdesc->func_offset_high_word = ((uint32_t)function & 0xFFFF0000) >> 16;
}


/* 初始化可编程中断控制器 8259A - 被idt_init调用 */
static void pic_init(void) {
    /* 初始化主片 */
    outb(PIC_M_CTRL, 0x11);  // ICW1: 边沿触发, 级联8259, 需要ICW4
    outb(PIC_M_DATA, 0x20);  // ICW2: 起始中断向量号为0x20 (IR[0-7]为0x20~0x27)
    outb(PIC_M_DATA, 0x04);  // ICW3: IR2接从片
    outb(PIC_M_DATA, 0x01);  // ICW4: 8086模式, 正常EOI
    
    /* 初始化从片 */
    outb(PIC_S_CTRL, 0x11);  // ICW1: 边沿触发, 级联8259, 需要ICW4
    outb(PIC_S_DATA, 0x28);  // ICW2: 起始中断向量号为0x28 (IR[8-15]为0x28~0x2F)
    outb(PIC_S_DATA, 0x02);  // ICW3: 设置从片连接到主片的IR2引脚
    outb(PIC_S_DATA, 0x01);  // ICW4: 8086模式, 正常EOI
    
       
    outb(PIC_M_DATA, 0xf8);  // 主片， 开放IR0、1(时钟中断、键盘中断、从片)，IRQ2用于级联从片
    outb(PIC_S_DATA, 0xbf);  // 打开从片上的IRQ14（硬盘）
    
    put_str("pic_init done\n");
}

// 在interrupt.c中替换相关函数

/* 获取当前中断状态 */
enum intr_status intr_get_status(void) {
    uint32_t eflags;
    asm volatile ("pushfl; popl %0" : "=r"(eflags));
    return (eflags & EFLAGS_IF) ? INTR_ON : INTR_OFF;
}

/* 关中断并返回之前中断状态 */
enum intr_status intr_disable(void) {
    uint32_t eflags;
    asm volatile ("pushfl; popl %0; cli" : "=r"(eflags) : : "memory");
    return (eflags & EFLAGS_IF) ? INTR_ON : INTR_OFF;
}

/* 开中断并返回之前中断状态 */
enum intr_status intr_enable(void) {
    // 直接使用sti，不检查当前状态
    uint32_t eflags;
    asm volatile ("pushfl; popl %0; sti" : "=r"(eflags) : : "memory");
    return (eflags & EFLAGS_IF) ? INTR_ON : INTR_OFF;
}

/* 设置中断状态 */
enum intr_status intr_set_status(enum intr_status status) {
    return (status == INTR_ON) ? intr_enable() : intr_disable();
}

/* 在中断处理程序数组第 vector_no 个元素中
   注册安装中断处理程序 function */
void register_handler(uint8_t vector_no, intr_handler function) {
    /* idt_table 数组中的函数是在进入中断后根据中断向量号调用的
       * 见 kernel/kernel.S 的 call [idt_table + %1*4] */
    idt_table[vector_no] = function;
}


