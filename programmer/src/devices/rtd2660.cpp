#include <plog/Log.h>
#include <CRC.h>
#include "rtd2660.h"
#include "../bits.h"

using namespace devices;

rtd2660::rtd2660(i2c::connection *connection): device::device(connection) {
	PLOG_DEBUG << "Device created with connection " << connection;
	this->flash = NULL;
}

void rtd2660::enterISPMode() {
	PLOG_DEBUG << "Entering ISP mode";

	if (this->isInISPMode()) {
		PLOG_WARNING << "Already in ISP mode";
		return;
	}

	uint8_t reg_value = 0;
	BIT_SET(reg_value, RTD2660::bf_program_instruction::isp_en);
	this->i2cc->write(RTD2660::registers::program_instruction, reg_value);

	if (!this->isInISPMode()) {
		throw devices::exception("Unable to enter ISP mode");
	}

	PLOG_INFO << "Device entered to ISP mode";
}

bool rtd2660::isInISPMode() {
	uint8_t reg_value = this->i2cc->read(RTD2660::registers::program_instruction);
	return BIT_CHECK(reg_value, RTD2660::bf_program_instruction::isp_en);
}

void rtd2660::exitISPMode() {
	PLOG_DEBUG << "Exiting from ISP mode";

	if (!this->isInISPMode()) {
		PLOG_WARNING << "Device is not in ISP mode";
		return;
	}

	uint8_t data = 0;
	this->i2cc->write(RTD2660::registers::program_instruction, data);

	if (this->isInISPMode()) {
		throw devices::exception("Unable to exit from ISP mode");
	}

	PLOG_INFO << "Device exited from ISP mode";
}

uint8_t rtd2660::calculateCRC(uint32_t startAddress, uint32_t endAddress) {
	PLOG_DEBUG << "Request CRC checksum from the flash controller";

	// Read the program_instruction register
	uint8_t reg_value = this->i2cc->read(RTD2660::registers::program_instruction);

	// write start address to registers
	this->i2cc->write(RTD2660::registers::flash_prog_isp0, startAddress >> 16);
	this->i2cc->write(RTD2660::registers::flash_prog_isp1, startAddress >> 8);
	this->i2cc->write(RTD2660::registers::flash_prog_isp2, startAddress);

	// write end address to registers
	this->i2cc->write(RTD2660::registers::CRC_end_addr0, endAddress >> 16);
	this->i2cc->write(RTD2660::registers::CRC_end_addr1, endAddress >> 8);
	this->i2cc->write(RTD2660::registers::CRC_end_addr2, endAddress);

	// Enable the CRC 

	BIT_SET(reg_value, RTD2660::bf_program_instruction::crc_start);
	this->i2cc->write(RTD2660::registers::program_instruction, reg_value);

	PLOG_VERBOSE << "Wait for crc_done bit is setted";

	while(1) {
		reg_value = this->i2cc->read(RTD2660::registers::program_instruction);
		if (BIT_CHECK(reg_value, RTD2660::bf_program_instruction::crc_done) == 1) break;
		usleep(1000);
	}

	PLOG_VERBOSE << "CRC calculated by the controller, read out the result";

	return this->i2cc->read(RTD2660::registers::CRC_result);
}

void rtd2660::SPI_waitProgOperation() {
	PLOG_VERBOSE << "Wait for prog_en bit clear";

	uint8_t reg_value;

	while(1) {
		// Read the program_instruction register
		reg_value = this->i2cc->read(RTD2660::registers::program_instruction);
		// Check the program enable bit
		if (BIT_CHECK(reg_value, RTD2660::bf_program_instruction::prog_en) == 0) break;
		usleep(1000);
	}
}

void rtd2660::SPI_waitOperation() {
	PLOG_VERBOSE << "Wait for enable bit clear";

	uint8_t reg_value;

	while(1) {
		// Read the Common Instruction Register
		reg_value = this->i2cc->read(RTD2660::registers::common_inst_en);
		// Check the enable bit
		if (BIT_CHECK(reg_value, RTD2660::bf_common_inst_en::comm_inst_en) == 0) break;
		usleep(1000);
	}
}

