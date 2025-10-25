#include "BgmHeader.h"
#include <SFML/System/Err.hpp>
#include <SFML/System/InputStream.hpp>

namespace {
constexpr char InvalidOggFile[] = "Invalid OGG file";
constexpr char InvalidFlacFile[] = "Invalid FLAC file";
constexpr char FileCorrupted[] = "Corrupted";
bool readOggVorbisCommentOHMSSP(bgm::InputStream& file, std::string& _key, std::string& _val);
bool readFlacCommentOHMSSP(bgm::InputStream& file, std::string& _key, std::string& _val);
}

namespace bgm {

std::ostream& err() {
	return sf::err();
}

std::any readLoopPoints(InputStream& stream) {
	try {
		std::string key;
		std::string val;

		char header[4]{};
		if (auto res = stream.seek(0); !res) {
			err() << "Failed to init file." << std::endl;
			return {};
		}
		if (auto res = stream.read(&(header[0]), 4); !res || res != 4) {
			err() << "Failed to read file." << std::endl;
			return {};
		}
		if (auto res = stream.seek(0); !res) {
			err() << "Failed to reset file." << std::endl;
			return {};
		}

		{
			std::string h = std::string(header, 4);
			if (h == "OggS") {
				if (!readOggVorbisCommentOHMSSP(stream, key, val)) {
					err() << "Failed to read comments for OGG file." << std::endl;
					return {};
				}
			}
			else if (h == "fLaC") {
				if (!readFlacCommentOHMSSP(stream, key, val)) {
					err() << "Failed to read comments for FLAC file." << std::endl;
					return {};
				}
			}
			else {
				err() << "Unsupported file format." << std::endl;
				return {};
			}
		}

		if (key == "OHMSSPD") {
			if (*(val.cbegin()) != '<' || *(val.crbegin()) != '>') {
				err() << FileCorrupted << std::endl;
				return {};
			}

			auto split = val.find('|');
			if (split == std::string::npos) {
				err() << FileCorrupted << std::endl;
				return {};
			}

			std::string st = val.substr(1, split - 1);
			std::string in = val.substr(split + 1, val.length() - split - 2);

			return Span<std::uint64_t>{ std::stoull(st), std::stoull(in) };
		}
		else if (key == "OHMSSPC") {
			if (*(val.cbegin()) != '>' || *(val.crbegin()) != '<') {
				err() << FileCorrupted << std::endl;
				return {};
			}

			auto split = val.find(':');
			if (split == std::string::npos) {
				err() << FileCorrupted << std::endl;
				return {};
			}

			std::string st = val.substr(1, split - 1);
			std::string in = val.substr(split + 1, val.length() - split - 2);

			return Span<Time>{ microseconds(std::stoull(st)), microseconds(std::stoull(in)) };
		}
		else {
			err() << "No loop points info found." << std::endl;
		}
	}
	catch (...) {
		err() << "An exception occurred when reading loop points info." << std::endl;
	}
	return {};
}

void BGM_STREAM_EMPTY_DELETER::operator()(InputStream* object) const {
	(void)object;
}

}

