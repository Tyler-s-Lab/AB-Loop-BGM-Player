#include "BgmHeader.h"
#include <SFML/System/Err.hpp>
#include <SFML/System/InputStream.hpp>

namespace {
constexpr char InvalidOggFile[] = "Invalid OGG file";
constexpr char FileCorrupted[] = "Corrupted";
bool readOggVorbisCommentOHMSSP(bgm::InputStream& file, std::string& _key, std::string& _val);
}

namespace bgm {

std::ostream& err() {
	return sf::err();
}

std::any readLoopPoints(InputStream& stream) {
	try {
		std::string key;
		std::string val;

		if (!readOggVorbisCommentOHMSSP(stream, key, val)) {
			err() << "Failed to read comments for OGG file." << std::endl;
			return {};
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
	std::vector<uint8_t> packetData;
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
		// 小端序
		//uint64_t granule = 0;
		//for (int i = 0; i < 8; ++i) {
		//	uint8_t byte{};
		//	if (auto r = file.read(&byte, 1); !r || r != 1) {
		//		bgm::err() << InvalidOggFile << std::endl;
		//		return false;
		//	}
		//	granule |= (static_cast<uint64_t>(byte) << (i * 8));
		//}
		//header.granulePosition = granule;
		if (auto r = file.read(&header.granulePosition, 8); !r || r != 8) {
			bgm::err() << InvalidOggFile << std::endl;
			return false;
		}
		if (auto r = file.read(&header.bitstreamSerialNumber, 4); !r || r != 4) {
			bgm::err() << InvalidOggFile << std::endl;
			return false;
		}
		if (auto r = file.read(&header.pageSequenceNumber, 4); !r || r != 4) {
			bgm::err() << InvalidOggFile << std::endl;
			return false;
		}
		if (auto r = file.read(&header.checksum, 4); !r || r != 4) {
			bgm::err() << InvalidOggFile << std::endl;
			return false;
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
				isCurentCommentPacket = isCurentCommentPacket || (packetData[0] == 0x03);
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

				// 读取厂商字符串长度
				uint32_t vendorLength;
				vendorLength = (packetData[pos] | (packetData[pos + 1] << 8) | (packetData[pos + 2] << 16) | (packetData[pos + 3] << 24));
				pos += 4;
				if (pos > packetSize) {
					bgm::err() << InvalidOggFile << std::endl;
					return false;
				}
				pos += vendorLength; // 跳过厂商字符串
				if (pos > packetSize) {
					bgm::err() << InvalidOggFile << std::endl;
					return false;
				}

				// 读取注释数量
				uint32_t commentCount;
				commentCount = (packetData[pos] | (packetData[pos + 1] << 8) | (packetData[pos + 2] << 16) | (packetData[pos + 3] << 24));
				pos += 4;
				if (pos > packetSize) {
					bgm::err() << InvalidOggFile << std::endl;
					return false;
				}

				// 读取每个注释
				for (uint32_t c = 0; c < commentCount; ++c) {
					uint32_t commentLength;
					commentLength = (packetData[pos] | (packetData[pos + 1] << 8) | (packetData[pos + 2] << 16) | (packetData[pos + 3] << 24));
					pos += 4;
					if (pos > packetSize) {
						bgm::err() << InvalidOggFile << std::endl;
						return false;
					}

					std::string comment((char*)&packetData[pos], commentLength);
					pos += commentLength;
					if (pos > packetSize) {
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
						_key = key;
						_val = value;
						found = true;
					}
				}

				// 注释结束
				if (pos != packetSize - 1 || packetData[pos] != 0x01) {
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

}
