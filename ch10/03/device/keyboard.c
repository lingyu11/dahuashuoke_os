#include "keyboard.h"
#include "print.h"
#include "interrupt.h"
#include "io.h"
#include "global.h"
#include "stdbool.h"
#include "ioqueue.h"

#define KBD_BUF_PORT 0x60

/* 控制字符定义 */
#define esc        '\033'
#define backspace  '\b'
#define tab        '\t'
#define enter      '\r'
#define delete     '\177'

#define char_invisible  0
#define ctrl_l_char     char_invisible
#define ctrl_r_char     char_invisible
#define shift_l_char    char_invisible
#define shift_r_char    char_invisible
#define alt_l_char      char_invisible
#define alt_r_char      char_invisible
#define caps_lock_char  char_invisible

/* 控制键通码和断码 */
#define shift_l_make    0x2a
#define shift_r_make    0x36
#define alt_l_make      0x38
#define alt_r_make      0xe038
#define alt_r_break     0xe0b8
#define ctrl_l_make     0x1d
#define ctrl_r_make     0xe01d
#define ctrl_r_break    0xe09d
#define caps_lock_make  0x3a

struct ioqueue kbd_buf;

/* 状态管理变量 */
static bool ctrl_status, shift_status, alt_status, caps_lock_status, ext_scancode;

//扫描码从从0x00到0x3A  可以作为键盘映射表的索引 值为字符
static char keymap[][2] = {
    {0, 0}, {esc, esc}, {'1', '!'}, {'2', '@'},
    {'3', '#'}, {'4', '$'}, {'5', '%'}, {'6', '^'},
    {'7', '&'}, {'8', '*'}, {'9', '('}, {'0', ')'},
    {'-', '_'}, {'=', '+'}, {backspace, backspace},
    {tab, tab}, {'q', 'Q'}, {'w', 'W'}, {'e', 'E'},
    {'r', 'R'}, {'t', 'T'}, {'y', 'Y'}, {'u', 'U'},
    {'i', 'I'}, {'o', 'O'}, {'p', 'P'}, {'[', '{'},
    {']', '}'}, {enter, enter}, {ctrl_l_char, ctrl_l_char},
    {'a', 'A'}, {'s', 'S'}, {'d', 'D'}, {'f', 'F'},
    {'g', 'G'}, {'h', 'H'}, {'j', 'J'}, {'k', 'K'},
    {'l', 'L'}, {';', ':'}, {'\'', '"'}, {'`', '~'},
    {shift_l_char, shift_l_char}, {'\\', '|'}, {'z', 'Z'},
    {'x', 'X'}, {'c', 'C'}, {'v', 'V'}, {'b', 'B'},
    {'n', 'N'}, {'m', 'M'}, {',', '<'}, {'.', '>'},
    {'/', '?'}, {shift_r_char, shift_r_char}, {'*', '*'},
    {alt_l_char, alt_l_char}, {' ', ' '}, {caps_lock_char, caps_lock_char}
};

/* 键盘中断处理程序 */
static void intr_keyboard_handler(void) {
    bool ctrl_down_last = ctrl_status;
    bool shift_down_last = shift_status;
    bool caps_lock_last = caps_lock_status;
    bool break_code;
    uint16_t scancode = inb(KBD_BUF_PORT);
    bool is_letter, is_digit_symbol;
    bool shift = false;
    uint8_t index = 0;
    char cur_char;

    if (scancode == 0xe0) {
        ext_scancode = true;
        return;
    }

    if (ext_scancode) {
        scancode = ((0xe000) | scancode);
        ext_scancode = false;
    }

    break_code = ((scancode & 0x0080) != 0);

    if (break_code) {
        uint16_t make_code = (scancode & 0xff7f);

        if (make_code == ctrl_l_make || make_code == ctrl_r_make) {
            ctrl_status = false;
        } else if (make_code == shift_l_make || make_code == shift_r_make) {
            shift_status = false;
        } else if (make_code == alt_l_make || make_code == alt_r_make) {
            alt_status = false;
        }
        return;
    } 

    /* 通码处理 */
    if ((scancode > 0x00 && scancode < 0x3b) || 
        (scancode == alt_r_make) || 
        (scancode == ctrl_r_make)) {

        index = (scancode & 0x00ff);

        /* 判断是否为字母键 */
        is_letter = (scancode >= 0x10 && scancode <= 0x1c) || 
                    (scancode >= 0x1e && scancode <= 0x26) || 
                    (scancode >= 0x2c && scancode <= 0x32);

        /* 判断是否为数字/符号键 */
        is_digit_symbol = (scancode >= 0x02 && scancode <= 0x0d) || 
                          (scancode >= 0x1a && scancode <= 0x1b) || 
                          (scancode >= 0x27 && scancode <= 0x29) || 
                          (scancode >= 0x33 && scancode <= 0x35);

        if (is_letter) {
            /* 字母键处理逻辑 */
            if (shift_down_last && caps_lock_last) {
                shift = false;
            } else if (shift_down_last || caps_lock_last) {
                shift = true;
            } else {
                shift = false;
            }
        } else if (is_digit_symbol) {
            shift = shift_down_last;
        }

        cur_char = keymap[index][shift];

        if (cur_char) {
            if (!ioq_full(&kbd_buf)) {
                //put_char(cur_char);
                ioq_putchar(&kbd_buf, cur_char);
            }
            return;
        }

        /* 更新控制键状态 */
        if (scancode == ctrl_l_make || scancode == ctrl_r_make) {
            ctrl_status = true;
        } else if (scancode == shift_l_make || scancode == shift_r_make) {
            shift_status = true;
        } else if (scancode == alt_l_make || scancode == alt_r_make) {
            alt_status = true;
        } else if (scancode == caps_lock_make) {
            caps_lock_status = !caps_lock_status;
        }
    }
}

/* 键盘初始化 */
void keyboard_init(void) {
    put_str("keyboard init start\n");
    ioqueue_init(&kbd_buf);
    register_handler(0x21, (intr_handler)intr_keyboard_handler);
    put_str("keyboard init done\n");
}