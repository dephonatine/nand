#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "asm.h"

#define     USAGE() { printf("USAGE: ./assemble [program.asm]\n"); exit(0);}
#define     CHECK(argc) { if (argc < 2) { USAGE(); } }
#define     sfree(x) { do { free(x); x=NULL; } while(0) };
#define     smalloc(x) { do { ptr = malloc(x); if(!ptr) { exit(fprintf(stderr, "malloc error\n"));  } return ptr; } while(0) }
#define     INSTRUCTION_C       0
#define     INSTRUCTION_A       1

/* assembler for the Hack ISA. */

FILE *output;


uint16_t g_addr;
struct line_data *head;
struct line_data *cur;
struct symbol_data *symhead;
struct symbol_data *symcur;
uint32_t lnum;


FILE *open_file(char *file) {
    FILE *fp = fopen(file, "r+");
    assert(fp != -1);
    return fp;
}


uint8_t *read_file(char *file) {
    FILE *fp = fopen(file, "r+");
            
    fseek(fp, 0, SEEK_END);
    uint64_t size = ftell(fp);
    uint8_t *buf = malloc(size + 1);
    fseek(fp, 0, SEEK_SET);
    fread(buf, 1, size, fp);
    return buf;

}

// initialize both list heads
void INIT_LIST_HEAD(void) {
    symhead = malloc(sizeof(struct symbol_data) + 1);
    symhead->symbol = "SP";
    symhead->list.next = NULL;
    symhead->list.prev = NULL;
    symcur = symhead;

    head = malloc(sizeof(struct line_data) + 1);
    head->data = "this is the list head";
    head->list.next = NULL;
    head->list.prev = NULL;
    cur = head;
    return;
}


struct line_data *line_add(struct line_data *new) {
    struct line_data *tmp = cur;
    new->list.prev = tmp;
    new->list.next = NULL;
    tmp->list.next = new;
    cur = new;
    return cur;
}


struct symbol_data *sym_add(struct symbol_data *new) {
    struct symbol_data *tmp = symcur;
    new->list.prev = tmp;
    new->list.next = NULL;
    tmp->list.next = new;
    symcur = new;
    return symcur;
}


struct line_data *readline(uint8_t *buf) {
    uint8_t i = 0;
    if(buf[i] == NULL) { return NULL; }
    struct line_data *line = malloc(sizeof(struct line_data) + 1);
    line->data = malloc(0x50);
    line->opcode = malloc(18);
    line->num = lnum;
    lnum++;
    while(buf[i] != 10) {
        line->data[i] = buf[i];
        i++;
    }
    line->size = i;

    struct line_data *ret = line_add(line);

    return ret;
}


void populate_list(uint8_t *buf) {
    size_t offset = 0;
    for(int i = 0; cur = readline(buf + offset); i++) {
        offset += cur->size;
    }
    return;
}
 

void show_list(struct line_data *ptr) {
    while(ptr) {
        printf("line %d: %s\n", ptr->num, ptr->data);
        ptr = ptr->list.next;
    }
    return;
}


void show_syms(struct symbol_data *ptr) { 
    while(ptr->list.next) { 
        printf("Symbol: %s\nAddress:%d\n", ptr->symbol, ptr->addr);
        ptr = ptr->list.next;
    }
}


void handle_sym(uint8_t *sym, struct line_data *s) {
    printf("Symbol detected: %s\n", sym);
    printf("[^] Checking if symbol already exists.\n");
    struct symbol_data *ptr = symhead;
    while(ptr->list.next) {
        if(!strcmp(sym, ptr->symbol)) { 
            printf("[^] Symbol found. Appending corresponding opcode.\n");
            dec2bin(ptr->addr, s->opcode);
            printf("[DEBUG] opcode: %s\n", s->opcode);
            return;
        }
        ptr = ptr->list.next;
    }
    
    printf("[^] Symbol not found. Adding to table.\n");
    
    symcur->symbol = sym;
    symcur->addr = g_addr;
    dec2bin(g_addr, s->opcode);
    g_addr++;
    struct symbol_data *new = malloc(sizeof(struct symbol_data) + 1);
    sym_add(new);


    return;

}


struct c_inst *tokenize_c(uint8_t *ptr) {
    struct c_inst *x = malloc(sizeof(struct c_inst) + 1);
    // dest and jump are mutually exclusive

    uint8_t **val1, **val2;
    uint8_t *TOKEN = { 0 };

    if(strstr(ptr, ";")) { // this breaks if for example a ';' is in comment on same line
        x->type = JMP;
        val1 = &x->comp;
        val2 = &x->jump;
        TOKEN = "\r;";
    }
    else { 
        x->type = EQ;
        val1 = &x->dest;
        val2 = &x->comp;
        x->jump = "000";
        TOKEN = "\r=";
    }

