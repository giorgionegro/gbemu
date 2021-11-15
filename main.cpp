#include <cstdint>
#include <cstdio>
#include <cstring>

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
                    uint8_t z: 1;
                    uint8_t n: 1;
                    uint8_t h: 1;
                    uint8_t c: 1;
                    uint8_t unused: 4;

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
//memory write

//memory read
uint8_t read_memory(uint16_t address) {
    if (address < 0x8000 && address >= 0x4000) {
        return r.bank[address];
    }
    return mem.memory[address];
}

//mem read 16bit
uint16_t read_memory16(uint16_t address) {
    if (address < 0x8000 && address >= 0x4000) {
        return r.bank[address] | (r.bank[address + 1] << 8);
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

//init all
void init() {
    mem_init();
    reg_init();
    load_rom();
    load_header();
    print_cartridge_header();


}

void cpu_step(uint8_t opcode) {
    reg.PC++;
    uint8_t temp, cb_opcode;
    unsigned int adjustment = 0;
    uint8_t * regs[] = {&reg.B, &reg.C, &reg.D, &reg.E, &reg.H, &reg.L,nullptr, &reg.A};


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
                reg.PC += read_memory(reg.PC);
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
                reg.PC += read_memory(reg.PC);
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
            reg.C = *regs[opcode & 0x0F];
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
            reg.E=*regs[opcode&0x0F];
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
            reg.A = reg.A & *regs[opcode & 0xF];
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
            reg.A = reg.A ^ *regs[opcode & 0xF];
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
            reg.A = reg.A | *regs[opcode & 0xF];
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
            temp = *regs[opcode & 0xF];
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
            reg.PC++;
            //a switch statement for all the CB instructions
            cb_opcode = read_memory(reg.PC);
            switch (cb_opcode)

            {
                case 0x00 ... 0x05:case 0x07://RLC B ... B  and operation from B to A based on cb_opcode
                    *regs[cb_opcode & 0xF] = (*regs[cb_opcode & 0xF] << 1) | ((*regs[cb_opcode & 0xF] & 0x80) >> 7);
                    reg.z = (*regs[cb_opcode & 0xF] == 0);
                    reg.n = 0;
                    reg.h = 0;
                    reg.c = (*regs[cb_opcode & 0xF] & 0x80) >> 7;
                    break;
                case 0x06://RLC (HL)
                    write_memory( reg.HL, (read_memory(reg.HL) << 1) | ((read_memory(reg.HL) & 0x80) >> 7));
                    break;
                case 0x08 ... 0x0D:case 0x0F://RRC B ... B  and operation from B to A based on cb_opcode
                    *regs[cb_opcode & 0xF] = (*regs[cb_opcode & 0xF] >> 1) | ((*regs[cb_opcode & 0xF] & 0x01) << 7);
                    break;
                case 0x0E://RRC (HL)
                    write_memory( reg.HL, (read_memory(reg.HL) >> 1) | ((read_memory(reg.HL) & 0x01) << 7));
                    break;
                case 0x10 ... 0x15:case 0x17://RL B ... B  and operation from B to A based on cb_opcode
                    temp = (*regs[cb_opcode & 0xF] & 0x80) >> 7;
                    *regs[cb_opcode & 0xF] = (*regs[cb_opcode & 0xF] << 1) | reg.c;
                    reg.z = (*regs[cb_opcode & 0xF] == 0);
                    reg.n = 0;
                    reg.h = 0;
                    reg.c = temp;
                    break;
                case 0x16://RL (HL)
                    temp = (read_memory(reg.HL) & 0x80) >> 7;
                    write_memory( reg.HL, (read_memory(reg.HL) << 1) | reg.c);
                    reg.c = temp;
                    break;
                case 0x18 ... 0x1D:case 0x1F://RR B ... B  and operation from B to A based on cb_opcode
                    temp = (*regs[cb_opcode & 0xF] & 0x01);
                    *regs[cb_opcode & 0xF] = (*regs[cb_opcode & 0xF] >> 1) | (reg.c << 7);
                    reg.z = (*regs[cb_opcode & 0xF] == 0);
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
                    reg.c = (*regs[cb_opcode & 0xF] & 0x80) >> 7;
                    *regs[cb_opcode & 0xF] = *regs[cb_opcode & 0xF] << 1;
                    reg.z = (*regs[cb_opcode & 0xF] == 0);
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

                default:


                    printf("Unknown opcode: %02X %02X at PC %04X", opcode,cb_opcode, reg.PC);
                    wannadie = 1;
                    break;

            }
            break;
        case 0xCC://CALL Z, nn TODO implement



            //TODO check flag on inc and dec add and sub
        default:


            printf("Unknown opcode: %02X\nat PC %04X", opcode, reg.PC);
            wannadie = 1;
            break;
    }
}


int main() {
    init();
    while (true) {
        cpu_step(read_memory(reg.PC));\
        if (wannadie)
            break;
    }


    return 0;
}
