#include <cstdint>
#include <cstdio>
#include <cstring>
#include <thread>
#include <vector>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
//import pthread;
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <unistd.h>
#include <pthread.h>

//set namespace
using namespace std;
#define HDMA1 0xFF51
#define HDMA2 0xFF52
#define HDMA3 0xFF53
#define HDMA4 0xFF54
#define HDMA5 0xFF55
#define VBK 0xFF4F
#define KEY1 0xFF4D
#define RP 0xFF56
#define OPRI 0xFF68
#define SVBK 0xFF70

//memory map
#define ROM0 0x0000
#define ROM1 0x4000
#define VRAM 0x8000
#define ERAM 0xA000
#define WRAM 0xC000
#define OAM 0xFE00
#define HRAM 0xFF80


//VRAM Tile Data
#define VRAM_TILE0 0x8000
#define VRAM_TILE1 0x8800

//VRAM Tile Map
#define VRAM_MAP0 0x9800
#define VRAM_MAP1 0x9C00

//IO Ranges
#define Control 0xFF00
#define Comunication 0xFF01
#define Timer 0xFF01
#define  Sound 0xFF10
#define Waveform_Ram 0xFF30
#define LCD_Control 0xFF40
#define VRAM_Bank 0xFF4F
#define disable_boot 0xFF50
#define VRAM_DMA 0xFF51
#define Palettes 0xFF68
#define WRAM_Bank 0xFF70
//LCD Position and Scrolling
#define SCY 0xFF42 //Scroll Y
#define SCX 0xFF43 //Scroll X
#define LY 0xFF44 //current line
#define LYC 0xFF45 // if equal to LY then the "LYC=LY" flag in the STAT register is set amd STAT interrupt is triggered
#define WY 0xFF4A //Window Y
#define WX 0xFF4B //Window X
//cartridge header
typedef struct cartridge_header {
    char title[16];
    uint8_t logo[48];
    uint8_t type;
    uint8_t rom_size;
    uint8_t ram_size;
    uint8_t destination_code;
    uint8_t old_license_code;
    uint8_t mask_rom_version;
    uint8_t header_checksum;
    uint8_t global_checksum[2];
} cartridge_header;
//declare cartridge header
cartridge_header header;

//LCD Control
#define LCDC 0xFF40
typedef struct lcdc_reg {
    uint8_t LCD_enable: 1; //controls the LCD and PPU if 0 both will be off
    uint8_t Window_tile_map: 1; //0 = $9800-$9BFF, 1 = $9C00-$9FFF controls with bg map the window uses for rendering
    uint8_t Window_enable: 1; // controls if the windows shall be displayed
    uint8_t BG_and_Window_tile_data: 1; //0 = $8800-$97FF, 1 = $8000-$8FFF controls which tile data is used for the background and window
    uint8_t BG_tile_map: 1; //0 = $9800-$9BFF, 1 = $9C00-$9FFF controls which tile map is used for the background
    uint8_t OBJ_enable: 1;
    uint8_t OBJ_size: 1;
    uint8_t BG_and_Window_tile_map: 1;


} lcdc_reg;
uint16_t breakpoint = 0xFFFF;
//LCD Status
#define STAT 0xFF41
typedef struct stat_reg {
    union {
        struct {
            uint8_t mode_7: 1;
            uint8_t LYC_LC_int_source: 1;
            uint8_t mode2_OAM_Stat_int_source: 1;
            uint8_t mode_1_VBlank_Stat_int_source: 1;
            uint8_t mode_0_HBlank_Stat_interrupt_source: 1;
            uint8_t LYC_LY_coincidence_flag: 1;
            uint8_t mode_flag: 2;

        };
        uint8_t value;
    };

} stat_reg;
#define pixel_width 160
#define pixel_height 144
#define pixel_size pixel_width*pixel_height
//palettes
#define BG_palette 0xFF47
#define OBJ_palette0 0xFF48
#define OBJ_palette1 0xFF49
//Timer and Divider Register
#define DIV 0xFF04 //increment at 16384Hz
#define TIMA 0xFF05 //timer counter incremented at the rate specified by TAC
#define TMA 0xFF06 //timer modulo when TIMA overflows it is set to value specified in this register
#define TAC 0xFF07 //controls the timer speed bits 0-1 control the speed of the timer, bit 2 is the timer enable 00 = clock/1024, 01 = clock/16, 10 = clock/64, 11 = clock/256
//Interrupts
#define IF 0xFF0F //interrupt flag bit 0 = VBlank, 1 = LCD STAT, 2 = timer, 3 = serial, 4 = joypad
#define IE 0xFFFF //interrupt enable bit 0 = VBlank, 1 = LCD STAT, 2 = timer, 3 = serial, 4 = joypad


typedef char pixel;
typedef struct tile {
    pixel data[8][8];
} tile;

typedef struct sprite {
    pixel data[8][8];
} sprite;
typedef struct sprite_info {
    uint8_t y; //y position
    uint8_t x; //x position
    uint8_t Tile_index; //tile index
    union {
        struct {
            uint8_t Palette_number: 4;
            uint8_t Priority: 1;
            uint8_t Flip_horizontal: 1;
            uint8_t Flip_vertical: 1;
        };
        uint8_t flags;
    };

} sprite_info;
typedef struct layer {
    pixel data[pixel_width][pixel_height];
} layer;


typedef struct Background {
    tile tiles[256];
} background;


typedef struct Window {
    tile tiles[256];
} window;
typedef struct Object {
    tile tiles[2];
} object;
//registers
typedef struct registers {
    union {
        struct {

            union {
                struct {
                    uint8_t unused: 4;
                    uint8_t c: 1;
                    uint8_t h: 1;
                    uint8_t n: 1;
                    uint8_t z: 1;

                };

                uint8_t F;
            };
            uint8_t A;
        };
        unsigned short AF;
    };

    union {
        struct {
            uint8_t C;
            uint8_t B;
        };
        uint16_t BC;
    };

    union {
        struct {
            uint8_t E;
            uint8_t D;
        };
        uint16_t DE;
    };

    union {
        struct {
            uint8_t L;
            uint8_t H;
        };
        uint16_t HL;
    };

    uint16_t SP;
    uint16_t PC;
    //ime
    uint8_t ime:1;

} registers;


//memory
typedef struct memory {
    uint8_t memory[0xFFFF];
} memory;
typedef struct bank_number {
    union {
        struct {
            uint8_t bank: 5;
            uint8_t unused: 3;
        };
        uint8_t value;
    };
} bank_number;

//rom
typedef struct rom {
    char *bank;
    char buffer[0xFFFF0];
} rom;
//declare rom
rom r;
memory mem;

//register declaration
registers reg;
//reg to string header
char *regop_to_string();
//memory read
uint8_t read_memory(uint16_t address) {
    if (address < 0x8000 && address >= 0x4000) {
        return r.bank[address-0x4000];
    }
    return mem.memory[address];
}

//mem read 16bit
uint16_t read_memory16(uint16_t address) {
    if (address < 0x8000 && address >= 0x4000) {
        return r.bank[address-0x4000] | (r.bank[address + 1-0x4000] << 8);
    }
    return mem.memory[address] | (mem.memory[address + 1] << 8);
}

//mem write 16bit
void write_memory16(uint16_t address, uint16_t value) {
    if (address < 0x8000) {
    } else {
        mem.memory[address] = value & 0xFF;
        mem.memory[address + 1] = (value >> 8) & 0xFF;
    }
}


