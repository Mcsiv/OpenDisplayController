#include <plog/Log.h>
#include <iostream>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

extern "C"
{
  #include <linux/i2c-dev.h>
  #include <i2c/smbus.h>
}

#include "i2c.h"

using namespace i2c;

connection::connection(int adapter, uint8_t address) {
	this->adapter = adapter;
	this->address = address;
	this->file = -1;
	this->filename =  "/dev/i2c-" + std::to_string(adapter);

	PLOG_DEBUG << "[i2c-connection] New i2c connector created (Adapter: " << std::to_string(this->adapter)
		<< " Address: 0x" << std::hex << std::setfill('0') << std::setw(2) << (int)this->address
		<< " File: " << this->filename << ")";

	this->open();
}

bool connection::isOpened() {
	if (this->file == -1) return false;
	return true;
}

void connection::open() {
	if (this->isOpened()) return;

	this->file = ::open(this->filename.c_str(), O_RDWR);
	if (this->file < 0) {
		this->file = -1;
		throw i2c::exception("Unable to open the adapter");
	}

	if (ioctl(file, I2C_SLAVE, this->address) < 0) {
		try {
			this->close();
		} catch(i2c::exception& e) {
			throw i2c::exception("Unable to set slave address and close the connection");
		}

		throw i2c::exception("Unable to set slave address");
	}

}

void connection::close() {
	if (!this->isOpened()) return;

	if (::close(this->file) < 0) {
		throw i2c::exception("Unable to close the adapter");
	}

	this->file = -1;
}

void connection::write(uint8_t reg, uint8_t data) {
	if (i2c_smbus_write_byte_data(this->file, reg, data) < 0) throw i2c::exception("Unable to write i2c device");
}

uint8_t connection::read(uint8_t reg) {
	int32_t result;
	if ((result = i2c_smbus_read_byte_data(this->file, reg)) == -1) throw i2c::exception("Unable to read i2c device");
	return (uint8_t)result;
}

void connection::writeBlock(uint8_t reg, uint8_t *data, uint8_t len) {
	if (i2c_smbus_write_i2c_block_data(this->file, reg, len, data) < 0) throw i2c::exception("Unable to write i2c device");
}

uint8_t connection::readBlock(uint8_t reg, uint8_t *dest, uint8_t len) {
	int read;
	if ((read = i2c_smbus_read_i2c_block_data(this->file, reg, len, dest)) == -1) throw i2c::exception("Unable to read i2c device");
	return read;
}

connection::~connection() {
	if (this->isOpened()) this->close();
}