size_t rtd2660::SPI_read(uint32_t address, uint8_t *data, size_t bufferSize) {
	PLOG_VERBOSE << "Start SPI_read";
	uint32_t ret = this->SPI_commonCommand(RTD2660::v_comm_inst::read, 0x03, 3, 3, address);

	PLOG_DEBUG << "Read " << bufferSize << " byte data through SPI from 0x" << std::hex << std::setfill('0') << std::setw(6) << (int)address;

	int32_t remaining = bufferSize;
	int32_t readed = 0;

	while (remaining > 0) { // if we have remaining data
		int32_t chunkSize = remaining;
		if (chunkSize > 32) chunkSize = 32; // the chunk size can't be larger than 32 byte

		uint32_t readLength = this->i2cc->readBlock(RTD2660::registers::program_data_port, data, chunkSize);
		readed += readLength;
		data += readLength; // move the data pointer forward
		remaining -= readLength; // consume the remaining bytes
	}

	PLOG_VERBOSE << "Read done (" << readed << " bytes readed)";

	return readed;
}

uint32_t rtd2660::SPI_commonCommand(RTD2660::v_comm_inst type, uint8_t opCode, uint8_t readNum, uint8_t writeNum, uint32_t writeValue) {
	PLOG_VERBOSE << "Start SPI_commonCommand";

	// Clamp values
	readNum &= 0b11;  // 2 bit
	writeNum &= 0b11; // 2 bit
	writeValue &= 0xFFFFFF; // 16 bit

	// Set the register value, without the enable bit

	uint8_t reg_value =
		(type << RTD2660::bf_common_inst_en::comm_inst)
		| (writeNum << RTD2660::bf_common_inst_en::write_num)
		| (readNum << RTD2660::bf_common_inst_en::read_num);

	PLOG_VERBOSE << "Write the Common Instruction Register";
	this->i2cc->write(RTD2660::registers::common_inst_en, reg_value);

	// write cmd op code
	PLOG_VERBOSE << "Write cmd op code";
	this->i2cc->write(RTD2660::registers::common_op_code, opCode);

	// write bytes to ISP
	PLOG_VERBOSE << "Write bytes to ISP";

	switch (writeNum) {
		case 0: break; // No write
		case 1: // Write 1 byte
			this->i2cc->write(RTD2660::registers::flash_prog_isp0, writeValue);
			break;
		case 2: // Write 2 bytes
			this->i2cc->write(RTD2660::registers::flash_prog_isp0, writeValue >> 8);
			this->i2cc->write(RTD2660::registers::flash_prog_isp1, writeValue);
			break;
		case 3: // Write 3 bytes
			this->i2cc->write(RTD2660::registers::flash_prog_isp0, writeValue >> 16);
			this->i2cc->write(RTD2660::registers::flash_prog_isp1, writeValue >> 8);
			this->i2cc->write(RTD2660::registers::flash_prog_isp2, writeValue);
			break;
	}

	PLOG_VERBOSE << "Set the enable bit on the Common Instruction Register";

	BIT_SET(reg_value, RTD2660::bf_common_inst_en::comm_inst_en);
	this->i2cc->write(RTD2660::registers::common_inst_en, reg_value);

	// Enable bit cleared when the MCU finished the operation on the flash device
	this->SPI_waitOperation();

	// The MCU finished with the operation, we can read out the values if needed

	if (readNum != 0) PLOG_VERBOSE << "Read out common_inst_read ports";

	uint32_t retValue = 0;

	switch (readNum) {
		case 1:
			retValue = this->i2cc->read(RTD2660::registers::common_inst_read_port0);
			break;
		case 2:
			retValue =
				(this->i2cc->read(RTD2660::registers::common_inst_read_port0) << 8)
				| this->i2cc->read(RTD2660::registers::common_inst_read_port1);
			break;
		case 3:
			retValue =
				(this->i2cc->read(RTD2660::registers::common_inst_read_port0) << 16)
				| (this->i2cc->read(RTD2660::registers::common_inst_read_port1) << 8)
				| this->i2cc->read(RTD2660::registers::common_inst_read_port2);
			break;
	}

	return retValue;
}

uint32_t rtd2660::getFlashJedecID() {
	uint32_t jedecId = this->SPI_commonCommand(RTD2660::v_comm_inst::read, 0x9f, 3, 0, 0);
	PLOG_DEBUG << "Flash device Jedec ID: "  << std::hex << jedecId;
	return jedecId;
}