//mem init
void mem_init() {
    for (int i = 0; i < 0x10000; i++) {
        mem.memory[i] = 0;
    }
    unsigned char ioReset[0x100] = {
            0x0F, 0x00, 0x7C, 0xFF, 0x00, 0x00, 0x00, 0xF8, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x01,
            0x80, 0xBF, 0xF3, 0xFF, 0xBF, 0xFF, 0x3F, 0x00, 0xFF, 0xBF, 0x7F, 0xFF, 0x9F, 0xFF, 0xBF, 0xFF,
            0xFF, 0x00, 0x00, 0xBF, 0x77, 0xF3, 0xF1, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF,
            0x91, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFC, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x7E, 0xFF, 0xFE,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x3E, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xC0, 0xFF, 0xC1, 0x00, 0xFE, 0xFF, 0xFF, 0xFF,
            0xF8, 0xFF, 0x00, 0x00, 0x00, 0x8F, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xCE, 0xED, 0x66, 0x66, 0xCC, 0x0D, 0x00, 0x0B, 0x03, 0x73, 0x00, 0x83, 0x00, 0x0C, 0x00, 0x0D,
            0x00, 0x08, 0x11, 0x1F, 0x88, 0x89, 0x00, 0x0E, 0xDC, 0xCC, 0x6E, 0xE6, 0xDD, 0xDD, 0xD9, 0x99,
            0xBB, 0xBB, 0x67, 0x63, 0x6E, 0x0E, 0xEC, 0xCC, 0xDD, 0xDC, 0x99, 0x9F, 0xBB, 0xB9, 0x33, 0x3E,
            0x45, 0xEC, 0x52, 0xFA, 0x08, 0xB7, 0x07, 0x5D, 0x01, 0xFD, 0xC0, 0xFF, 0x08, 0xFC, 0x00, 0xE5,
            0x0B, 0xF8, 0xC2, 0xCE, 0xF4, 0xF9, 0x0F, 0x7F, 0x45, 0x6D, 0x3D, 0xFE, 0x46, 0x97, 0x33, 0x5E,
            0x08, 0xEF, 0xF1, 0xFF, 0x86, 0x83, 0x24, 0x74, 0x12, 0xFC, 0x00, 0x9F, 0xB4, 0xB7, 0x06, 0xD5,
            0xD0, 0x7A, 0x00, 0x9E, 0x04, 0x5F, 0x41, 0x2F, 0x1D, 0x77, 0x36, 0x75, 0x81, 0xAA, 0x70, 0x3A,
            0x98, 0xD1, 0x71, 0x02, 0x4D, 0x01, 0xC1, 0xFF, 0x0D, 0x00, 0xD3, 0x05, 0xF9, 0x00, 0x0B, 0x00
    };
    //copy from IO reset to memory from 0xFF00
    memcpy(&mem.memory[0xFF00],ioReset, sizeof(ioReset));
}

//reg init
void reg_init() {
    reg.AF = 0x01b0;
    reg.BC = 0x0013;
    reg.DE = 0x00d8;
    reg.HL = 0x014d;
    reg.SP = 0xfffe;
    reg.PC = 0x0100;
}


//load rom From rom.gb
void load_rom() {
    FILE *rom_file;
    rom_file = fopen("rom.gb", "rw");
    fread(r.buffer, 0xFFFF0, 1, rom_file);
    r.bank = r.buffer + 0x4000;
    memcpy(mem.memory, r.buffer, 0x4000);
    fclose(rom_file);
}

//load cartridge header from rom r to header starting from 0x100
void load_header() {
    for (int i = 0; i < 48; i++) {
        header.logo[i] = r.buffer[i + 0x100];
    }
    for (int i = 0; i < 16; i++) {
        header.title[i] = r.buffer[i + 0x134];
    }
    header.type = r.buffer[0x147];
    header.rom_size = r.buffer[0x148];
    header.ram_size = r.buffer[0x149];
    header.destination_code = r.buffer[0x14A];
    header.old_license_code = r.buffer[0x14B];
    header.mask_rom_version = r.buffer[0x14C];
    header.header_checksum = r.buffer[0x14D];
    header.global_checksum[0] = r.buffer[0x14E];
    header.global_checksum[1] = r.buffer[0x14F];
}

//print cartridge header
void print_cartridge_header() {
    printf("Title: %s\n", header.title);
    printf("Type: %d\n", header.type);
    printf("ROM Size: %d\n", header.rom_size);
    printf("RAM Size: %d\n", header.ram_size);
    printf("Destination Code: %d\n", header.destination_code);
    printf("Old License Code: %d\n", header.old_license_code);
    printf("Mask ROM Version: %d\n", header.mask_rom_version);
    printf("Header Checksum: %d\n", header.header_checksum);
    printf("Global Checksum: %s\n", header.global_checksum);
}
//typedef 5 bit bank number
//switch rom bank

void switch_rom_bank(bank_number bank) {
    uint16_t bank_address;
    if (bank.bank == 0) {
        return;
    } else if (bank.bank == 1) {
        bank_address = 0x4000;
    } else {
        bank_address = 0x8000 + (bank.bank - 2) * 0x4000;
    }
    r.bank = r.buffer + bank_address;
}

#define RBN 0x2000

//write to rom register
void write_to_rom_register(uint16_t address, uint8_t value) {

    if (address > RBN && address < RBN + 0x2000) {
        bank_number bank;
        bank.value = value;
        switch_rom_bank(bank);
    }
}

//write memory
void write_memory(uint16_t address, uint8_t value) {
    if (address < 0x8000) {
        write_to_rom_register(address, value);
    } else {
        mem.memory[address] = value;
    }
    mem.memory[address] = value;
}

int wannadie = 0;
//init IO memory

SDL_mutex *display_string_mutex;

char *display_string_string;