    uint8_t *tok = strtok(ptr, TOKEN);
    *val1 = tok;
    uint8_t *tok2 = strtok(NULL, TOKEN);
    strtok(tok2, "/");
    strtok(tok2, " ");
    *val2 = tok2;
    printf("val1: %s\n", *val1);
    printf("val2: %s\n", *val2);
    return x;
}


uint8_t check_opcode(uint8_t type, struct c_inst *instruction) {
    uint8_t k = 0;
    uint8_t **mnem;
    uint8_t *val;
    switch(type) { 
        case JMP:
            k = 7;
            mnem = jmp_mnemonic;
            val = instruction->jump;
            break;
        case DEST:
            k = 8;
            mnem = dest_mnemonic;
            val = instruction->dest;
            break;
    
        case COMP:
            k = 28;
            mnem = c_mnemonic;
            val = instruction->comp;
            break;
    }
    
    for(int i = 0; i < k; i++) { 
        if(!strcmp(val, mnem[i])) { 
            return i;
        }
    
    }

}


void handle_c(struct line_data *ptr) { 
    struct c_inst *instruction = tokenize_c(ptr->data);
    uint8_t cidx = check_opcode(COMP, instruction);
    sprintf(ptr->opcode, "111");
    if(instruction->type == JMP) {
        strcat(ptr->opcode, c_opcode[cidx]);
        strcat(ptr->opcode, "000");
        uint8_t jidx = check_opcode(JMP, instruction);
        strcat(ptr->opcode, jmp_opcode[jidx]);
        return;
    }
    else { 
        strcat(ptr->opcode, c_opcode[cidx]);
        uint8_t didx = check_opcode(DEST, instruction);
        strcat(ptr->opcode, dest_opcode[didx]);
        strcat(ptr->opcode, "000\n");
        return;
    
    }
}


void a_inst(struct line_data *ptr) {
    if(!strstr(ptr->data, "@")) { goto bail; }
    char *tok = strtok(ptr->data, "@");
    strtok(ptr->data, "/");
    strtok(ptr->data, " ");
    printf("ADDRESS: %s\n", tok);

    if((tok[0] <= 0x5A && tok[0] >= 0x41) || (tok[0] <= 0x7A && tok[0] >= 0x61)) {
        handle_sym(tok, ptr);
        ptr->state = DONE;
        return;
    }
    else {
        long val = strtol(tok, NULL, 10);
        dec2bin((uint16_t)val, ptr->opcode);
        ptr->state = DONE;
        return;
    }
bail:
    return;
}


// modular for debugging purposes. remove type parameter later.
void handle_all_inst(struct line_data *start, uint8_t type) {
    void *ptr[] = {&handle_c, &a_inst};
    void (*func)(struct line_data *) = ptr[type];
    struct line_data *tmp = start;
    while(tmp) {
        if (tmp->state == DONE || tmp->data[0] == NULL) { goto next; }
        func(tmp);
next:
        tmp = tmp->list.next;
    }

}


void remove_line(struct line_data *x) { 
    struct line_data *next;
    struct line_data *prev;
    x->list.next = next;
    x->list.prev = prev;
    next->list.prev = prev;
    prev->list.next = next;
    free(x);
}


void flush_file(struct line_data *ptr, char *arg) {
    uint8_t *tok = strtok(arg, ".");
    uint8_t *fout = malloc(0x24);
    sprintf(fout, "%s.hack", tok);
    printf("%s\n", fout);
    output = fopen(fout, "w+");
    while(ptr) {
        fprintf(output, ptr->opcode);
        ptr = ptr->list.next;
    }
}

void first_pass(struct line_data *ptr) { 
    while(ptr) { 
        if(strstr(ptr->data, ")")) {
            uint8_t *tok = strtok(ptr->data, ")");
            strtok(tok, "(");
            struct symbol_data *sym = malloc(sizeof(struct symbol_data) + 1);
            sym->symbol = tok;
            

        }
        ptr = ptr->list.next;
    }

}


void main(int argc, char *argv[]) {
    CHECK(argc);
    if(!strstr(argv[1], ".asm")) { USAGE(); }
    lnum = 0;
    g_addr = 16; // Symbols  begin at memory address 16.
    uint8_t *buf = read_file(argv[1]);
    INIT_LIST_HEAD();
    for(int i = 0; i < 10; i++) { 
        printf("%d ", buf[i]);
    }
    populate_list(buf);
    struct line_data *ptr = head;
    struct line_data *strt = ptr->list.next;
 //   handle_all_inst(ptr, INSTRUCTION_A);
 //   handle_all_inst(strt, INSTRUCTION_C);
 //   flush_file(strt, argv[1]);
    show_list(strt);
    // symbols already defined by the ISA




}
