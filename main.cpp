

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


}lcdc_reg;

//LCD Status
#define STAT 0xFF41
typedef struct stat_reg{
   union {
       struct{
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

}stat_reg;
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
                struct{
                    uint8_t  zero: 1;
                    uint8_t n: 1;
                    uint8_t h: 1;
                    uint8_t c: 1;
                    uint8_t unused: 4;

                }flag;

                char F;
            };
            char A;
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
    uint8_t memory[0x10000];
} memory;

//rom
typedef struct rom {
    char buffer[0x8000];
} rom;

memory mem;

//memory write
void write_memory(uint16_t address, uint8_t value){
    mem.memory[address] = value;
}
//memory read
uint8_t read_memory(uint16_t address){
    return mem.memory[address];
}

//mem read 16bit
uint16_t read_memory16(uint16_t address){
    return mem.memory[address] | (mem.memory[address+1] << 8);
}
//mem write 16bit
void write_memory16(uint16_t address, uint16_t value){
    mem.memory[address] = value & 0xFF;
    mem.memory[address+1] = (value >> 8) & 0xFF;
}


//register declaration
registers reg;
//mem init
void mem_init(){
    for(int i = 0; i < 0x10000; i++){
        mem.memory[i] = 0;
    }
}
//reg init
void reg_init(){
    reg.AF = 0;
    reg.BC = 0;
    reg.DE = 0;
    reg.HL = 0;
    reg.SP = 0;
    reg.PC = 0;
}
//declare rom
rom r;

//load rom From rom.gb
void load_rom(){
    FILE *rom_file;
    rom_file = fopen("rom.gb", "rw");
    fread(r.buffer , 0x8000, 1, rom_file);
    fclose(rom_file);
}
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
//load cartridge header from rom r to header starting from 0x100
void load_header(){
    for(int i = 0; i < 48; i++){
        header.logo[i] = r.buffer[i+0x100];
    }
    for(int i = 0; i < 16; i++){
        header.title[i] = r.buffer[i+0x134];
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
void print_cartridge_header(){
    printf("Title: %s\n", header.title);
    printf("Type: %d\n", header.type);
    printf("ROM Size: %d\n", header.rom_size);
    printf("RAM Size: %d\n", header.ram_size);
    printf("Destination Code: %d\n", header.destination_code);
    printf("Old License Code: %d\n", header.old_license_code);
    printf("Mask ROM Version: %d\n", header.mask_rom_version);
    printf("Header Checksum: %d\n", header.header_checksum);
    printf("Global Checksum: %d\n", header.global_checksum);
}
//read rom header and print info



int main(){
    load_rom();
    load_header();
    print_cartridge_header();
    return 0;
}