void cpu_step(uint8_t opcode) {
    reg.PC++;
    uint8_t temp, cb_opcode;
    unsigned int adjustment = 0;
    uint8_t * regs[] = {&reg.B, &reg.C, &reg.D, &reg.E, &reg.H, &reg.L,nullptr, &reg.A};

    if(opcode==0xF1){

        printf("%d\n",reg.A);
        wannadie=1;
    }
    switch (opcode) {
        case 0x00://NOP
            break;
        case 0x01://LD BC,d16
            reg.BC = read_memory16(reg.PC);
            reg.PC += 2;
            break;
        case 0x02://LD (BC),A
            write_memory(reg.BC, reg.AF & 0x00FF);
            break;
        case 0x03://INC BC
            reg.BC++;
            break;
        case 0x04://INC B
            reg.h=(reg.B&0x0F)==0x0F;
            reg.B++;
            reg.z = (reg.B == 0);
            reg.n = 0;
            break;
        case 0x05://DEC B
            reg.h=(reg.B&0x0F)==0x00;
            reg.B--;
            reg.z = (reg.B == 0);
            reg.n = 0;
            break;
        case 0x06://LD B,d8
            reg.B = read_memory(reg.PC);
            reg.PC++;
            break;
        case 0x07://RLCA rotate A left. Old bit 7 to Carry flag
            reg.F = 0x00;
            reg.c = reg.A >> 7;
            reg.A = (reg.A << 1) | (reg.A >> 7);
            reg.z = 1 * (reg.A == 0);
            break;
        case 0x08://LD (a16),SP
            write_memory16(read_memory16(reg.PC), reg.SP);
            reg.PC += 2;
            break;
        case 0x09://ADD HL,BC
            temp = reg.HL;
            reg.HL += reg.BC;
            reg.z = (reg.HL == 0);
            reg.n = 0;
            reg.h = (temp & 0xFFF) + (reg.BC & 0xFFF) > 0xFFF;
            reg.c = reg.HL >0xFFFF;
            break;
        case 0x0A://LD A,(BC)
            reg.A = read_memory(reg.BC);
            break;
        case 0x0B://DEC BC
            reg.BC--;
            break;
        case 0x0C://INC C
            reg.h=(reg.C&0x0F)==0x0F;
            reg.C++;
            reg.z = (reg.C == 0);
            reg.n = 0;

            break;
        case 0x0D://DEC C
            reg.h=(reg.C&0x0F)==0x00;
            reg.C--;
            reg.z = (reg.C == 0);
            reg.n = 0;
            break;
        case 0x0E://LD C,d8
            reg.C = read_memory(reg.PC);
            reg.PC++;
            break;
        case 0x0F://RRCA rotate A right. Old bit 0 to Carry flag
            reg.F = 0x00;
            reg.c = reg.A >> 7;
            reg.A = (reg.A >> 1) | (reg.A << 7);
            reg.z = 1 * (reg.A == 0);
            break;
        case 0x10://STOP not yet implemented
            reg.PC++;
            break;
        case 0x11://LD DE,d16
            reg.DE = read_memory16(reg.PC);
            reg.PC += 2;
            break;
        case 0x12://LD (DE),A
            write_memory(reg.DE, reg.A);
            break;
        case 0x13://INC DE
            reg.DE++;
            break;
        case 0x14://INC D
            reg.h=(reg.D&0x0F)==0x0F;
            reg.D++;
            reg.z = (reg.D == 0);
            reg.n = 0;
            break;
        case 0x15://DEC D
            reg.h=(reg.D&0x0F)==0x00;
            reg.D--;
            reg.z = (reg.B == 0);
            reg.n = 0;
            break;
        case 0x16://LD D,d8
            reg.D = read_memory(reg.PC);
            reg.PC++;
            break;
        case 0x17://RLA rotate A left through Carry flag
            reg.F = 0x00;
            reg.c = reg.A >> 7;
            temp = reg.c;
            reg.A = (reg.A << 1) | temp;
            reg.z = 1 * (reg.A == 0);
            break;
        case 0x18://JR r8
            reg.PC += read_memory(reg.PC);
            break;
        case 0x19://ADD HL,DE
        temp = reg.HL;
            reg.HL += reg.DE;
            reg.z = (reg.HL == 0);
            reg.n = 0;
            reg.h = (temp & 0xFFF) + (reg.DE & 0xFFF) > 0xFFF;
            reg.c = reg.HL >0xFFFF;
            break;
        case 0x1A://LD A,(DE)
            reg.A = read_memory(reg.DE);
            break;
        case 0x1B://DEC DE
            reg.DE--;
            break;
        case 0x1C://INC E
            reg.h=(reg.E&0x0F)==0x0F;
            reg.E++;
            reg.z = (reg.E == 0);
            reg.n = 0;
            break;
        case 0x1D://DEC E
            reg.h=(reg.E&0x0F)==0x00;
            reg.E--;
            reg.z = (reg.E == 0);
            reg.n = 0;
            break;
        case 0x1E://LD E,d8
            reg.E = read_memory(reg.PC);
            reg.PC++;
            break;
        case 0x1F://RRA rotate A right through Carry flag
            temp = reg.c;
            reg.F = 0x00;
            reg.c = reg.A >> 7;
            reg.A = (reg.A >> 1) | (temp << 7);
            reg.z = (reg.A == 0);
            break;
        case 0x20://JR NZ,r8
            if (reg.z == 0) {
                reg.PC += read_memory(reg.PC);
            } else {
                reg.PC++;
            }
            break;
        case 0x21://LD HL,d16
            reg.HL = read_memory16(reg.PC);
            reg.PC += 2;
            break;
        case 0x22://LD (HL+),A
            write_memory(reg.HL, reg.A);
            reg.HL++;
            break;
        case 0x23://INC HL
            reg.HL++;
            break;
        case 0x24://INC H
            reg.h=(reg.H&0x0F)==0x0F;
            reg.H++;
            reg.z = (reg.H == 0);
            reg.n = 0;
            break;
        case 0x25://DEC H
            reg.h=(reg.H&0x0F)==0x00;
            reg.H--;

            reg.z = (reg.H == 0);
            reg.n = 0;
            break;
        case 0x26://LD H,d8
            reg.H = read_memory(reg.PC);
            reg.PC++;
            break;
        case 0x27://DAA decimal adjust A
            if (!reg.n) {
                if (reg.h || ((reg.A & 0x0F) > 9))
                    reg.A += 0x06;
                if (reg.c || (reg.A > 0x99)) {
                    reg.A += 0x60;
                    reg.c = 1;
                }
            } else {
                if (reg.c)
                    reg.A -= 0x60;
                if (reg.h)
                    reg.A -= 0x06;
            }
            reg.z = (reg.A == 0);
            reg.h = 0;
            break;
        case 0x28://JR Z,r8
            if (reg.z)
                reg.PC =reg.PC+ read_memory(reg.PC)+1;
            else
                reg.PC++;
            break;
        case 0x29://add HL,HL
            reg.HL += reg.HL;
            reg.n = 0;
            reg.h = reg.HL >> 11;
            reg.c = reg.HL >> 15;
            break;
        case 0x2A:// LD A,(HL+)
            reg.A = read_memory(reg.HL++);
            break;
        case 0x2B://DEC DE
            reg.HL--;
            break;
        case 0x2C://INC L
            reg.h=(reg.L&0x0F)==0x0F;
            reg.L++;
            reg.z = (reg.L == 0);
            reg.n = 0;
            break;
        case 0x2D://DEC L
            reg.L--;
            reg.z = (reg.L == 0);
            reg.n = 0;
            reg.h = reg.L >> 4;
            break;
        case 0x2E://LD L,n
            reg.L = read_memory(reg.PC);
            reg.PC++;
            break;
        case 0x2F://CPL
            reg.A = ~reg.A;
            reg.n = 1;
            reg.h = 1;
            break;
        case 0x30://JR NC,e
            if (reg.c == 0) {
                reg.PC += read_memory(reg.PC);
            } else {
                reg.PC++;
            }
            break;
        case 0x31://LD SP,nn
            reg.SP = read_memory16(reg.PC);
            reg.PC += 2;
            break;
        case 0x32://LD (HL-),A
            write_memory(reg.HL--, reg.A);
            break;
        case 0x33://INC SP
            reg.SP++;
            break;
        case 0x34://INC (HL) TODO need to check this

            temp = read_memory(reg.HL);
            write_memory(reg.HL, temp + 1);
            reg.z = temp + 1 == 0;
            reg.n = 0;
            reg.h = (temp & 0x0F) == 0x0F;
            break;
        case 0x35://DEC (HL)
            temp = read_memory(reg.HL);
            write_memory(reg.HL, temp - 1);
            reg.z = temp - 1 == 0;
            reg.n = 1;
            reg.h = (temp & 0x0F) == 0x00;
            break;
        case 0x36://LD (HL),n
            write_memory(reg.HL, read_memory(reg.PC));
            reg.PC++;
            break;
        case 0x37://SCF
            reg.n = 0;
            reg.h = 0;
            reg.c = 1;
            break;
        case 0x38://JR C,e
            if (reg.c) {
                reg.PC +=(int8_t)read_memory(reg.PC++);
            } else {
                reg.PC++;
            }
            break;
        case 0x39://ADD HL,SP
            temp=reg.HL;
            reg.HL += reg.SP;
            reg.n = 0;
            reg.h = (temp & 0xFFF) + (reg.SP & 0xFFF) > 0xFFF;
            reg.c = reg.HL >0xFFFF;
            break;
        case 0x3A://LD A,(HL-)
            reg.A = read_memory(reg.HL--);
            break;
        case 0x3B://DEC SP
            reg.SP--;
            break;
        case 0x3C://INC A
            reg.A++;
            reg.z = (reg.A == 0);
            reg.n = 0;
            reg.h = ((reg.A & 0xF) < (reg.A-1 & 0xF));
            break;
        case 0x3D://DEC A
            reg.A--;
            reg.z = reg.A == 0;
            reg.n = 1;
            reg.h = ((reg.A & 0xF) > (reg.A+1 & 0xF));
            break;
        case 0x3E://LD A,n
            reg.A = read_memory(reg.PC);
            reg.PC++;
            break;
        case 0x3F://CCF
            reg.n = 0;
            reg.h = 0;
            reg.c = !reg.c;
            break;
        case 0x40 ... 0x45:case 0x47://LD B,B
            reg.B=*regs[opcode&0x7];
            break;
        case 0x46://LD B,(HL)
            reg.B = read_memory(reg.HL);
            break;
        case 0x48 ... 0x4D:case 0x4F://LD C,B ... A
            reg.C = *regs[opcode & 0x07];
            break;
        case 0x4E://LD C,(HL)
            reg.C = read_memory(reg.HL);
            break;
        case 0x50 ... 0x55:case 0x57://LD D,B .... LD D,A
            reg.D=*regs[opcode&0x07];
            break;
        case 0x56://LD D,(HL)
            reg.D = read_memory(reg.HL);
            break;
        case 0x58 ... 0x5D: case 0x5F://LD E,B ... A
            reg.E=*regs[opcode&0x07];
            break;
        case 0x5E://LD E,(HL)
            reg.E = read_memory(reg.HL);
            break;
        case 0x60 ... 0x65: case 0x67://LD H, B ... L A
                reg.H=*regs[opcode & 0x07];
                break;
        case 0x66://LD H,(HL)
            reg.H = read_memory(reg.HL);
            break;
            case 0x68 ... 0x6D:case 0x6F://LD L, B, C, D, E, H, L, A
                reg.L = *regs[opcode & 0x07];
                break;


        case 0x6E://LD L,(HL)
            reg.L = read_memory(reg.HL);
            break;

            case 0x70 ... 0x75: case 0x77://LD (HL),B
            write_memory(reg.HL, *regs[opcode & 0x07]);
            break;
        case 0x76://HALT
            //halted = true;     TODO : implement
            break;
        case 0x78 ... 0x7D:case 0x7F://LD A , B , C, D, E, H, L, A
            reg.A = *regs[opcode & 0x07];
            break;
        case 0x7E://LD A,(HL)
            reg.A = read_memory(reg.HL);
            break;
        case 0x80 ... 0x85:
        case 0x87://ADD A,B ... L ,A add to a reg b,c,d,e,h,l,a
            temp = *regs[opcode & 0x07];
            reg.A += temp;
            reg.z = (reg.A == 0);
            reg.n = 0;
            reg.h = ((reg.A & 0x0F) < (temp & 0x0F));
            reg.c = (reg.A < temp);
            break;
        case 0x86://ADD A,(HL)
            reg.A += read_memory(reg.HL);
            reg.z = (reg.A == 0);
            reg.n = 0;
            reg.h = ((reg.A & 0xF) < (read_memory(reg.HL) & 0xF));
            reg.c = (reg.A < read_memory(reg.HL));
            break;

        case 0x88 ... 0x8D:
        case 0x8F://ADC A,B ... L ,A add to a reg b,c,d,e,h,l,a with carry
            temp = *regs[opcode & 0x07];
            reg.A += temp + reg.c;
            reg.z = (reg.A == 0);
            reg.n = 0;
            reg.h = ((reg.A & 0x0F) < (temp & 0x0F) + reg.c);
            reg.c = (reg.A < temp + reg.c);
            break;
        case 0x8E://ADC A,(HL)
            reg.A += read_memory(reg.HL) + reg.c;
            reg.z = (reg.A == 0);
            reg.n = 0;
            reg.h = ((reg.A & 0xF) < (read_memory(reg.HL) & 0xF));
            reg.c = (reg.A < read_memory(reg.HL));
            break;

        case 0x90 ... 0x95:
        case 0x97://SUB B ... A subtract to A reg B,C,D,E,H,L,A
            temp = *regs[opcode & 0x07];
            reg.A -= temp;
            reg.z = (reg.A == 0);
            reg.n = 1;
            reg.h = ((reg.A & 0xF) > (temp & 0xF));
            reg.c = (reg.A > temp);
            break;
        case 0x96://SUB (HL)
            reg.A -= read_memory(reg.HL);
            temp = read_memory(reg.HL);
            reg.z = (reg.A == 0);
            reg.n = 1;
            reg.h = ((reg.A & 0xF) > (temp & 0xF));
            reg.c = (reg.A > temp);
            break;
        case 0x98 ... 0x9D:
        case 0x9F://SBC A,B ... A subtract to A reg B,C,D,E,H,L,A
            temp = *regs[opcode & 0x7];
            reg.A -= temp + reg.c;
            reg.z = (reg.A == 0);
            reg.n = 1;
            reg.h = ((reg.A & 0xF) > (temp & 0xF));
            reg.c = ((reg.A + reg.c) > temp);
            break;
        case 0x9E://SBC A,(HL)
            reg.A -= read_memory(reg.HL) + reg.c;
            temp = read_memory(reg.HL);
            reg.z = (reg.A == 0);
            reg.n = 1;
            reg.h = ((reg.A & 0xF) > (temp & 0xF));
            reg.c = ((reg.A + reg.c) > temp);
            break;
        case 0xA0 ... 0xA5:
        case 0xA7:// AND A,B ... A  and operation from B to A based on opcode
            reg.A = reg.A & *regs[opcode & 0x7];
            reg.z = (reg.A == 0);
            reg.n = 0;
            reg.h = 1;
            reg.c = 0;
            break;
        case 0xA6://AND A,(HL)
            reg.A = reg.A & read_memory(reg.HL);
            reg.z = (reg.A == 0);
            reg.n = 0;
            reg.h = 1;
            reg.c = 0;
            break;
            case 0xA8 ... 0xAD: case 0xAF://XOR A,B ... A  and operation from B to A based on opcode
            reg.A = reg.A ^ *regs[opcode & 0x7];
            reg.z = (reg.A == 0);
            reg.n = 0;
            reg.h = 0;
            reg.c = 0;
            break;
        case 0xAE://XOR A,(HL)
            reg.A = reg.A ^ read_memory(reg.HL);
            reg.z = (reg.A == 0);
            reg.n = 0;
            reg.h = 0;
            reg.c = 0;
            break;
        case 0xB0 ... 0xB5:case 0xB7://OR A,B ... A  and operation from B to A based on opcode
            reg.A = reg.A | *regs[opcode & 0x7];
            reg.z = (reg.A == 0);
            reg.n = 0;
            reg.h = 0;
            reg.c = 0;
            break;
        case 0xB6://OR A,(HL)
            reg.A = reg.A | read_memory(reg.HL);
            reg.z = (reg.A == 0);
            reg.n = 0;
            reg.h = 0;
            reg.c = 0;
            break;
        case 0xB8 ... 0xBD:case 0xBF://CP A,B ... A  and operation from B to A based on opcode
            temp = *regs[opcode & 0x7];
            reg.z = (reg.A == temp);
            reg.n = 1;
            reg.h = ((reg.A & 0xF) > (temp & 0xF));
            reg.c = (reg.A > temp);
            break;
        case 0xBE://CP A,(HL)
            temp = read_memory(reg.HL);
            reg.z = (reg.A == temp);
            reg.n = 1;
            reg.h = ((reg.A & 0xF) > (temp & 0xF));
            reg.c = (reg.A > temp);
            break;
        case 0xC0://RET NZ
            if (!reg.z)
            {
                reg.PC= read_memory16(reg.SP);
                reg.SP += 2;

            }
            break;
        case 0xC1://POP BC
            reg.C = read_memory(reg.SP + 1);
            reg.B = read_memory(reg.SP);
            reg.SP += 2;
            break;
        case 0xC2://JP NZ,nn
            if (!reg.z)
            {
                reg.PC = read_memory16(reg.PC);
            }
            else
            {
                reg.PC += 2;
            }
            break;
        case 0xC3://JP nn
            reg.PC = read_memory16(reg.PC);
            break;
        case 0xC4://CALL NZ,nn
            if (!reg.z)
            {
                reg.SP -= 2;
                write_memory16(reg.SP, reg.PC + 2);
                reg.PC = read_memory16(reg.PC);
            }
            else
            {
                reg.PC += 2;
            }
            break;
        case 0xC5://PUSH BC
            reg.SP -= 2;
            write_memory16(reg.SP, reg.BC);
            break;
        case 0xC6://ADD A,n
            reg.A += read_memory(reg.PC);
            reg.z = (reg.A == 0);
            reg.n = 0;
            reg.h = ((reg.A & 0xF) > (read_memory(reg.PC) & 0xF));
            reg.c = ((reg.A + read_memory(reg.PC)) > 0xFF);
            reg.PC++;
            break;
        case 0xC7://RST 0
            reg.SP -= 2;
            write_memory16(reg.SP, reg.PC);
            reg.PC = 0;
            break;
        case 0xC8://RET Z
            if (reg.z)
            {
                reg.PC= read_memory16(reg.SP);
                reg.SP += 2;

            }
            break;
        case 0xC9://RET
            reg.PC= read_memory16(reg.SP);
            reg.SP += 2;
            break;
        case 0xCA://JP Z,nn
            if (reg.z)
            {
                reg.PC = read_memory16(reg.PC);
            }
            else
            {
                reg.PC += 2;
            }
            break;
        case 0xCB://CB prefix
            //a switch statement for all the CB instructions
            cb_opcode = read_memory(reg.PC++);
            switch (cb_opcode)

            {
                case 0x00 ... 0x05:case 0x07://RLC B ... B  rotate left operation from B to A based on cb_opcode
                    *regs[cb_opcode & 0x7] = (*regs[cb_opcode & 0x7] << 1) | ((*regs[cb_opcode & 0x7] & 0x80) >> 7);
                    reg.z = (*regs[cb_opcode & 0x7] == 0);
                    reg.n = 0;
                    reg.h = 0;
                    reg.c = (*regs[cb_opcode & 0x7] & 0x80) >> 7;
                    break;
                case 0x06://RLC (HL)
                    write_memory( reg.HL, (read_memory(reg.HL) << 1) | ((read_memory(reg.HL) & 0x80) >> 7));
                    break;
                case 0x08 ... 0x0D:case 0x0F://RRC B ... B  rotate right  operation from B to A based on cb_opcode
                    *regs[cb_opcode & 0x7] = (*regs[cb_opcode & 0x7] >> 1) | ((*regs[cb_opcode & 0x7] & 0x01) << 7);
                    break;
                case 0x0E://RRC (HL)
                    write_memory( reg.HL, (read_memory(reg.HL) >> 1) | ((read_memory(reg.HL) & 0x01) << 7));
                    break;
                case 0x10 ... 0x15:case 0x17://RL B ... B  rotate right through carry bit operation from B to A based on cb_opcode
                    temp = (*regs[cb_opcode & 0x7] & 0x80) >> 7;
                    *regs[cb_opcode & 0x7] = (*regs[cb_opcode & 0x7] << 1) | reg.c;
                    reg.z = (*regs[cb_opcode & 0x7] == 0);
                    reg.n = 0;
                    reg.h = 0;
                    reg.c = temp;
                    break;
                case 0x16://RL (HL)
                    temp = (read_memory(reg.HL) & 0x80) >> 7;
                    write_memory( reg.HL, (read_memory(reg.HL) << 1) | reg.c);
                    reg.c = temp;
                    break;
                case 0x18 ... 0x1D:case 0x1F://RR B ... B  rotate right through carry bit from B to A based on cb_opcode
                    temp = (*regs[cb_opcode & 0x7] & 0x01);
                    *regs[cb_opcode & 0x7] = (*regs[cb_opcode & 0x7] >> 1) | (reg.c << 7);
                    reg.z = (*regs[cb_opcode & 0x7] == 0);
                    reg.n = 0;
                    reg.h = 0;
                    reg.c = temp;
                    break;
                case 0x1E://RR (HL)
                    temp = (read_memory(reg.HL) & 0x01);
                    write_memory( reg.HL, (read_memory(reg.HL) >> 1) | (reg.c << 7));
                    reg.z = (read_memory(reg.HL) == 0);
                    reg.n = 0;
                    reg.h = 0;
                    reg.c = temp;
                    break;
                case 0x20 ... 0x25:case 0x27://SLA B ... B  and operation from B to A based on cb_opcode
                    reg.c = (*regs[cb_opcode & 0x7] & 0x80) >> 7;
                    *regs[cb_opcode & 0x7] = *regs[cb_opcode & 0x7] << 1;
                    reg.z = (*regs[cb_opcode & 0x7] == 0);
                    reg.n = 0;
                    reg.h = 0;
                    break;
                case 0x26://SLA (HL)
                    reg.c = (read_memory(reg.HL) & 0x80) >> 7;
                    write_memory( reg.HL, read_memory(reg.HL) << 1);
                    reg.z = (read_memory(reg.HL) == 0);
                    reg.n = 0;
                    reg.h = 0;
                    break;
                case 0x28 ... 0x2D:case 0x2F://SRA B ... B  Shift Right Arithmetic register from B to A based on cb_opcode
                    reg.c = (*regs[cb_opcode & 0x7] & 0x01);
                    *regs[cb_opcode & 0x7] = (*regs[cb_opcode & 0x7] >> 1);
                    reg.z = (*regs[cb_opcode & 0x7] == 0);
                    reg.n = 0;
                    reg.h = 0;
                    break;
                case 0x2E://SRA (HL)
                    reg.c = (read_memory(reg.HL) & 0x01);
                    write_memory( reg.HL, read_memory(reg.HL) >> 1);
                    reg.z = (read_memory(reg.HL) == 0);
                    reg.n = 0;
                    reg.h = 0;
                    break;
                case 0x30 ... 0x35:case 0x37://SWAP B ... A  Swap upper and lower nibbles of register r8 based on cb_opcode
                    *regs[cb_opcode & 0x7] = ((temp & 0xF0) >> 4) | ((temp & 0x0F) << 4);
                    reg.z = (*regs[cb_opcode & 0x7] == 0);
                    reg.n = 0;
                    reg.h = 0;
                    reg.c = 0;
                    break;
                case 0x36://SWAP (HL)
                    temp = (read_memory(reg.HL) & 0xF0) >> 4;
                    write_memory( reg.HL, (read_memory(reg.HL) & 0x0F) << 4 | temp);
                    reg.z = (read_memory(reg.HL) == 0);
                    reg.n = 0;
                    reg.h = 0;
                    reg.c = 0;
                    break;
                case 0x38 ... 0x3D:case 0x3F://SRL B ... B  Shift Right Logical register r8 from B to A based on cb_opcode
                    reg.c = (*regs[cb_opcode & 0x7] & 0x01);
                    *regs[cb_opcode & 0x7] = *regs[cb_opcode & 0x7] >> 1;
                    reg.z = (*regs[cb_opcode & 0x7] == 0);
                    reg.n = 0;
                    reg.h = 0;
                    break;
                case 0x3E://SRL (HL)
                    reg.c = (read_memory(reg.HL) & 0x01);
                    write_memory( reg.HL, read_memory(reg.HL) >> 1);
                    reg.z = (read_memory(reg.HL) == 0);
                    reg.n = 0;
                    reg.h = 0;
                    break;
                case 0x40 ... 0x45:case 0x47://BIT 0, B ... A  Test bit 0 of register r8 based on cb_opcode
                    reg.z = ((*regs[cb_opcode & 0x7] & 0x01) == 0);
                    reg.n = 0;
                    reg.h = 1;
                    break;
                case 0x46://BIT 0, (HL)
                    reg.z = ((read_memory(reg.HL) & 0x01) == 0);
                    reg.n = 0;
                    reg.h = 1;
                    break;
                case 0x48 ... 0x4D:case 0x4F://BIT 1, B ... A  Test bit 1 of register r8 based on cb_opcode
                    reg.z = ((*regs[cb_opcode & 0x7] & 0x02) == 0);
                    reg.n = 0;
                    reg.h = 1;
                    break;
                case 0x4E://BIT 1, (HL)
                    reg.z = ((read_memory(reg.HL) & 0x02) == 0);
                    reg.n = 0;
                    reg.h = 1;
                    break;
                case 0x50 ... 0x55:case 0x57://BIT 2, B ... A  Test bit 2 of register r8 based on cb_opcode
                    reg.z = ((*regs[cb_opcode & 0x7] & 0x04) == 0);
                    reg.n = 0;
                    reg.h = 1;
                    break;
                case 0x56://BIT 2, (HL)
                    reg.z = ((read_memory(reg.HL) & 0x04) == 0);
                    reg.n = 0;
                    reg.h = 1;
                    break;
                case 0x58 ... 0x5D:case 0x5F://BIT 3, B ... A  Test bit 3 of register r8 based on cb_opcode
                    reg.z = ((*regs[cb_opcode & 0x7] & 0x08) == 0);
                    reg.n = 0;
                    reg.h = 1;
                    break;
                case 0x5E://BIT 3, (HL)
                    reg.z = ((read_memory(reg.HL) & 0x08) == 0);
                    reg.n = 0;
                    reg.h = 1;
                    break;
                case 0x60 ... 0x65:case 0x67://BIT 4, B ... A  Test bit 4 of register r8 based on cb_opcode
                    reg.z = ((*regs[cb_opcode & 0x7] & 0x10) == 0);
                    reg.n = 0;
                    reg.h = 1;
                    break;
                case 0x66://BIT 4, (HL)
                    reg.z = ((read_memory(reg.HL) & 0x10) == 0);
                    reg.n = 0;
                    reg.h = 1;
                    break;
                case 0x68 ... 0x6D:case 0x6F://BIT 5, B ... A  Test bit 5 of register r8 based on cb_opcode
                    reg.z = ((*regs[cb_opcode & 0x7] & 0x20) == 0);
                    reg.n = 0;
                    reg.h = 1;
                    break;
                case 0x6E://BIT 5, (HL)
                    reg.z = ((read_memory(reg.HL) & 0x20) == 0);
                    reg.n = 0;
                    reg.h = 1;
                    break;
                case 0x70 ... 0x75:case 0x77://BIT 6, B ... A  Test bit 6 of register r8 based on cb_opcode
                    reg.z = ((*regs[cb_opcode & 0x7] & 0x40) == 0);
                    reg.n = 0;
                    reg.h = 1;
                    break;
                case 0x76://BIT 6, (HL)
                    reg.z = ((read_memory(reg.HL) & 0x40) == 0);
                    reg.n = 0;
                    reg.h = 1;
                    break;
                case 0x78 ... 0x7D:case 0x7F://BIT 7, B ... A  Test bit 7 of register r8 based on cb_opcode
                    reg.z = ((*regs[cb_opcode & 0x7] & 0x80) == 0);
                    reg.n = 0;
                    reg.h = 1;
                    break;
                case 0x7E://BIT 7, (HL)
                    reg.z = ((read_memory(reg.HL) & 0x80) == 0);
                    reg.n = 0;
                    reg.h = 1;
                    break;
                case 0x80 ... 0x85:case 0x87://RES 0, B ... A  Reset bit 0 of register r8 based on cb_opcode
                    *regs[cb_opcode & 0x7] &= 0xFE;
                    break;
                case 0x86://RES 0, (HL)
                    write_memory(reg.HL, read_memory(reg.HL) & 0xFE);
                    break;
                case 0x88 ... 0x8D:case 0x8F://RES 1, B ... A  Reset bit 1 of register r8 based on cb_opcode
                    *regs[cb_opcode & 0x7] &= 0xFD;
                    break;
                case 0x8E://RES 1, (HL)
                    write_memory(reg.HL, read_memory(reg.HL) & 0xFD);
                    break;
                case 0x90 ... 0x95:case 0x97://RES 2, B ... A  Reset bit 2 of register r8 based on cb_opcode
                    *regs[cb_opcode & 0x7] &= 0xFB;
                    break;
                case 0x96://RES 2, (HL)
                    write_memory(reg.HL, read_memory(reg.HL) & 0xFB);
                    break;
                case 0x98 ... 0x9D:case 0x9F://RES 3, B ... A  Reset bit 3 of register r8 based on cb_opcode
                    *regs[cb_opcode & 0x7] &= 0xF7;
                    break;
                case 0x9E://RES 3, (HL)
                    write_memory(reg.HL, read_memory(reg.HL) & 0xF7);
                    break;
                case 0xA0 ... 0xA5:case 0xA7://RES 4, B ... A  Reset bit 4 of register r8 based on cb_opcode
                    *regs[cb_opcode & 0x7] &= 0xEF;
                    break;
                case 0xA6://RES 4, (HL)
                    write_memory(reg.HL, read_memory(reg.HL) & 0xEF);
                    break;
                case 0xA8 ... 0xAD:case 0xAF://RES 5, B ... A  Reset bit 5 of register r8 based on cb_opcode
                    *regs[cb_opcode & 0x7] &= 0xDF;
                    break;
                case 0xAE://RES 5, (HL)
                    write_memory(reg.HL, read_memory(reg.HL) & 0xDF);
                    break;
                case 0xB0 ... 0xB5:case 0xB7://RES 6, B ... A  Reset bit 6 of register r8 based on cb_opcode
                    *regs[cb_opcode & 0x7] &= 0xBF;
                    break;
                case 0xB6://RES 6, (HL)
                    write_memory(reg.HL, read_memory(reg.HL) & 0xBF);
                    break;
                case 0xB8 ... 0xBD:case 0xBF://RES 7, B ... A  Reset bit 7 of register r8 based on cb_opcode
                    *regs[cb_opcode & 0x7] &= 0x7F;
                    break;
                case 0xBE://RES 7, (HL)
                    write_memory(reg.HL, read_memory(reg.HL) & 0x7F);
                    break;
                case 0xC0 ... 0xC5:case 0xC7://SET 0, B ... A  Set bit 0 of register r8 based on cb_opcode
                    *regs[cb_opcode & 0x7] |= 0x01;
                    break;
                case 0xC6://SET 0, (HL)
                    write_memory(reg.HL, read_memory(reg.HL) | 0x01);
                    break;
                case 0xC8 ... 0xCD:case 0xCF://SET 1, B ... A  Set bit 1 of register r8 based on cb_opcode
                    *regs[cb_opcode & 0x7] |= 0x02;
                    break;
                case 0xCE://SET 1, (HL)
                    write_memory(reg.HL, read_memory(reg.HL) | 0x02);
                    break;
                case 0xD0 ... 0xD5:case 0xD7://SET 2, B ... A  Set bit 2 of register r8 based on cb_opcode
                    *regs[cb_opcode & 0x7] |= 0x04;
                    break;
                case 0xD6://SET 2, (HL)
                    write_memory(reg.HL, read_memory(reg.HL) | 0x04);
                    break;
                case 0xD8 ... 0xDD:case 0xDF://SET 3, B ... A  Set bit 3 of register r8 based on cb_opcode
                    *regs[cb_opcode & 0x7] |= 0x08;
                    break;
                case 0xDE://SET 3, (HL)
                    write_memory(reg.HL, read_memory(reg.HL) | 0x08);
                    break;
                case 0xE0 ... 0xE5:case 0xE7://SET 4, B ... A  Set bit 4 of register r8 based on cb_opcode
                    *regs[cb_opcode & 0x7] |= 0x10;
                    break;
                case 0xE6://SET 4, (HL)
                    write_memory(reg.HL, read_memory(reg.HL) | 0x10);
                    break;
                case 0xE8 ... 0xED:case 0xEF://SET 5, B ... A  Set bit 5 of register r8 based on cb_opcode
                    *regs[cb_opcode & 0x7] |= 0x20;
                    break;
                case 0xEE://SET 5, (HL)
                    write_memory(reg.HL, read_memory(reg.HL) | 0x20);
                    break;
                case 0xF0 ... 0xF5:case 0xF7://SET 6, B ... A  Set bit 6 of register r8 based on cb_opcode
                    *regs[cb_opcode & 0x7] |= 0x40;
                    break;
                case 0xF6://SET 6, (HL)
                    write_memory(reg.HL, read_memory(reg.HL) | 0x40);
                    break;
                case 0xF8 ... 0xFD:case 0xFF://SET 7, B ... A  Set bit 7 of register r8 based on cb_opcode
                    *regs[cb_opcode & 0x7] |= 0x80;
                    break;
                case 0xFE://SET 7, (HL)
                    write_memory(reg.HL, read_memory(reg.HL) | 0x80);
                    break;
                default:


                    printf("Unknown opcode: %02X %02X at PC %04X", opcode,cb_opcode, reg.PC);
                    wannadie = 1;
                    break;

            }
            break;
        case 0xCC://CALL Z, nn TODO implement
            reg.PC += 2;
            if (reg.z) {
                reg.SP -= 2;
                write_memory16(reg.SP, reg.PC);
                reg.PC = read_memory16(reg.PC);
            }
            break;
        case 0xCD://CALL nn
            reg.PC += 2;
            reg.SP -= 2;
            write_memory16(reg.SP, reg.PC);
            reg.PC = read_memory16(reg.PC);
            break;
        case 0xCE://ADC A, n
            reg.A += read_memory(reg.PC) + reg.c;
            reg.PC++;
            reg.z = (reg.A == 0);
            reg.n = 0;
            reg.h = ((reg.A & 0xF) > (read_memory16(reg.PC-1) & 0xF));
            reg.c = (reg.A > read_memory16(reg.PC-1));
            break;
        case 0xCF://RST 8
            reg.PC += 2;
            reg.SP -= 2;
            write_memory16(reg.SP, reg.PC);
            reg.PC = 0x08;
            break;
        case 0xD0://RET NC
            if (!reg.c) {
                reg.PC = read_memory16(reg.SP);
                reg.SP += 2;
            }
            break;
        case 0xD1://POP DE
            reg.DE = read_memory16(reg.SP);
            reg.SP += 2;
            break;
        case 0xD2://JP NC, nn
            if (!reg.c) {
                reg.PC = read_memory16(reg.PC);
            } else {
                reg.PC += 2;
            }
            break;
        case 0xD3://
            break;
        case 0xD4://CALL NC, nn TODO test
            if (!reg.c) {
                reg.PC += 2;
                reg.SP -= 2;
                write_memory16(reg.SP, reg.PC);
                reg.PC = read_memory16(reg.PC-2);
            } else {
                reg.PC += 2;
            }
            break;
        case 0xD5://PUSH DE
            reg.SP -= 2;
            write_memory16(reg.SP, reg.DE);
            break;
        case 0xD6://SUB n
            reg.A -= read_memory(reg.PC);
            reg.PC++;
            reg.z = (reg.A == 0);
            reg.n = 1;
            reg.h = ((reg.A & 0xF) < (read_memory16(reg.PC-1) & 0xF));
            reg.c = (reg.A < read_memory16(reg.PC-1));
            break;
        case 0xD7://RST 10
            reg.PC += 2;
            reg.SP -= 2;
            write_memory16(reg.SP, reg.PC);
            reg.PC = 0x10;
            break;
        case 0xD8://RET C
            if (reg.c) {
                reg.PC = read_memory16(reg.SP);
                reg.SP += 2;
            }
            break;
        case 0xD9://RETI
            reg.PC = read_memory16(reg.SP);
            reg.SP += 2;
            //set ime
            reg.ime = 1;
            break;
        case 0xDA://JP C, nn
            if (reg.c) {
                reg.PC = read_memory16(reg.PC);
            } else {
                reg.PC += 2;
            }
            break;
        case 0xDB://
            break;
        case 0xDC://CALL C, nn
            if (reg.c) {
                reg.SP -= 2;
                write_memory16(reg.SP, reg.PC+2);
                reg.PC = read_memory16(reg.PC);
            } else {
                reg.PC += 2;
            }
            break;
        case 0xDD://
            break;
        case 0xDE://SBC A, n
            reg.A -= read_memory(reg.PC) + reg.c;
            reg.PC++;
            reg.z = (reg.A == 0);
            reg.n = 1;
            reg.h = ((reg.A & 0xF) < (read_memory16(reg.PC-1) & 0xF));
            reg.c = (reg.A < read_memory16(reg.PC-1));
            break;
        case 0xDF://RST 18
            reg.PC += 2;
            reg.SP -= 2;
            write_memory16(reg.SP, reg.PC);
            reg.PC = 0x18;
            break;
        case 0xE0://LDH (n), A
            write_memory(read_memory(reg.PC++) + 0xFF00, reg.A);
            break;
        case 0xE1://POP HL
            reg.HL = read_memory16(reg.SP);
            reg.SP += 2;
            break;
        case 0xE2://LDH (C), A
            write_memory(reg.C + 0xFF00, reg.A);
            break;
        case 0xE3://
            break;
        case 0xE4://
            break;
        case 0xE5://PUSH HL
            reg.SP -= 2;
            write_memory16(reg.SP, reg.HL);
            break;
        case 0xE6://AND n
            reg.A &= read_memory(reg.PC);
            reg.PC++;
            reg.z = (reg.A == 0);
            reg.n = 0;
            reg.h = 1;
            reg.c = 0;
            break;
        case 0xE7://RST 20
            reg.PC += 2;
            reg.SP -= 2;
            write_memory16(reg.SP, reg.PC);
            reg.PC = 0x20;
            break;
        case 0xE8://ADD SP, n
            reg.SP += read_memory(reg.PC++);
            reg.z = 0;
            reg.n = 0;

            reg.h = ((reg.SP & 0xFFFF) + (read_memory(reg.PC-1) & 0xFFFF) > 0xFFFF);
            reg.c = reg.SP > 0xFFFF;
            break;
        case 0xE9://JP (HL)
            reg.PC = reg.HL;
            break;
        case 0xEA://LD (nn), A
            write_memory(read_memory16(reg.PC), reg.A);
            reg.PC += 2;
            break;
        case 0xEB://
            break;
        case 0xEC://
            break;
        case 0xED://
            break;
        case 0xEE://XOR n
            reg.A ^= read_memory(reg.PC);
            reg.PC++;
            reg.z = (reg.A == 0);
            reg.n = 0;
            reg.h = 0;
            reg.c = 0;
            break;
        case 0xEF://RST 28
            reg.PC += 2;
            reg.SP -= 2;
            write_memory16(reg.SP, reg.PC);
            reg.PC = 0x28;
            break;
        case 0xF0://LDH A, (n)
            reg.A = read_memory(0xFF00 + read_memory(reg.PC));
            reg.PC++;
            break;
        case 0xF1://POP AF
            reg.AF = read_memory16(reg.SP);
            reg.SP += 2;
            break;
        case 0xF2://LDH A, (C)
            reg.A = read_memory(0xFF00 + reg.C);
            break;

        case 0xF3://DI
        //clear ime from memory yet to implement interupts
            reg.ime = 0;
            break;
        case 0xF4://
            break;
        case 0xF5://PUSH AF
            reg.SP -= 2;
            write_memory16(reg.SP, reg.AF);
            break;
        case 0xF6://OR n
            reg.A |= read_memory(reg.PC);
            reg.PC++;
            reg.z = (reg.A == 0);
            reg.n = 0;
            reg.h = 0;
            reg.c = 0;
            break;
        case 0xF7://RST 30
            reg.PC += 2;
            reg.SP -= 2;
            write_memory16(reg.SP, reg.PC);
            reg.PC = 0x30;
            break;
        case 0xF8://LD HL, SP+n
            reg.HL = reg.SP + read_memory(reg.PC++);
            reg.z = 0;
            reg.n = 0;
            reg.h = ((reg.HL & 0xFFFF) + (read_memory(reg.PC-1) & 0xFFFF) > 0xFFFF);
            reg.c = reg.HL > 0xFFFF;
            break;
        case 0xF9://LD SP, HL
            reg.SP = reg.HL;
            break;
        case 0xFA://LD A, (nn)
            reg.A = read_memory(read_memory16(reg.PC));
            reg.PC += 2;
            break;
        case 0xFB://EI
            reg.ime = 1;
            break;
        case 0xFC://
            break;
        case 0xFD://
            break;
        case 0xFE://CP n
            temp=reg.A-read_memory(reg.PC);
            reg.PC++;
            reg.z = (temp == 0);
            reg.n = 1;
            reg.h =( (int)reg.A- read_memory(reg.PC-1))<0;
            reg.c = (temp < read_memory(reg.PC-1));
            break;
        case 0xFF://RST 38
            reg.PC += 2;
            reg.SP -= 2;
            write_memory16(reg.SP, reg.PC);
            reg.PC = 0x38;
            break;








            //TODO check flag on inc and dec add and sub
        default:
            printf("Unknown opcode: %02X\nat PC %04X", opcode, reg.PC-1);
            wannadie = 1;
            break;
    }
}


