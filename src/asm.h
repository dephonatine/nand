#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define     JMP                 0
#define     EQ                  1
#define     PENDING             2
#define     DONE                3
#define     DEST                4
#define     COMP                5
#define	    VAL			6
#define	    LABEL		7
#define     INSTRUCTION_C       8
#define     INSTRUCTION_A       9

struct list_head {
    struct  list_head *next;
    struct  list_head *prev;
} __attribute__((packed));

struct line_data {
    uint8_t     *data;
    uint8_t     *opcode;
    uint8_t     state;
    size_t      size;
    uint32_t    num;
    struct list_head list;
} __attribute__((packed));

struct symbol_data {
    uint8_t     *symbol;
    uint16_t    addr; 
    uint8_t	type;
    struct list_head list;
} __attribute__((packed));


struct c_inst { 
    uint8_t     *comp;
    uint8_t     *dest;
    uint8_t     *jump;
    uint8_t     type;
} __attribute__((packed));


extern uint32_t lnum;
extern struct line_data *head;
extern struct line_data *cur;
extern struct symbol_data *symhead;
extern struct symbol_data *symcur;
extern uint16_t g_addr;
extern struct symbol_data *screen;
extern struct symbol_data *kbd;

/* symbols defined by the ISA */
#define INIT_SYMS() {\
    struct symbol_data *screen = malloc(sizeof(struct symbol_data) + 1);\
    struct symbol_data *kbd = malloc(sizeof(struct symbol_data) + 1); \
    struct symbol_data *arg = malloc(sizeof(struct symbol_data) + 1);\
    struct symbol_data *this = malloc(sizeof(struct symbol_data) + 1);\
    struct symbol_data *that = malloc(sizeof(struct symbol_data) + 1);\
    struct symbol_data *next = malloc(sizeof(struct symbol_data) + 1);\
    for(int i = 0; i < 15; i++) {\
        uint8_t *nm = malloc(8);\
        struct symbol_data *s = malloc(sizeof(struct symbol_data) + 1);\
        sprintf(nm, "R%d", i);\
        s->symbol = nm;\
        s->addr = i;\
        sym_add(s);\
    }\
    screen->symbol = "SCREEN";\
    screen->addr = 16384;\
    kbd->symbol = "KBD";\
    kbd->addr = 24576;\
    arg->symbol = "ARG";\
    arg->addr = 2;\
    this->symbol = "THIS";\
    this->addr = 3;\
    that->symbol = "THAT";\
    that->addr = 4;\
    sym_add(screen);\
    sym_add(kbd);\
    sym_add(arg);\
    sym_add(this);\
    sym_add(that);\
    sym_add(next);\
}

/* not mine */
uint8_t *dec2bin(uint16_t c, uint8_t *str) {
    uint8_t *tmp = str;
    int i = 0;
    for(i = 15; i >= 0; i--){
        if((c & (1 << i)) != 0){
            sprintf(tmp, "1");

        }else{
            sprintf(tmp, "0");

        } 
    tmp++;
    }
    sprintf(tmp, "\n");
    return str;
}


/* mnemonic commands and their corresponding opcodes */

const uint8_t *c_mnemonic[] = {"0", "1", "-1", "D", "A", "!D", "!A", "-D", "-A", "D+1", "A+1", "D-1", "A-1", "D+A", "D-A", "A-D", "D&A", "D|A", "M", "!M", "-M", "M+1", "M-1", "D+M", "D-M", "M-D", "D&M", "D|M"};

const uint8_t *c_opcode[] = {"0101010", "0111111", "0111010", "0001100", "0110000", "0001101", "0110001", "0001111", "0110011", "0011111", "0110111", "0001110", "0110010", "0000010", "0010011", "0000111", "0000000", "0010101", "1110000", "1110001", "1110011", "1110111", "1110010", "1000010", "1010011", "1000111", "1000000", "1010101"};


const uint8_t *dest_mnemonic[] = {"0", "M", "D", "MD", "A", "AM", "AD", "AMD"};
const uint8_t *dest_opcode[] = {"000", "001", "010", "011", "100", "101", "110", "111"};
const uint8_t *jmp_mnemonic[] = {"JGT", "JEQ", "JGE", "JLT", "JNE", "JLE", "JMP"};
const uint8_t *jmp_opcode[] = {"001\n", "010\n", "011\n", "100\n", "101\n", "110\n", "111\n"};