void rtd2660::setupFlashOpCodes() {
	if (this->flash == NULL) return;
	int16_t wren = this->flash->getOpCode_writeEnable();
	int16_t ewsr = this->flash->getOpCode_writeRegister();
	int16_t read = this->flash->getOpCode_read();
	int16_t fast_read = this->flash->getOpCode_fastRead();
	int16_t program = this->flash->getOpCode_program();
	int16_t read_status_register = this->flash->getOpCode_readStatusRegister();

	if (wren != -1) this->i2cc->write(RTD2660::registers::wren_op_code, wren);
	else PLOG_WARNING << "No flash opcode for wren";

	if (ewsr != -1) this->i2cc->write(RTD2660::registers::ewsr_op_code, ewsr);
	else PLOG_WARNING << "No flash opcode for ewsr";

	if (read != -1) this->i2cc->write(RTD2660::registers::read_op_code, read);
	else PLOG_WARNING << "No flash opcode for read";

	if (fast_read != -1) this->i2cc->write(RTD2660::registers::fast_read_op_code, fast_read);
	else PLOG_WARNING << "No flash opcode for fast_read";

	if (program != -1) this->i2cc->write(RTD2660::registers::program_op_code, program);
	else PLOG_WARNING << "No flash opcode for program";

	if (read_status_register != -1) this->i2cc->write(RTD2660::registers::read_status_register_op_code, read_status_register);
	else PLOG_WARNING << "No flash opcode for read_status_register";
}

void rtd2660::setFlashDevice(flash::device *flash) {
	this->flash = flash;
	if (this->flash != NULL) this->setupFlashOpCodes();
}

size_t rtd2660::readFlashContent(uint8_t *buffer, uint32_t startAddress, size_t size) {
	if (this->flash == NULL) throw devices::exception("Unable to read flash content without flash device setted before");

	uint32_t currentAddress = startAddress;
	uint32_t remaining = size;
	uint32_t chunkSize;

	uint8_t *dataPtr = buffer;

	while(1) {

		// chunk size maximum is 1kb
		if (remaining > 1024) chunkSize = 1024;
		else chunkSize = remaining;

		PLOG_INFO << "Read flash content - (" << chunkSize << " byte from address 0x" << std::hex << std::setfill('0') << std::setw(6) << (int)currentAddress << ")";

		uint32_t readed = this->SPI_read(currentAddress, dataPtr, chunkSize);
		if (readed == 0) throw devices::exception("Unable to read flash content / 0 byte readed");
		dataPtr += readed; // move the data pointer
		currentAddress += readed; // move the address forward
		remaining -= readed; // decrease the remaining data
		if (remaining <= 0) break;
	}

	uint32_t totalReaded = currentAddress - startAddress;

	PLOG_INFO << "Flash content readed out, check CRC";

	// Check CRC

	uint8_t mcuCRC = this->calculateCRC(startAddress, startAddress + size - 1);
	uint8_t localCRC = CRC::Calculate(buffer, size, CRC::CRC_8());


	PLOG_DEBUG << "MCU CRC: " << std::setfill('0') << std::setw(2) << std::hex << (int)mcuCRC;
	PLOG_DEBUG << "Generated CRC: " << std::setfill('0') << std::setw(2) << std::hex << (int)localCRC;

	if (mcuCRC != localCRC) throw devices::exception("Generated CRC/MCU CRC mismatch");

	PLOG_INFO << "CRC ok";

	return totalReaded;
}

