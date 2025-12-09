#include <stdint.h>
#include <stddef.h>
#define SC_UP_ARROW 0x48
#define HISTORY_MAX 10

/* --- VGA DRIVER --- */
static const size_t VGA_WIDTH = 80;
static const size_t VGA_HEIGHT = 25;
static uint16_t* const VGA_MEMORY = (uint16_t*) 0xB8000;

size_t terminal_row = 0;
size_t terminal_column = 0;
uint8_t terminal_color = 0x07;
uint16_t* terminal_buffer = (uint16_t*) 0xB8000;
int ctrl_pressed=0;
uint32_t uptime_seconds=0;
uint32_t seed=1;
char history[HISTORY_MAX][256];
int history_count=0;
int history_index=-1;


uint32_t rand32(){
   seed=seed*134253+1526376474;
   return seed;
}

static inline uint16_t vga_entry(unsigned char uc, uint8_t color) {
    return (uint16_t) uc | (uint16_t) color << 8;
}

size_t strlen(const char* str) {
    size_t len = 0;
    while (str[len]) len++;
    return len;
}

int strcmp(const char* s1, const char* s2) {
    while(*s1 && (*s1 == *s2)) {
        s1++; s2++;
    }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

void terminal_initialize(void) {
    terminal_row = 0;
    terminal_column = 0;
    for (size_t y = 0; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            terminal_buffer[y * VGA_WIDTH + x] = vga_entry(' ', terminal_color);
        }
    }
}

void terminal_putchar(char c) {
    if (c == '\n') {
        terminal_column = 0;
        terminal_row++;
    } else if (c == '\b') {
        if (terminal_column > 0) {
            terminal_column--;
            terminal_buffer[terminal_row * VGA_WIDTH + terminal_column] = vga_entry(' ', terminal_color);
        }
    } else {
        size_t index = terminal_row * VGA_WIDTH + terminal_column;
        terminal_buffer[index] = vga_entry(c, terminal_color);
        terminal_column++;
    }

    if (terminal_column >= VGA_WIDTH) {
        terminal_column = 0;
        terminal_row++;
    }
    if (terminal_row >= VGA_HEIGHT) terminal_row = 0;
}

void terminal_write(const char* data, size_t size) {
    for (size_t i = 0; i < size; i++) terminal_putchar(data[i]);
}

void terminal_writestring(const char* data) {
    terminal_write(data, strlen(data));
}

/* --- HELPER: Integer to String (itoa) --- */
/* We need this to print file sizes! */
void terminal_write_dec(uint32_t num) {
    if (num == 0) {
        terminal_putchar('0');
        return;
    }

    char buffer[32];
    int i = 0;
    while (num > 0) {
        buffer[i++] = (num % 10) + '0';
        num /= 10;
    }

    /* Print in reverse order */
    while (--i >= 0) {
        terminal_putchar(buffer[i]);
    }
}

void get_cpu_string(char* out){
   uint32_t regs[4];
   
   __asm__ volatile(
      "cpuid"
      : "=a"(regs[0]),"=b"(regs[1]),"=c"(regs[2]),"=d"(regs[3])
      : "a"(0)
   );
   *(uint32_t*)&out[0]=regs[1];
   *(uint32_t*)&out[4]=regs[3];
   *(uint32_t*)&out[8]=regs[2];
   out[12]=0;
}

/* --- MOCK FILE SYSTEM --- */

/* 1. Define what a file entry looks like */
struct FileEntry {
    char name[32];
    uint32_t size;     // File size in bytes
    uint8_t flags;     // 0 = File, 1 = Directory
};

/* 2. Create some fake files in memory */
struct FileEntry file_system[] = {
    {"kernel.elf",  102400, 0}, /* The kernel itself */
    {"boot",        0,      1}, /* A directory */
    {"grub.cfg",    256,    0}, /* Config file */
    {"readme.txt",  42,     0}, /* Text file */
    {"passwords",   12,     0}  /* Secrets */
};

/* Calculate number of files in our array */
#define FS_COUNT (sizeof(file_system) / sizeof(struct FileEntry))

