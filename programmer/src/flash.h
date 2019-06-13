#pragma once

#include <string>

namespace flash {

	struct manufacturer_s {
		uint8_t id;
		const char* name;

		int16_t writeEnable;
		int16_t writeRegister;
		int16_t read;
		int16_t fastRead;
		int16_t program;
		int16_t readStatusRegister;
		int16_t chipErase;
	};

	struct desc_s {
		const char* name;
		uint32_t jedecId;
		uint32_t size_kb;
		uint32_t pageSize;
		uint32_t blockSize_kb;

		int16_t writeEnable;
		int16_t writeRegister;
		int16_t read;
		int16_t fastRead;
		int16_t program;
		int16_t readStatusRegister;
		int16_t chipErase;
	};

	enum standardRegisters {
		JEDECID = 0x9f
	};

	class device {
		private:
			uint32_t jedecId;
			uint8_t manufacturerId;

			struct desc_s *desc;
			struct manufacturer_s *manufacturer;

		public:
			device(uint32_t jedecId);
			~device();

			std::string getManufacturerName() {return std::string(this->manufacturer->name);};

			int16_t getOpCode_writeEnable() {
				if (this->desc->writeEnable != -1) return this->desc->writeEnable;
				return this->manufacturer->writeEnable;
			};
			int16_t getOpCode_writeRegister() {
				if (this->desc->writeRegister != -1) return this->desc->writeRegister;
				return this->manufacturer->writeRegister;
			};
			int16_t getOpCode_read() {
				if (this->desc->read != -1) return this->desc->read;
				return this->manufacturer->read;
			};
			int16_t getOpCode_fastRead() {
				if (this->desc->fastRead != -1) return this->desc->fastRead;
				return this->manufacturer->fastRead;
			};
			int16_t getOpCode_program() {
				if (this->desc->program != -1) return this->desc->program;
				return this->manufacturer->program;
			};
			int16_t getOpCode_readStatusRegister() {
				if (this->desc->readStatusRegister != -1) return this->desc->readStatusRegister;
				return this->manufacturer->readStatusRegister;
			};
			int16_t getOpCode_chipErase() {
				if (this->desc->chipErase != -1) return this->desc->chipErase;
				return this->manufacturer->chipErase;
			};

			std::string getName() {return std::string(this->desc->name);};
			uint32_t getSize() {return this->desc->size_kb * 1024;};
			uint32_t getPageSize() {return this->desc->pageSize;};
			uint32_t getBlockSize() {return this->desc->blockSize_kb * 1024;};
	};
};