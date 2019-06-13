#pragma once

#include <string>
#include <exception>

namespace i2c {

	class exception : public std::exception {
		public:
			exception(const std::string m="unnamed exception"):msg(m){};
			const char* what(){return msg.c_str();};
		private:
			std::string msg;
	};

	class connection {
		private:
			int file;
			std::string filename;

			int adapter;
			uint8_t address;

		public:
			connection(int adapter, uint8_t address);

			bool isOpened();
			void open();
			void close();

			void write(uint8_t reg, uint8_t data);
			uint8_t read(uint8_t reg);

			void writeBlock(uint8_t reg, uint8_t *data, uint8_t len);
			uint8_t readBlock(uint8_t reg, uint8_t *dest, uint8_t len);

			~connection();
	};

};