namespace {

std::optional<uint32_t> readbe32(bgm::InputStream& stream) {
	std::byte buf[4]{};
	std::uint32_t res = 0;
	if (auto r = stream.read(&(buf[0]), 4); !r || r != 4) {
		return {};
	}
	res = ((uint32_t)buf[0] << 24) | ((uint32_t)buf[1] << 16) | ((uint32_t)buf[2] << 8) | ((uint32_t)buf[3]);
	return res;
}

std::optional<uint64_t> readbe64(bgm::InputStream& stream) {
	std::byte buf[8]{};
	std::uint64_t res = 0;
	if (auto r = stream.read(&(buf[0]), 8); !r || r != 8) {
		return {};
	}
	res = ((uint64_t)buf[0] << 56) | ((uint64_t)buf[1] << 48) | ((uint64_t)buf[2] << 40) | ((uint64_t)buf[3] << 32);
	res |= ((uint64_t)buf[4] << 24) | ((uint64_t)buf[5] << 16) | ((uint64_t)buf[6] << 8) | ((uint64_t)buf[7]);
	return res;
}

std::optional<uint32_t> readle32(bgm::InputStream& stream) {
	std::byte buf[4]{};
	std::uint32_t res = 0;
	if (auto r = stream.read(&(buf[0]), 4); !r || r != 4) {
		return {};
	}
	res = ((uint32_t)buf[3] << 24) | ((uint32_t)buf[2] << 16) | ((uint32_t)buf[1] << 8) | ((uint32_t)buf[0]);
	return res;
}

std::optional<uint64_t> readle64(bgm::InputStream& stream) {
	std::byte buf[8]{};
	std::uint64_t res = 0;
	if (auto r = stream.read(&(buf[0]), 8); !r || r != 8) {
		return {};
	}
	res = ((uint64_t)buf[7] << 56) | ((uint64_t)buf[6] << 48) | ((uint64_t)buf[5] << 40) | ((uint64_t)buf[4] << 32);
	res |= ((uint64_t)buf[3] << 24) | ((uint64_t)buf[2] << 16) | ((uint64_t)buf[1] << 8) | ((uint64_t)buf[0]);
	return res;
}

bool readTagData(std::byte* buf, uint64_t len, std::string& _key, std::string& _val) {
	uint64_t pos = 0;
	bool found = false;

	// 读取厂商字符串长度
	uint32_t vendorLength;
	vendorLength = ((uint32_t)buf[pos] | ((uint32_t)buf[pos + 1] << 8) | ((uint32_t)buf[pos + 2] << 16) | ((uint32_t)buf[pos + 3] << 24));
	pos += 4;
	if (pos > len) {
		bgm::err() << InvalidOggFile << std::endl;
		return false;
	}
	pos += vendorLength; // 跳过厂商字符串
	if (pos > len) {
		bgm::err() << InvalidOggFile << std::endl;
		return false;
	}

	// 读取注释数量
	uint32_t commentCount;
	commentCount = ((uint32_t)buf[pos] | ((uint32_t)buf[pos + 1] << 8) | ((uint32_t)buf[pos + 2] << 16) | ((uint32_t)buf[pos + 3] << 24));
	pos += 4;
	if (pos > len) {
		bgm::err() << InvalidOggFile << std::endl;
		return false;
	}

	// 读取每个注释
	for (uint32_t c = 0; c < commentCount; ++c) {
		uint32_t commentLength;
		commentLength = ((uint32_t)buf[pos] | ((uint32_t)buf[pos + 1] << 8) | ((uint32_t)buf[pos + 2] << 16) | ((uint32_t)buf[pos + 3] << 24));
		pos += 4;
		if (pos > len) {
			bgm::err() << InvalidOggFile << std::endl;
			return false;
		}

		std::string comment((char*)&buf[pos], commentLength);
		pos += commentLength;
		if (pos > len) {
			bgm::err() << InvalidOggFile << std::endl;
			return false;
		}

		// 解析键值对
		size_t equalsPos = comment.find('=');
		if (equalsPos == std::string::npos) {
			continue;
		}
		std::string key = comment.substr(0, equalsPos);
		std::string value = comment.substr(equalsPos + 1);
		if (key.length() >= 6 && key.substr(0, 6) == "OHMSSP") {
			if (found) {
				bgm::err() << "Redundant tag.";
				throw std::runtime_error("Redundant tag.");
			}
			_key = key;
			_val = value;
			found = true;
		}
	}

	if (pos != len) {
		found = false;
	}

	return found;
}

struct OggPageHeader {
	char capturePattern[4]; // "OggS"
	uint8_t version;
	uint8_t headerType;
	uint64_t granulePosition;
	uint32_t bitstreamSerialNumber;
	uint32_t pageSequenceNumber;
	uint32_t checksum;
	uint8_t segmentCount;
	uint8_t* segmentTable;
};

/*struct OggCommentPacketHeader {
	uint8_t headerTypeFlag; // 1 = identification, 3 = comment, 5 = setup;
	char packetPattern[6]; // "vorbis"
	uint32_t vendorLength;
	char* vendorString;
	uint32_t userCommentListLength;
	uint8_t** comments; // Every comment consists of 4 bytes for length N, then followed by N bytes for comment string.
	uint8_t framingFlag; // = 1;
};*/

bool readOggVorbisCommentOHMSSP(bgm::InputStream& file, std::string& _key, std::string& _val) {
	// OGG文件由多个packet组成，需要找到包含注释的packet

	uint64_t packetSize = 0;
	std::vector<std::byte> packetData;
	bool isCurentCommentPacket = false;

	while (true) {
		OggPageHeader header{};

		if (auto r = file.read(&(header.capturePattern[0]), 4); !r || r != 4) {
			bgm::err() << InvalidOggFile << std::endl;
			return false;
		}
		if (std::string(header.capturePattern, 4) != "OggS") {
			bgm::err() << InvalidOggFile << std::endl;
			return false;
		}

		if (auto r = file.read(&header.version, 1); !r || r != 1) {
			bgm::err() << InvalidOggFile << std::endl;
			return false;
		}
		if (auto r = file.read(&header.headerType, 1); !r || r != 1) {
			bgm::err() << InvalidOggFile << std::endl;
			return false;
		}

		if (auto r = readle64(file); !r) {
			bgm::err() << InvalidOggFile << std::endl;
			return false;
		}
		else {
			header.granulePosition = *r;
		}

		if (auto r = readle32(file); !r) {
			bgm::err() << InvalidOggFile << std::endl;
			return false;
		}
		else {
			header.bitstreamSerialNumber = *r;
		}

		if (auto r = readle32(file); !r) {
			bgm::err() << InvalidOggFile << std::endl;
			return false;
		}
		else {
			header.pageSequenceNumber = *r;
		}

		if (auto r = readle32(file); !r) {
			bgm::err() << InvalidOggFile << std::endl;
			return false;
		}
		else {
			header.checksum = *r;
		}

		if (auto r = file.read(&header.segmentCount, 1); !r || r != 1) {
			bgm::err() << InvalidOggFile << std::endl;
			return false;
		}

		// 读取segment大小表
		std::vector<uint8_t> segmentTable(header.segmentCount);
		if (auto r = file.read(segmentTable.data(), header.segmentCount); !r || r != header.segmentCount) {
			bgm::err() << InvalidOggFile << std::endl;
			return false;
		}

		// 拼接Packet的数据
		for (auto size : segmentTable) {
			packetData.resize(packetSize + size);
			if (packetSize == 0 || isCurentCommentPacket) {
				if (auto r = file.read(packetData.data() + packetSize, size); !r || r != size) {
					bgm::err() << InvalidOggFile << std::endl;
					return false;
				}
				isCurentCommentPacket = isCurentCommentPacket || ((uint8_t)packetData[0] == 0x03);
			}
			else {
				if (auto p = file.tell(); !p) {
					bgm::err() << InvalidOggFile << std::endl;
					return false;
				}
				else if (auto r = file.seek(*p + size); !r || r != (*p + size)) {
					bgm::err() << InvalidOggFile << std::endl;
					return false;
				}
			}
			packetSize += size;
			if (size == 0xFF) {
				continue;
			}
			// 当前packet结束
			bool found = false;
			while (isCurentCommentPacket) {
				isCurentCommentPacket = false;

				size_t pos = 1;
				if (std::string((char*)(&packetData[pos]), 6) != "vorbis") {
					break;
				}
				pos += 6;
				if (pos > packetSize) {
					bgm::err() << InvalidOggFile << std::endl;
					return false;
				}

				found = readTagData(packetData.data() + pos, packetSize - pos - 1, _key, _val);

				// 注释结束
				if ((uint8_t)packetData[packetSize - 1] != 0x01) {
					bgm::err() << InvalidOggFile << std::endl;
					return false;
				}
			}
			if (found) {
				return true;
			}
			packetData.clear();
			packetSize = 0;
		}
	}
	return false;
}


bool readFlacCommentOHMSSP(bgm::InputStream& file, std::string& _key, std::string& _val) {
	char header[4]{};

	if (auto r = file.read(&(header[0]), 4); !r || r != 4) {
		bgm::err() << InvalidFlacFile << std::endl;
		return false;
	}
	if (std::string(header, 4) != "fLaC") {
		bgm::err() << InvalidFlacFile << std::endl;
		return false;
	}

	bool isLastBlock = false;
	while (!isLastBlock) {
		uint32_t blockHeader = 0;
		if (auto r = readbe32(file); !r) {
			bgm::err() << InvalidFlacFile << std::endl;
			return false;
		}
		else {
			blockHeader = *r;
		}

		uint8_t blockCtrl = (blockHeader >> 24) & 0xFF;
		uint32_t blockDataSize = blockHeader & 0x00FFFFFF;

		isLastBlock = (blockCtrl & 0x80) == 0x80;
		if ((blockCtrl & 0x7F) != 4) {
			if (auto p = file.tell(); !p) {
				bgm::err() << InvalidFlacFile << std::endl;
				return false;
			}
			else if (auto r = file.seek(*p + blockDataSize); !r || r != (*p + blockDataSize)) {
				bgm::err() << InvalidFlacFile << std::endl;
				return false;
			}
			continue;
		}
		
		// 是标签信息
		std::vector<std::byte> tagData(blockDataSize);
		if (auto r = file.read(tagData.data(), blockDataSize); !r || r != blockDataSize) {
			bgm::err() << InvalidFlacFile << std::endl;
			return false;
		}

		bool res = readTagData(tagData.data(), blockDataSize, _key, _val);

		if (res) {
			return true;
		}
	}

	return false;
}

}
