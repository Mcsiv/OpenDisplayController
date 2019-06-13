#pragma once

#include <string>
#include <exception>
#include "../i2c.h"
#include "../flash.h"

namespace devices {

	class exception : public std::exception {
		public:
			exception(const std::string m="unnamed exception"):msg(m){};
			const char* what(){return msg.c_str();};
		private:
			std::string msg;
	};

	class device {
		protected:
			i2c::connection *i2cc;
		public:
			device(i2c::connection *connection) {
				if (!connection->isOpened()) throw new devices::exception("Unable to use closed i2c connection");
				this->i2cc = connection;
			};
			~device() {};
			virtual void enterISPMode() = 0;
			virtual bool isInISPMode() = 0;
			virtual void exitISPMode() = 0;

			virtual uint32_t getFlashJedecID() = 0;
			virtual void setFlashDevice(flash::device *flash) = 0;
			virtual size_t readFlashContent(uint8_t *buffer, uint32_t startAddress, size_t size) = 0;
			virtual void writeFlashContent(uint8_t *buffer, uint32_t startAddress, size_t size) = 0;
	};

};