void rtd2660::writeFlashContent(uint8_t *buffer, uint32_t startAddress, size_t size) {
	if (this->flash == NULL) throw devices::exception("Unable to write flash content without flash device setted before");

	// check erase support
	bool hasEraseSupport = false;
	if (this->flash->getOpCode_chipErase() != -1) hasEraseSupport = true;

	// set registers

	if (this->flash->getOpCode_writeRegister() != -1) {
		// Unprotect the status register - EWSR (Enable Write Status Register)
		this->SPI_commonCommand(RTD2660::v_comm_inst::write_after_EWSR, 0x01, 0, 1, 0x00);
	} else PLOG_WARNING << "Flash chip hasnt got EWSR register, write may be faulty";

	// Unprotect the flash - WREN (WRite ENable)
	this->SPI_commonCommand(RTD2660::v_comm_inst::write_after_WREN, 0x01, 0, 1, 0x00);

	if (hasEraseSupport) {
		// Erase chip content
		PLOG_INFO << "Erasing flash content";
		this->SPI_commonCommand(RTD2660::v_comm_inst::erase, 0xc7, 0, 0, 0x00); // Read 0xC7 from flash SHER
		this->SPI_waitProgOperation();
		PLOG_INFO << "Erase finished";
	} else PLOG_WARNING << "Flash chip hasnt got chip erase support, the write process will be slower";

	// Write content
	uint8_t *dataPtr = buffer;
	uint32_t currentAddress = startAddress;
	uint32_t remaining = size;
	uint32_t chunkSize;

	while (1) {
		// we can write 256 byte in 1 cycle
		if (remaining <= 0) break;

		if (remaining > 256) chunkSize = 256;
		else chunkSize = remaining;

		if (hasEraseSupport) { // If we has erase support, we can check the next 'chunkSize' amount of byte.
			// Erase setting all of the byte to 0xFF
			bool containsData = false;
			// Check the dataPtr, if we have anything else than 0xFF, we need to write this chunk
			for (int i = 0; i < chunkSize; i++) {
				if (*(dataPtr + i) != 0xFF) {
					containsData = true;
					break;
				}
			}

			if (!containsData) {
				PLOG_VERBOSE << "Skip flash content (" << chunkSize << " byte to address 0x" << std::hex << std::setfill('0') << std::setw(6) << (int)currentAddress << ")";
				// skip the write itself
				remaining -= chunkSize; // consume the remaining data
				currentAddress += chunkSize; // move the address forward
				dataPtr += chunkSize;
				continue;
			}
		}

		PLOG_INFO << "Write flash content (" << chunkSize << " byte to address 0x" << std::hex << std::setfill('0') << std::setw(6) << (int)currentAddress << ")";

		// write the data length into the register
		this->i2cc->write(RTD2660::registers::program_length, chunkSize - 1);

		// write the data adress into the registers
		this->i2cc->write(RTD2660::registers::flash_prog_isp0, currentAddress >> 16);
		this->i2cc->write(RTD2660::registers::flash_prog_isp1, currentAddress >> 8);
		this->i2cc->write(RTD2660::registers::flash_prog_isp2, currentAddress);

		uint32_t totalWritten = 0;

		// upload the data to the register
		while(1) {

			uint8_t regChunkSize;
			if (chunkSize > 32) regChunkSize = 32;
			else regChunkSize = chunkSize;

			PLOG_VERBOSE << "Write " << (int)regChunkSize << " byte to the program data port (Total chunk size: " << chunkSize << ")";

			// write 32 byte each time to the register
			this->i2cc->writeBlock(RTD2660::registers::program_data_port, dataPtr, regChunkSize);

			totalWritten += regChunkSize;
			// move the data pointer forward
			dataPtr += regChunkSize;
			// consume the chunk size
			chunkSize -= regChunkSize;

			if (chunkSize <= 0) break;
		}

		remaining -= totalWritten; // consume the remaining data
		currentAddress += totalWritten; // move the address forward

		// Read the program_instruction register
		uint8_t reg_value = this->i2cc->read(RTD2660::registers::program_instruction);
		// set the program enable bit and start the write cycle
		BIT_SET(reg_value, RTD2660::bf_program_instruction::prog_en);
		// write back the value with the enable bit
		this->i2cc->write(RTD2660::registers::program_instruction, reg_value);

		// wait for the write cycle
		this->SPI_waitProgOperation();
	}

	// Protect the status register 
	this->SPI_commonCommand(RTD2660::v_comm_inst::write_after_EWSR, 0x01, 0, 1, 0x1c);
	// Protect the flash
	this->SPI_commonCommand(RTD2660::v_comm_inst::write_after_WREN, 0x01, 0, 1, 0x1c);

	PLOG_INFO << "Write finished, check CRC";

	// Check CRC

	uint8_t mcuCRC = this->calculateCRC(startAddress, startAddress + size - 1);
	uint8_t localCRC = CRC::Calculate(buffer, size, CRC::CRC_8());

	PLOG_DEBUG << "MCU CRC: " << std::setfill('0') << std::setw(2) << std::hex << (int)mcuCRC;
	PLOG_DEBUG << "Generated CRC: " << std::setfill('0') << std::setw(2) << std::hex << (int)localCRC;

	if (mcuCRC != localCRC) throw devices::exception("Generated CRC/MCU CRC mismatch");

	PLOG_INFO << "CRC ok";
}

rtd2660::~rtd2660() {
}