/* 3. The Logic for LS */
void command_ls(int show_details) {
    for (size_t i = 0; i < FS_COUNT; i++) {
        struct FileEntry* file = &file_system[i];

        if (show_details) {
            /* Logic for 'ls -l' */
            /* Print Type (d for dir, - for file) */
            if (file->flags == 1) terminal_writestring("d ");
            else terminal_writestring("- ");

            /* Print Size (padded manually for alignment) */
            terminal_writestring("Size: ");
            terminal_write_dec(file->size);
            terminal_writestring(" B   ");
            
            /* Print Name */
            terminal_writestring(file->name);
            terminal_putchar('\n');
        } else {
            /* Logic for simple 'ls' */
            terminal_writestring(file->name);
            
            /* Add a slash if it's a directory */
            if (file->flags == 1) terminal_putchar('/');
            
            terminal_writestring("  ");
        }
    }
    terminal_putchar('\n');
}

/* --- KEYBOARD AND SHELL --- */

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ( "inb %1, %0" : "=a"(ret) : "Nd"(port) );
    return ret;
}

char kbd_US[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
  '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',   0,
  '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',   0, '*', 0, ' '
};

char get_char() {
    char c = 0;
    while(1) {
        if (inb(0x64) & 0x1) {
            uint8_t scancode = inb(0x60);
            if (scancode & 0x80){
                uint8_t released=scancode & 0x7F;
                
                if(released == 0x1D)
                   ctrl_pressed = 0;
                continue;
            }
            
            if (scancode == 0x1D){
               ctrl_pressed=1;
               continue;
            }
            
            if(ctrl_pressed && scancode == 0x2E){
               return 3;
            }
            c = kbd_US[scancode];
            if (c != 0) return c;
        }
    }
}

void shell() {
    char buffer[256];
    int index = 0;
    
    terminal_writestring("\n> ");

    while(1) {
        char c = get_char();
        uptime_seconds+=1;
        
        if(c==3){
           terminal_writestring("^c\n");
           index=0;
           terminal_writestring("> ");
           continue;
        }
        if (c == '\n') {
            terminal_putchar('\n');
            buffer[index] = '\0';

            /* --- COMMAND PARSING --- */
            if (strcmp(buffer, "help") == 0) {
                terminal_writestring("Available commands: help, echo, cls, ls, ls -l\n");
            } 
            else if (strcmp(buffer, "ls") == 0) {
                command_ls(0); /* 0 = No details */
            }
            else if(strcmp(buffer,"time")==0){
               terminal_writestring("uptime: ");
               terminal_writestring(uptime_seconds);
               terminal_writestring(" seconds\n");
            }
            else if(strcmp(buffer,"cpu")==0){
               char cpu_name[13];
               get_cpu_string(cpu_name);
               terminal_writestring("CPU: ");
               terminal_writestring(cpu_name);
               terminal_putchar('\n');
            }
            else if(strcmp(buffer,"rand")==0){
               terminal_write_dec(rand32());
               terminal_putchar('\n');
            }
            else if(strcmp(buffer,"halt")==0){
               terminal_writestring("BYE!!!");
               while(1) __asm__ volatile("hlt");
            }
            else if(strcmp(buffer,"whoami")==0){
               terminal_writestring("kaushik");
            }
            else if (strcmp(buffer, "ls -l") == 0) {
                command_ls(1); /* 1 = Show details */
            }
            else if (strcmp(buffer, "cls") == 0) {
                terminal_initialize();
            }
            /* Handle echo (naive implementation) */
            else if (buffer[0] == 'e' && buffer[1] == 'c' && buffer[2] == 'h' && buffer[3] == 'o' && buffer[4] == ' ') {
                terminal_writestring(buffer + 5);
                terminal_putchar('\n');
            }
            else if (index > 0) {
                terminal_writestring("Unknown command.\n");
            }

            index = 0;
            terminal_writestring("> ");
        } 
        else if (c == '\b') {
            if (index > 0) {
                terminal_putchar('\b');
                index--;
            }
        } 
        else {
            if (index < 255) {
                buffer[index++] = c;
                terminal_putchar(c);
            }
        }
    }
}

void kernel_main(void) {
    terminal_initialize();
    terminal_writestring("Kernel Loaded.\n");
    terminal_writestring("Type 'ls' to list files.\n");
    shell();
}
