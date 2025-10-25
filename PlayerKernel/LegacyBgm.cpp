#include <SFML/Audio/Music.hpp>
#include "BGM.h"

#include <SFML/System/Err.hpp>
#include <SFML/System/Exception.hpp>
#include <SFML/System/InputStream.hpp>
#include <SFML/System/FileInputStream.hpp>
#include <SFML/System/MemoryInputStream.hpp>

#include <mutex>
#include <memory>
#include <string>
#include <vector>
#include <ostream>
#include <utility>
#include <cstdint>
#include <optional>
#include <filesystem>

namespace {

struct BGM_STREAM_EMPTY_DELETER {
	void operator()(sf::InputStream* object) const {
		(void)object;
	}
};

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

bool readOggVorbisCommentOHMSSP(sf::InputStream& file, std::string& _key, std::string& _val) {
	// OGG文件由多个packet组成，需要找到包含注释的packet

	uint64_t packetSize = 0;
	std::vector<uint8_t> packetData;
	bool isCurentCommentPacket = false;

	while (true) {
		OggPageHeader header{};

		if (auto r = file.read(&(header.capturePattern[0]), 4); !r || r != 4) {
			sf::err() << "Invalid OGG file" << std::endl;
			return false;
		}
		if (std::string(header.capturePattern, 4) != "OggS") {
			sf::err() << "Invalid OGG file" << std::endl;
			return false;
		}

		if (auto r = file.read(&header.version, 1); !r || r != 1) {
			sf::err() << "Invalid OGG file" << std::endl;
			return false;
		}
		if (auto r = file.read(&header.headerType, 1); !r || r != 1) {
			sf::err() << "Invalid OGG file" << std::endl;
			return false;
		}
		// 小端序
		//uint64_t granule = 0;
		//for (int i = 0; i < 8; ++i) {
		//	uint8_t byte{};
		//	if (auto r = file.read(&byte, 1); !r || r != 1) {
		//		sf::err() << "Invalid OGG file" << std::endl;
		//		return false;
		//	}
		//	granule |= (static_cast<uint64_t>(byte) << (i * 8));
		//}
		//header.granulePosition = granule;
		if (auto r = file.read(&header.granulePosition, 8); !r || r != 4) {
			sf::err() << "Invalid OGG file" << std::endl;
			return false;
		}
		if (auto r = file.read(&header.bitstreamSerialNumber, 4); !r || r != 4) {
			sf::err() << "Invalid OGG file" << std::endl;
			return false;
		}
		if (auto r = file.read(&header.pageSequenceNumber, 4); !r || r != 4) {
			sf::err() << "Invalid OGG file" << std::endl;
			return false;
		}
		if (auto r = file.read(&header.checksum, 4); !r || r != 4) {
			sf::err() << "Invalid OGG file" << std::endl;
			return false;
		}
		if (auto r = file.read(&header.segmentCount, 1); !r || r != 1) {
			sf::err() << "Invalid OGG file" << std::endl;
			return false;
		}

		// 读取segment大小表
		std::vector<uint8_t> segmentTable(header.segmentCount);
		if (auto r = file.read(segmentTable.data(), header.segmentCount); !r || r != header.segmentCount) {
			sf::err() << "Invalid OGG file" << std::endl;
			return false;
		}

		// 拼接Packet的数据
		for (auto size : segmentTable) {
			packetData.reserve(packetSize + size);
			if (packetSize == 0 || isCurentCommentPacket) {
				if (auto r = file.read(packetData.data() + packetSize, size); !r || r != size) {
					sf::err() << "Invalid OGG file" << std::endl;
					return false;
				}
				isCurentCommentPacket = isCurentCommentPacket || packetData[0] == 0x03;
			}
			else {
				if (auto p = file.tell(); !p) {
					sf::err() << "Invalid OGG file" << std::endl;
					return false;
				}
				else if (auto r = file.seek(*p + size); !r || r != (*p + size)) {
					sf::err() << "Invalid OGG file" << std::endl;
					return false;
				}
			}
			packetSize += size;
			if (size == 0xFF) {
				continue;
			}
			// 当前packet结束
			while (isCurentCommentPacket) {
				isCurentCommentPacket = false;

				size_t pos = 1;
				if (std::string((char*)(&packetData[pos]), 6) != "vorbis") {
					break;
				}
				pos += 6;
				if (pos > packetSize) {
					sf::err() << "Invalid OGG file" << std::endl;
					return false;
				}

				// 读取厂商字符串长度
				uint32_t vendorLength;
				vendorLength = (packetData[pos] | (packetData[pos + 1] << 8) | (packetData[pos + 2] << 16) | (packetData[pos + 3] << 24));
				pos += 4;
				if (pos > packetSize) {
					sf::err() << "Invalid OGG file" << std::endl;
					return false;
				}
				pos += vendorLength; // 跳过厂商字符串
				if (pos > packetSize) {
					sf::err() << "Invalid OGG file" << std::endl;
					return false;
				}

				// 读取注释数量
				uint32_t commentCount;
				commentCount = (packetData[pos] | (packetData[pos + 1] << 8) | (packetData[pos + 2] << 16) | (packetData[pos + 3] << 24));
				pos += 4;
				if (pos > packetSize) {
					sf::err() << "Invalid OGG file" << std::endl;
					return false;
				}

				// 读取每个注释
				for (uint32_t c = 0; c < commentCount; ++c) {
					uint32_t commentLength;
					commentLength = (packetData[pos] | (packetData[pos + 1] << 8) | (packetData[pos + 2] << 16) | (packetData[pos + 3] << 24));
					pos += 4;
					if (pos > packetSize) {
						sf::err() << "Invalid OGG file" << std::endl;
						return false;
					}

					std::string comment((char*)&packetData[pos], commentLength);
					pos += commentLength;
					if (pos > packetSize) {
						sf::err() << "Invalid OGG file" << std::endl;
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
						isCurentCommentPacket = true; // 利用它做成功的标记
					}
				}

				// 注释结束
				if (pos != packetSize - 1 || packetData[pos] != 0x01) {
					sf::err() << "Invalid OGG file" << std::endl;
					return false;
				}
			}
			if (isCurentCommentPacket) {
				return true;
			}
			packetData.clear();
			packetSize = 0;
		}
	}
	return false;
}


std::optional<sf::Music::Span<std::uint64_t>> readLoopPoints(sf::InputStream& stream) {
	try {
		std::string key;
		std::string val;

		if (!readOggVorbisCommentOHMSSP(stream, key, val)) {
			sf::err() << "Failed to read comments for OGG file." << std::endl;
			return {};
		}

		if (key == "OHMSSPD") {

		}
		else if (key == "OHMSSPC") {

		}
		else {
			sf::err() << "No loop points info to read." << std::endl;
			return {};
		}

	}
	catch (...) {
		sf::err() << "An exception occurred when reading loop points info." << std::endl;
		return {};
	}
	return {};
}

}

