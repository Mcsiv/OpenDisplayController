#include <stdio.h>
#include <plog/Log.h>
#include <plog/Appenders/ColorConsoleAppender.h>
#include <argparse.h>
#include "i2c.h"
#include "flash.h"
#include "devices/rtd2660.h"

void downloadFirmware(devices::device *device, std::string filename) {
	PLOG_INFO << "Download firmware from device, enter ISP mode first";
	device->enterISPMode();

	PLOG_INFO << "Query info about the flash chip";

	uint32_t flashJedecId = device->getFlashJedecID();
	flash::device *flash = new flash::device(flashJedecId);

	PLOG_INFO << "Flash device detected (jedec ID: " << std::hex << flashJedecId << " / Manufacturer: " << flash->getManufacturerName() << " / Name: " << flash->getName() << ")";

	device->setFlashDevice(flash);

	uint32_t startAddress = 0;
	uint32_t endAddress = flash->getSize();
	uint32_t size = endAddress - startAddress;

	uint8_t *buffer = (uint8_t*)malloc(size);
	if (!buffer) {
		delete flash;
		throw std::runtime_error("Unable to allocate memory for firmware");
	}

	size_t readed = device->readFlashContent(buffer, startAddress, size);

	if (readed != size) PLOG_WARNING << "Downloaded size is not same with the flash chip size (maybe the downloaded data is corrupt)";

	PLOG_INFO << "Write downloaded data into file";

	FILE *fp = fopen(filename.c_str(), "wb+");
	if (fp) {
		fwrite(buffer, sizeof(uint8_t), size, fp);
		fclose(fp);
	} else PLOG_FATAL << "Unable to open the file";

	PLOG_INFO << "Exit from ISP mode, the device will be restart after this";
	device->exitISPMode();

	delete flash;
}

void uploadFirmware(devices::device *device, std::string filename) {
	PLOG_INFO << "Upload firmware to device, enter ISP mode first";
	device->enterISPMode();

	PLOG_INFO << "Query info about the flash chip";

	uint32_t flashJedecId = device->getFlashJedecID();
	flash::device *flash = new flash::device(flashJedecId);

	PLOG_INFO << "Flash device detected (jedec ID: " << std::hex << flashJedecId << " / Manufacturer: " << flash->getManufacturerName() << " / Name: " << flash->getName() << ")";

	device->setFlashDevice(flash);

	FILE *fp = fopen(filename.c_str(), "rb");
	if (fp) {

		fseek(fp, 0L, SEEK_END);
		size_t binarySize = ftell(fp);
		fseek(fp, 0L, SEEK_SET);

		uint8_t *buffer = (uint8_t*)malloc(binarySize);

		if (buffer == NULL) {
			fclose(fp);
			delete flash;
			throw std::runtime_error("Unable to allocate memory for firmware");
		}

		size_t readed = fread(buffer, sizeof(uint8_t), binarySize, fp);

		if (readed == binarySize) {
			device->writeFlashContent(buffer, 0x0, readed);
		} else PLOG_FATAL << "The binary size and the readed size mismatch";

		fclose(fp);
	} else PLOG_FATAL << "Unable to open the file";

	PLOG_INFO << "Exit from ISP mode, the device will be restart after this";
	device->exitISPMode();
}

int main(int argc, char *argv[]) {

	ArgumentParser parser("odc_prog");

	parser.add_argument("-l", "log level (default/debug/verbose)", false);
	parser.add_argument("-t", "Device type (rtd2660)", true);
	parser.add_argument("-m", "Programmer mode (Available modes: download / upload)", true);
	parser.add_argument("-f", "Binary file for upload or download", true);
	parser.add_argument("-d", "i2c bus device ID (1 means /dev/i2c-1)", true);

	try {
		parser.parse(argc, argv);
	} catch (const ArgumentParser::ArgumentNotFound& ex) {
		std::cout << ex.what() << std::endl;
		return 0;
	}

	if (parser.is_help()) {
		std::cout << std::endl << "download: download firmware from the board" << std::endl
				<< "upload: upload firmware to the board" << std::endl
				<< "Example arguments: -d 2 -m upload -f firmware.bin" << std::endl << std::endl;
		return 0;
	}

	static plog::ColorConsoleAppender<plog::TxtFormatter> consoleAppender;
	plog::init(plog::info, "programmer.log").addAppender(&consoleAppender);

	int i2cID = parser.get<int>("d");
	std::string level = parser.get<std::string>("l");
	std::string deviceType = parser.get<std::string>("t");
	std::string file = parser.get<std::string>("f");
	std::string mode = parser.get<std::string>("m");

	if (level == "" | level == "info") plog::get()->setMaxSeverity(plog::info);
	else if (level == "debug") plog::get()->setMaxSeverity(plog::debug);
	else if (level == "verbose") plog::get()->setMaxSeverity(plog::verbose);

	i2c::connection *conn = NULL;
	try {
		conn = new i2c::connection(i2cID, 0x4A);
	} catch(i2c::exception& e) {
		PLOG_FATAL << "i2c exception: " << std::string(e.what());
		return 1;
	}

	devices::device *device = NULL;
	if (deviceType == "rtd2660") device = new devices::rtd2660(conn);
	else {
		PLOG_FATAL << "Unknown device: " << deviceType;
		delete conn;
		return 1;
	}

	try {
		if (mode == "download") downloadFirmware(device, file);
		else if (mode == "upload") uploadFirmware(device, file);
		else PLOG_FATAL << "Unknown mode: " << mode;
	} catch(devices::exception& e) {
		PLOG_FATAL << "device exception: " << std::string(e.what());
	} catch(i2c::exception& e) {
		PLOG_FATAL << "i2c exception: " << std::string(e.what());
	} catch(std::exception& e) {
		PLOG_FATAL << "std::exception: " << std::string(e.what());
	} catch(...) {
		std::exception_ptr p = std::current_exception();
		PLOG_FATAL << "Unknown exception: " << (p ? p.__cxa_exception_type()->name() : "null");
	}

	delete device;
	delete conn;

}
