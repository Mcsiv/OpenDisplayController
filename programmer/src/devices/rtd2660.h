#pragma once

#include "device.h"

namespace devices {

	namespace RTD2660 {

		enum registers {
			common_inst_en               = 0x60,
			common_op_code               = 0x61,
			wren_op_code                 = 0x62,
			ewsr_op_code                 = 0x63,
			flash_prog_isp0              = 0x64,
			flash_prog_isp1              = 0x65,
			flash_prog_isp2              = 0x66,
			common_inst_read_port0       = 0x67,
			common_inst_read_port1       = 0x68,
			common_inst_read_port2       = 0x69,
			read_op_code                 = 0x6a,
			fast_read_op_code            = 0x6b,
			program_op_code              = 0x6d,
			read_status_register_op_code = 0x6e,
			program_instruction          = 0x6f,
			program_data_port            = 0x70,
			program_length               = 0x71,
			CRC_end_addr0                = 0x72,
			CRC_end_addr1                = 0x73,
			CRC_end_addr2                = 0x74,
			CRC_result                   = 0x75
		};

		// Definitions: bf - bitfields / v - values

		// registers:: common_inst_en

		enum bf_common_inst_en {
			comm_inst_en = 0,		// R/W | 0:0 | 0 | Common instruction enable
			read_num     = 1,		// R/W | 2:1 | 0 | Common instruction read number
			write_num    = 3,		// R/W | 4:3 | 0 | Common instruction write number
			comm_inst    = 5,		// R/W | 7:5 | 0 | 000: No operation / 001: write / 010: read / 011: write after WREN / 100: write after EWSR / 101: Erase
		};

		enum v_comm_inst {
			noop             = 0b000,
			write            = 0b001,
			read             = 0b010,
			write_after_WREN = 0b011,
			write_after_EWSR = 0b100,
			erase            = 0b101
		};

		// registers::program_op_code

		enum v_com_op {
			WREN = 0x06,
			WRDI = 0x04,
			EWSR = 0x50,
			DP   = 0xB9,
			RDP  = 0xAB
		};

		// registers::program_instruction

		enum bf_program_instruction {
			rst_flash_ctrl = 0,		// R/W | 0:0 | 0 | 0: Disable / 1: Software reset flash controller
			crc_done       = 1,		// R   | 1:1 | 0 | When CRC is done, and will return to 0 when CRC_START is set.
			crc_start      = 2,		// R/W | 2:2 | 0 | When write one, read data from PROG_ST_ADDR to PROG_END_ADDR. And at the same time the CRC is calculated by IC automatically. This bit will be auto cleared when crc_done is 1.(Can be trigger in ISP Mode only) 
			prog_dummy     = 3,		// R/W | 3:3 | 0 | Dummy
			prog_buf_wr_en = 4,		// R   | 4:4 | 0 | 0: can’t write data to sram / 1: can write data to sram 
			prog_en        = 5,		// R/W | 5:5 | 0 | 0: finish / 1: program on-going (write 1 to start, auto clear when finish)
			prog_mode      = 6,		// R/W | 6:6 | 0 | 0: normal mode / 1: AAI mode
			isp_en         = 7		// R/W | 7:7 | 0 | enable ISP program : all registers except this register can’t write/read when ISP_ENABLE=0 | 0: disable / 1: enable (gating 8051 clock)
		};

	};

	class rtd2660: public device {
		private:
			flash::device *flash;
			void setupFlashOpCodes();

		public:
			rtd2660(i2c::connection *connection);
			~rtd2660();

			virtual void enterISPMode();
			virtual bool isInISPMode();
			virtual void exitISPMode();

			virtual uint8_t calculateCRC(uint32_t startAddress, uint32_t endAddress);

			virtual void SPI_waitProgOperation();
			virtual void SPI_waitOperation();
			virtual uint32_t SPI_commonCommand(RTD2660::v_comm_inst type, uint8_t opCode, uint8_t readNum, uint8_t writeNum, uint32_t writeValue);
			virtual size_t SPI_read(uint32_t address, uint8_t *data, size_t bufferSize);

			virtual uint32_t getFlashJedecID();
			void setFlashDevice(flash::device *flash);

			virtual size_t readFlashContent(uint8_t *buffer, uint32_t startAddress, size_t size);
			virtual void writeFlashContent(uint8_t *buffer, uint32_t startAddress, size_t size);

	};

};