//get tiles from memory
void get_tiles(tile *tiles) {
    int i;
    uint16_t temp;
    for (i = 0; i < 0x1000; i++) {
        temp = read_memory16(0x8000 + i);
        for(int j = 0; j < 8; j++) {
            for (int k = 0; k < 8; k++) {
                tiles[i].data[j][k] = (temp & (1 << (7-(k+j)))) >> (7-(k+j));
            }
        }
    }
}

//create a window with sdl2 to display a string until closed update string with display_string_mutex




//create a thread to run a display_string function
void display_string_thread(char *string) {

}
//function to update the string to display
void update_display_string(char *string) {
    strcpy(display_string_string, string);
}






void create_window() {
    char  *i;
    char ti[0x1000];
    if (TTF_Init() < 0) {
        // Error handling code
    }
    SDL_Window *window = SDL_CreateWindow("Gameboy", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 256, 256, 0);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, 0);
    SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, 256, 256);
    SDL_Event event;
    SDL_Window *window2 = SDL_CreateWindow("",   SDL_WINDOWPOS_UNDEFINED,SDL_WINDOWPOS_UNDEFINED,    1280,      120,  0);
    //set display string
    //create renderer
    SDL_Renderer *renderer2 = SDL_CreateRenderer(window2, -1, 0);
    TTF_Font* Sans = TTF_OpenFont("Arial.ttf", 200);
    SDL_Color White = {255, 255, 255};
    SDL_Surface* surfaceMessage2 = TTF_RenderText_Solid(Sans, regop_to_string(), White);
    SDL_Texture* Message2 = SDL_CreateTextureFromSurface(renderer2, surfaceMessage2);
    SDL_Rect Message_rect;
    Message_rect.x = 0;
    Message_rect.y = 0;
    Message_rect.w = 1280;
    Message_rect.h = 120;
    SDL_RenderCopy(renderer2, Message2, NULL, &Message_rect);
    SDL_RenderPresent(renderer2);
    int running = 1;
    int counter = 0;
    uint8_t *pixels = static_cast<uint8_t *>(malloc(1024 * 1024 * 4));
    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = 0;
            }
        }
        if(wannadie)
        {
            running = 0;
        }
        SDL_Event event;
        //update string with mutex
        SDL_FreeSurface(surfaceMessage2);
        SDL_DestroyTexture(Message2);
        SDL_DestroyRenderer(renderer2);
        renderer2 =SDL_CreateRenderer(window2, -1, 0);
        surfaceMessage2 = TTF_RenderText_Solid(Sans, regop_to_string(), White);
        Message2 = SDL_CreateTextureFromSurface(renderer2, surfaceMessage2);
        SDL_RenderCopy(renderer2, Message2, NULL, &Message_rect);
        SDL_UpdateWindowSurface(window2);

        SDL_RenderPresent(renderer2);
        //SDL_FreeSurface(surfaceMessage2);
        //SDL_DestroyTexture(Message2);
        //update_display_string("hello");
        //update_display_string(reg_to_string());
        printf("PC: %04X opcode %02X\n", reg.PC, read_memory(reg.PC));
        cpu_step(read_memory(reg.PC));
        counter++;
        //request user input if pc > breakpoint
        if (reg.PC > breakpoint) {
            printf("Press enter to continue\n");
            fgets(ti, 0x1000, stdin);
        }
        //print 0xFF44
        printf("%02X\n", read_memory(0xFF44));
        if(counter>=100){
            memcpy(ti,&mem.memory[0x8000],0x1000);
            tile *tiles=static_cast<tile *>(malloc(0x1000 * sizeof(tile)));
    get_tiles(tiles);
    for(int i=0;i<8;i++){
        for(int j=0;j<8;j++){
            for(int k=0;k<8;k++){
                for(int l=0;l<8;l++){
                    pixels[(i*8+k)*256+j*8+l]=tiles[i+j].data[k][l];
                }
            }
        }
    }
        free(tiles);

        SDL_UpdateTexture(texture, NULL, pixels, 256);
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);
        counter=0;
        }

    }
    free(pixels);
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}
//flag to string



char * reg_to_string() {
    char *string = static_cast<char *>(malloc(0x1000));
    sprintf(string, "AF: %04X BC: %04X DE: %04X HL: %04X SP: %04X PC: %04X  z:%d c:%d n:%d h:%d", reg.AF, reg.BC, reg.DE, reg.HL, reg.SP, reg.PC, reg.z, reg.c, reg.n, reg.h);
    return string;
}
//return reg and opcode to string
char *regop_to_string() {
    char *string = static_cast<char *>(malloc(0x1000));
    sprintf(string, "opcode %02X ", read_memory(reg.PC));
    sprintf(string, "%s%s", string, reg_to_string());
    return string;
}



//init all
void init() {
    SDL_Init(SDL_INIT_VIDEO);
    mem_init();
    reg_init();
    load_rom();
    load_header();
    print_cartridge_header();
    display_string_thread(regop_to_string());
    display_string_string = static_cast<char *>(malloc(sizeof(regop_to_string())));

}

//run create window in a thread
void run() {
//create with pthread
create_window();
}


int main(int argv, char** args) {
    scanf("%X",&breakpoint);
    init();
    run();


    return 0;
}