namespace sf {

struct Bgm::Impl {
	std::shared_ptr<sf::InputStream> stream;
	std::vector<std::int16_t> samples;  //!< Temporary buffer of samples
	std::recursive_mutex      mutex;    //!< Mutex protecting the data
	Span<std::uint64_t>       loopSpan; //!< Loop Range Specifier
};

Bgm::Bgm() : m_impl(std::make_unique<Impl>()) {}

Bgm::Bgm(const std::filesystem::path& filename) : Bgm() {
	if (!openFromFile(filename))
		throw Exception("Failed to open music from file (class Bgm)");
}

Bgm::Bgm(const void* data, std::size_t sizeInBytes) : Bgm() {
	if (!openFromMemory(data, sizeInBytes))
		throw Exception("Failed to open music from memory (class Bgm)");
}

Bgm::Bgm(InputStream& stream) : Bgm() {
	if (!openFromStream(stream))
		throw Exception("Failed to open music from stream (class Bgm)");
}

Bgm::~Bgm() {
	stop();
	m_impl.reset();
}

Bgm::Bgm(Bgm&&) noexcept = default;

Bgm& Bgm::operator=(Bgm&&) noexcept = default;

bool Bgm::openFromFile(const std::filesystem::path& filename) {
	stop();

	std::shared_ptr<FileInputStream> stream = std::make_shared<FileInputStream>();
	if (!stream->open(filename)) {
		err() << "Failed to open file stream to open bgm from file" << std::endl;
		return false;
	}

	Span<std::uint64_t> samplePoints;
	if (auto res = readLoopPoints(*stream); !res) {
		err() << "Failed to read comment to open bgm from file" << std::endl;
		return false;
	}
	else {
		samplePoints = *res;
	}

	if (!Base::openFromStream(*stream)) {
		err() << "Failed to open bgm from file" << std::endl;
		return false;
	}

	setLooping(true);

	// Check our state. This averts a divide-by-zero. GetChannelCount() is cheap enough to use often
	if (getChannelCount() == 0 || getSampleCount() == 0) {
		err() << "Music is not in a valid state to assign Loop Points." << std::endl;
		return;
	}


	setLoopPoints(timeSpan);

	m_stream = std::move(stream);
	return true;
}

bool Bgm::openFromMemory(const void* data, std::size_t sizeInBytes) {
	stop();

	std::shared_ptr<MemoryInputStream> stream = std::make_shared<MemoryInputStream>(data, sizeInBytes);

	Span<std::uint64_t> samplePoints;
	if (auto res = readLoopPoints(*stream); !res) {
		err() << "Failed to read comment to open bgm from memory" << std::endl;
		return false;
	}
	else {
		samplePoints = *res;
	}

	if (!Base::openFromStream(*stream)) {
		err() << "Failed to open bgm from memory" << std::endl;
		return false;
	}

	setLoopPoints(timeSpan);
	setLooping(true);

	m_stream = std::move(stream);
	return true;
}

bool Bgm::openFromStream(InputStream& _stream) {
	stop();

	std::shared_ptr<InputStream> stream = std::shared_ptr<InputStream>(&_stream, ::BGM_STREAM_EMPTY_DELETER());

	Span<std::uint64_t> samplePoints;
	if (auto res = readLoopPoints(*stream); !res) {
		err() << "Failed to read comment to open bgm from stream" << std::endl;
		return false;
	}
	else {
		samplePoints = *res;
	}

	if (!Base::openFromStream(*stream)) {
		err() << "Failed to open bgm from stream" << std::endl;
		return false;
	}

	setLoopPoints(timeSpan);
	setLooping(true);

	m_stream = std::move(stream);
	return true;
}

bool Bgm::onGetData(Chunk& data) {
	const std::lock_guard lock(m_impl->mutex);

	std::size_t         toFill = m_impl->samples.size();
	std::uint64_t       currentOffset = m_impl->file.getSampleOffset();
	const std::uint64_t loopEnd = m_impl->loopSpan.offset + m_impl->loopSpan.length;

	// If the loop end is enabled and imminent, request less data.
	// This will trip an "onLoop()" call from the underlying SoundStream,
	// and we can then take action.
	if (isLooping() && (m_impl->loopSpan.length != 0) && (currentOffset <= loopEnd) && (currentOffset + toFill > loopEnd))
		toFill = static_cast<std::size_t>(loopEnd - currentOffset);

	// Fill the chunk parameters
	data.samples = m_impl->samples.data();
	data.sampleCount = static_cast<std::size_t>(m_impl->file.read(m_impl->samples.data(), toFill));
	currentOffset += data.sampleCount;

	// Check if we have stopped obtaining samples or reached either the EOF or the loop end point
	return (data.sampleCount != 0) && (currentOffset < m_impl->file.getSampleCount()) &&
		(currentOffset != loopEnd || m_impl->loopSpan.length == 0);
}

void Bgm::onSeek(Time timeOffset) {
	const std::lock_guard lock(m_impl->mutex);
	m_impl->file.seek(timeOffset);
}

std::optional<std::uint64_t> Bgm::onLoop() {
	return std::optional<std::uint64_t>();
}

std::uint64_t Bgm::timeToSamples(Time position) const {
	// Always ROUND, no unchecked truncation, hence the addition in the numerator.
	// This avoids most precision errors arising from "samples => Time => samples" conversions
	// Original rounding calculation is ((Micros * Freq * Channels) / 1000000) + 0.5
	// We refactor it to keep std::int64_t as the data type throughout the whole operation.
	return ((static_cast<std::uint64_t>(position.asMicroseconds()) * getSampleRate() * getChannelCount()) + 500000) / 1000000;
}

} // namespace sf
