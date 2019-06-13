#include <exception>
#include <stdexcept>
#include "flash.h"

using namespace flash;

struct manufacturer_s manufacturers[] = {
	/*
		ID    - Manufacturer ID
		Name  - Manufacturer name
		WREN  - WRite ENable
		EWSR  - Enable-Write-Status-Reg
		READ  - Read Data Bytes
		FREAD - Fast Read
		PRGR  - Page Program
		RDSR  - Read Status Register
		CHER  - Chip Erase
		
	  ID   Name         WREN  EWSR  READ FREAD  PRGR  RDSR  CHER*/
	{0x20, "ST",        0x06,   -1, 0x03,   -1, 0x02, 0x05,   -1}, /* Based on M25P05 datasheet */
	{0xef, "Winbond",   0x06, 0x50, 0x03, 0x0b, 0x02, 0x05, 0xc7},
	{0xc2, "Macronix",  0x06, 0x50, 0x03, 0x0b, 0x02, 0x05,   -1},
	{0x1f, "Atmel",     0x06,   -1, 0x03, 0x0b, 0x02, 0x05, 0x60}, /* Based on AT25DF041A datasheet */
	{0xbf, "Microchip", 0x06, 0x50, 0x03, 0x0b, 0x02, 0x05,   -1}, /* Based on SST25LF020A datasheet */
	{0x00, "Unknown",     -1,   -1,   -1,   -1,   -1,   -1,   -1}
};

struct desc_s descriptions[] = {
	/*
	NAME              JEDEC ID     SIZE KB      PAGE    BLOCKSIZE KB   WREN  EWSR  READ FREAD  PRGR  RDSR  CHER */
	{"AT25DF041A",    0x1F4401,         512,    256,    64,            -1,   -1,   -1,   -1,   -1,   -1,   -1},
	{"AT25DF161" ,    0x1F4602,    2 * 1024,    256,    64,            -1,   -1,   -1,   -1,   -1,   -1,   -1},
	{"AT26DF081A",    0x1F4501,    1 * 1024,    256,    64,            -1,   -1,   -1,   -1,   -1,   -1,   -1},
	{"AT26DF0161",    0x1F4600,    2 * 1024,    256,    64,            -1,   -1,   -1,   -1,   -1,   -1,   -1},
	{"AT26DF161A",    0x1F4601,    2 * 1024,    256,    64,            -1,   -1,   -1,   -1,   -1,   -1,   -1},
	{"AT25DF321",     0x1F4701,    4 * 1024,    256,    64,            -1,   -1,   -1,   -1,   -1,   -1,   -1},
	{"AT25DF512B",    0x1F6501,          64,    256,    32,            -1,   -1,   -1,   -1,   -1,   -1,   -1},
	{"AT25DF512B",    0x1F6500,          64,    256,    32,            -1,   -1,   -1,   -1,   -1,   -1,   -1},
	{"AT25DF021",     0x1F3200,         256,    256,    64,            -1,   -1,   -1,   -1,   -1,   -1,   -1},
	{"AT26DF641",     0x1F4800,    8 * 1024,    256,    64,            -1,   -1,   -1,   -1,   -1,   -1,   -1},
	{"M25P05",        0x202010,          64,    256,    32,            -1,   -1,   -1,   -1,   -1,   -1,   -1},
	{"M25P10",        0x202011,         128,    256,    32,            -1,   -1,   -1,   -1,   -1,   -1,   -1},
	{"M25P20",        0x202012,         256,    256,    64,            -1,   -1,   -1,   -1,   -1,   -1,   -1},
	{"M25P40",        0x202013,         512,    256,    64,            -1,   -1,   -1,   -1,   -1,   -1,   -1},
	{"M25P80",        0x202014,    1 * 1024,    256,    64,            -1,   -1,   -1,   -1,   -1,   -1,   -1},
	{"M25P16",        0x202015,    2 * 1024,    256,    64,            -1,   -1,   -1,   -1,   -1,   -1,   -1},
	{"M25P32",        0x202016,    4 * 1024,    256,    64,            -1,   -1,   -1,   -1,   -1,   -1,   -1},
	{"M25P64",        0x202017,    8 * 1024,    256,    64,            -1,   -1,   -1,   -1,   -1,   -1,   -1},
	{"W25X10",        0xEF3011,         128,    256,    64,            -1,   -1,   -1,   -1,   -1,   -1,   -1},
	{"W25X20",        0xEF3012,         256,    256,    64,            -1,   -1,   -1,   -1,   -1,   -1,   -1},
	{"W25X40",        0xEF3013,         512,    256,    64,            -1,   -1,   -1,   -1,   -1,   -1,   -1},
	{"W25X80",        0xEF3014,    1 * 1024,    256,    64,            -1,   -1,   -1,   -1,   -1,   -1,   -1},
	{"MX25L512",      0xC22010,          64,    256,    64,            -1,   -1,   -1,   -1,   -1,   -1,   -1},
	{"MX25L3205",     0xC22016,    4 * 1024,    256,    64,            -1,   -1,   -1,   -1,   -1,   -1,   -1},
	{"MX25L6405",     0xC22017,    8 * 1024,    256,    64,            -1,   -1,   -1,   -1,   -1,   -1,   -1},
	{"MX25L8005",     0xC22014,        1024,    256,    64,            -1,   -1,   -1,   -1,   -1,   -1,   -1},
	{"MX25L4005",     0xC22013,        1024,    256,    64,            -1,   -1,   -1,   -1,   -1,   -1,   -1},
	{"SST25VF512",    0xBF4800,          64,    256,    32,            -1,   -1,   -1,   -1,   -1,   -1,   -1},
	{"SST25VF032",    0xBF4A00,    4 * 1024,    256,    32,            -1,   -1,   -1,   -1,   -1,   -1,   -1},
	{NULL, 0, 0, 0, 0}
};

device::device(uint32_t jedecId) {
	int i = 0;
	while (1) {
		if (descriptions[i].jedecId == 0) throw std::runtime_error("Unknown flash device type");
		if (descriptions[i].jedecId == jedecId) {
			this->desc = &descriptions[i];
			break;
		}
		i++;
	}

	this->jedecId = jedecId;
	this->manufacturerId = this->jedecId >> 16;

	i = 0;
	while(1) {
		if (manufacturers[i].id == 0x00 | manufacturers[i].id == this->manufacturerId) {
			this->manufacturer = &manufacturers[i];
			break;
		}
		i++;
	}
}

device::~device() {
}
