// *** Hardwarespecific defines ***


#define cbi(reg, bitmask) *reg &= ~bitmask
#define sbi(reg, bitmask) *reg |= bitmask
#define pulse_high(reg, bitmask) sbi(reg, bitmask); cbi(reg, bitmask);
#define pulse_low(reg, bitmask) sbi(reg, bitmask); sbi(reg, bitmask);

#define pulse_WR() 	REG_PIOD_CODR=0x2;	REG_PIOD_SODR=0x2;
#define cbi_RS() REG_PIOD_CODR=0x1;
#define sbi_RS() REG_PIOD_SODR=0x1;
#define cbi_CS() REG_PIOD_CODR=0x4;
#define sbi_CS() REG_PIOD_SODR=0x4;
#define cbi_RST() REG_PIOD_CODR=0x8;
#define sbi_RST() REG_PIOD_SODR=0x8;

#define pgm_read_word(data) *data
#define pgm_read_byte(data) *data

#define cport(port, data) port &= data
#define sport(port, data) port |= data

#define swap(type, i, j) {type t = i; i = j; j = t;}

#define fontbyte(x) cfont.font[x]  

#define regtype volatile uint32_t
#define regsize uint32_t
#define bitmapdatatype unsigned short*
