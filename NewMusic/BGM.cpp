#include "BGM.h"
#include <SFML/System/Err.hpp>

#include <stdio.h>

namespace {

struct OHMSAUDIOCOMMENTSTRUCTURE {
	long long offset;
	long long length;

	OHMSAUDIOCOMMENTSTRUCTURE() :
		offset(0),
		length(0)
	{}
};

bool getMusicOggCommentData(sf::InputStream& stream, unsigned char* buffer, size_t& bufferLength) {
	long long pos = 0;
	unsigned char tmp[16];
	unsigned int tmplength = 0;

	if (stream.seek(0) != 0) {
		sf::err() << "getMusicCommentData: seek failed" << std::endl;
		return false;
	}

	if (stream.read(tmp, 4) == -1) return false;
	if (tmp[0] != 'O' || tmp[1] != 'g' || tmp[2] != 'g' || tmp[3] != 'S') {
		sf::err() << "getMusicCommentData: wrong file type" << std::endl;
		return false;
	}

	if(stream.seek(0) != 0) return false;
	bool fin = false;
	while (!fin) {
		pos = stream.tell();
		if (pos == -1) return false;
		if (pos >= stream.getSize()) break;

		if (stream.seek(pos + 26) == -1) return false;

		unsigned char n;
		if (stream.read(&n, 1) == -1) return false;

		unsigned char segLength[256];
		if (stream.read(segLength, n) == -1) return false;

		// read segs
		for (int i = 0; !fin && i < n; ++i) {
			long long segStartPos = stream.tell();
			if (segStartPos == -1) return false;

			if (stream.read(tmp, 1) == -1) return false;

			// seg type is not COMMENT
			if (tmp[0] != 3) {
				if (stream.seek(segStartPos + segLength[i]) == -1) return false;
				continue;
			}

			// seg type is COMMENT
			if (stream.seek(segStartPos + 7) == -1) return false;

			if (stream.read(tmp, 4) == -1) return false;
			tmplength = static_cast<unsigned long long>(tmp[3]);
			tmplength = (((((tmplength << 8) + tmp[2]) << 8) + tmp[1]) << 8) + tmp[0];

			if (stream.seek(segStartPos + 11 + tmplength + 4) == -1) return false;

			while (!fin) {
				pos = stream.tell();
				if (pos == -1) return false;
				if (pos >= segStartPos + segLength[i]) break;

				if (stream.read(tmp, 4) == -1) return false;
				tmplength = static_cast<unsigned long long>(tmp[3]);
				tmplength = (((((tmplength << 8) + tmp[2]) << 8) + tmp[1]) << 8) + tmp[0];

				if (tmplength < 13) {
					if (stream.seek(pos + 4 + tmplength) == -1) return false;
					continue;
				}

				if (stream.read(tmp, 8) == -1) return false;
				// not 'OHMSSPC='
				if (!(tmp[0] == 'O' && tmp[1] == 'H' && tmp[2] == 'M' && tmp[3] == 'S' &&
					tmp[4] == 'S' && tmp[5] == 'P' && tmp[6] == 'C' && tmp[7] == '=')) {
					if (stream.seek(pos + 4 + tmplength) == -1) return false;
					continue;
				}
				// is 'OHMSSPC='
				tmplength = tmplength - 8;

				if (tmplength >= bufferLength) return false;
				bufferLength = tmplength;
				if (stream.read(buffer, tmplength) == -1) return false;
				buffer[tmplength] = '\0';
				fin = true;
			}
			if (stream.seek(segStartPos + segLength[i]) == -1) return false;
		}
	}
	return fin;
}


bool readMusicLoopPointStrict(sf::InputStream& stream, OHMSAUDIOCOMMENTSTRUCTURE& data) {
	unsigned char tmp[64];
	size_t length = 64;
	if (!getMusicOggCommentData(stream, tmp, length)) {
		return false;
	}
	if (tmp[0] != '>') {
		return false;
	}
	long long val = 0;
	size_t i = 1;
	for (; i < length; ++i) {
		if (tmp[i] == ':') break;
		if (tmp[i] < '0' || tmp[i] > '9') return false;
		val = val * 10 + tmp[i] - '0';
	}
	data.offset = val;
	if (i >= length - 1 || tmp[i] != ':') return false;
	val = 0;
	for (++i; i < length; ++i) {
		if (tmp[i] == '<') break;
		if (tmp[i] < '0' || tmp[i] > '9') return false;
		val = val * 10 + tmp[i] - '0';
	}
	data.length = val;
	if (i >= length || tmp[i] != '<') return false;
	return true;
}


} // namespace



namespace ohms {
namespace audio {


ohms::audio::BGM::BGM() :
	m_music(std::make_unique<sf::Music>())
{}


ohms::audio::BGM::~BGM() {
	m_music.reset();
	m_stream.reset();
}


bool BGM::openFromFile(const std::string& filename) {
	m_music->stop();

	std::unique_ptr<sf::FileInputStream> stream = std::make_unique<sf::FileInputStream>();
	if (!stream->open(filename)) {
		sf::err() << "ohms::audio::BGM: open file stream failed" << std::endl;
		return false;
	}

	OHMSAUDIOCOMMENTSTRUCTURE data;
	if (!readMusicLoopPointStrict(*stream, data)) {
		sf::err() << "ohms::audio::BGM: read comment failed" << std::endl;
		return false;
	}

	if (!m_music->openFromStream(*stream)) {
		sf::err() << "ohms::audio::BGM: Music open failed" << std::endl;
		return false;
	}

	m_music->setLoopPoints(sf::Music::TimeSpan(sf::microseconds(data.offset), sf::microseconds(data.length)));
	m_music->setLoop(true);

	m_stream = std::move(stream);
	return true;
}


void BGM::play() {
	return m_music->play();
}


void BGM::pause() {
	return m_music->pause();
}


void BGM::stop() {
	return m_music->pause();
}


sf::Music::Status BGM::getStatus() const {
	return m_music->getStatus();
}


bool BGM::getLoop() const {
	return m_music->getLoop();
}


void BGM::setLoop(bool loop) {
	return m_music->setLoop(loop);
}


sf::Music::TimeSpan BGM::getLoopPoints() const {
	return m_music->getLoopPoints();
}


float BGM::getVolume() const {
	return m_music->getVolume();
}


void BGM::setVolume(float volume) {
	return m_music->setVolume(volume);
}


sf::Time BGM::getPlayingOffset() const {
	return m_music->getPlayingOffset();
}


void BGM::setPlayingOffset(sf::Time timeOffset) {
	return m_music->setPlayingOffset(timeOffset);
}


float BGM::getPitch() const {
	return m_music->getPitch();
}


void BGM::setPitch(float pitch) {
	return m_music->setPitch(pitch);
}


sf::Vector3f BGM::getPosition() const {
	return m_music->getPosition();
}


void BGM::setPosition(float x, float y, float z) {
	return m_music->setPosition(x, y, z);
}


void BGM::setPosition(const sf::Vector3f& position) {
	return m_music->setPosition(position);
}


bool BGM::isRelativeToListener() const {
	return m_music->isRelativeToListener();
}


void BGM::setRelativeToListener(bool relative) {
	return m_music->setRelativeToListener(relative);
}


float BGM::getMinDistance() const {
	return m_music->getMinDistance();
}


void BGM::setMinDistance(float distance) {
	return m_music->setMinDistance(distance);
}


float BGM::getAttenuation() const {
	return m_music->getAttenuation();
}


void BGM::setAttenuation(float attenuation) {
	return m_music->setAttenuation(attenuation);
}


sf::Time BGM::getDuration() const {
	return m_music->getDuration();
}


unsigned int BGM::getChannelCount() const {
	return m_music->getChannelCount();
}


unsigned int BGM::getSampleRate() const {
	return m_music->getSampleRate();
}


} // namespace audio
} // namespace ohms


/*
bool readMusicLoopPoint(sf::InputStream& stream, OHMSAUDIOCOMMENTSTRUCTURE& data) {
	unsigned char tmp[256];
	size_t length = 256;
	if (!getMusicOggCommentData(stream, tmp, length)) {
		return false;
	}
	for (size_t i = 0; i < length; ++i) {
		if ('A' <= tmp[i] && tmp[i] <= 'Z') {
			tmp[i] = tmp[i] - 'A' + 'a';
		}
	}

	// <ohms{test=114514;offset:12345678;length:987654321;}>
	bool got = false;
	size_t st = 0;
	size_t ed = 0;
	for (size_t i = 0, n = length - 6; i < n; ++i) {
		if (tmp[i] == '<' && tmp[i + 1] == 'o' && tmp[i + 2] == 'h' &&
			tmp[i + 3] == 'm' && tmp[i + 4] == 's' && tmp[i + 5] == '{') {
			for (size_t j = i + 7; j < length; ++j) {
				if (tmp[j] == '>') {
					st = i + 5;
					ed = j - 1;
					got = true;
					break;
				}
			}
			if (got) {
				break;
			}
		}
	}
	if (!got) {
		sf::err() << "BGM: OGG Comment have no valid data." << std::endl;
		return false;
	}

	//for (size_t i = st + 1; i < ed; ++i) {
	//	printf_s("%c", tmp[i]);
	//}
	//printf_s("\n");

	// no using looppoint
	if (st + 1 == ed) return true;

	for (size_t i = st + 1; i < ed; ++i) {
		size_t p0 = i;
		size_t p1 = 0;
		bool asBin = false;
		for (; i < ed; ++i) {
			if (tmp[i] == ':') {
				p1 = i;
				break;
			}
			else if (tmp[i] == '=') {
				p1 = i;
				asBin = true;
				break;
			}
			else if (tmp[i] == ';') {
				return false;
			}
		}
		if (i == ed) {
			return false;
		}
		if (p1 == 0) {
			return false;
		}
		if (p1 - p0 == 6) {
			size_t val = 0;
			// offset
			if (tmp[p0] == 'o' && tmp[p0 + 1] == 'f' && tmp[p0 + 2] == 'f' &&
				tmp[p0 + 3] == 's' && tmp[p0 + 4] == 'e' && tmp[p0 + 5] == 't') {
				if (asBin) {
					sf::err() << "Binary comment not completed yet." << std::endl;
					return false;
				}
				else {
					for (++i; i < ed; ++i) {
						if (tmp[i] == ';') {
							break;
						}
						val = val * 10 + tmp[i] - '0';
					}
					if (i == ed) {
						return false;
					}
					data.offset = val;
				}
			}
			// length
			else if (tmp[p0] == 'l' && tmp[p0 + 1] == 'e' && tmp[p0 + 2] == 'n' &&
					 tmp[p0 + 3] == 'g' && tmp[p0 + 4] == 't' && tmp[p0 + 5] == 'h') {
				if (asBin) {
					sf::err() << "Binary comment not completed yet." << std::endl;
					return false;
				}
				else {
					for (++i; i < ed; ++i) {
						if (tmp[i] == ';') {
							break;
						}
						val = val * 10 + tmp[i] - '0';
					}
					if (i == ed) {
						return false;
					}
					data.length = val;
				}
			}
		}
		else {
			for (; i < ed; ++i) {
				if (tmp[i] == ';') {
					break;
				}
			}
			if (i == ed) {
				return false;
			}
		}
	}
	//printf_s("get offset: %llu, length: %llu\n", data.offset, data.length);
	return true;
}*/

/*
if (data.offset) {
	if (data.length) {
		m_music->setLoopPoints(sf::Music::TimeSpan(sf::microseconds(data.offset), sf::microseconds(data.length)));
	}
	else {
		sf::Time off = sf::microseconds(data.offset);
		m_music->setLoopPoints(sf::Music::TimeSpan(off,
							   m_music->getDuration() - off - sf::microseconds(4420000)));
		//m_music->setLoopPoints(sf::Music::TimeSpan(sf::microseconds(data.offset),
		//					   sf::microseconds(137739184) - sf::microseconds(data.offset)));

		//m_music->setLoopPoints(sf::Music::TimeSpan(sf::microseconds(data.offset),
		//					   m_music->getDuration() - sf::microseconds(data.offset) - sf::milliseconds(8)));
	}
}*/

				/*
				if (stream.read(tmp, 8) == -1) return false;
				// not 'COMMENT='
				if (!(tmp[0] == 'C' && tmp[1] == 'O' && tmp[2] == 'M' && tmp[3] == 'M' &&
					tmp[4] == 'E' && tmp[5] == 'N' && tmp[6] == 'T' && tmp[7] == '=')) {
					if (stream.seek(pos + 4 + tmplength) == -1) return false;
					continue;
				}
				// is 'COMMENT='
				tmplength = tmplength - 8;
				*/
				/*
				if (tmplength < 17) {
					if (stream.seek(pos + 4 + tmplength) == -1) return false;
					continue;
				}

				if (stream.read(tmp, 12) == -1) return false;
				// not 'DESCRIPTION='
				if (!(tmp[0] == 'D' && tmp[1] == 'E' && tmp[2] == 'S' && tmp[3] == 'C' &&
					tmp[4] == 'R' && tmp[5] == 'I' && tmp[6] == 'P' && tmp[7] == 'T' &&
					tmp[8] == 'I' && tmp[9] == 'O' && tmp[10] == 'N' && tmp[11] == '=')) {
					if (stream.seek(pos + 4 + tmplength) == -1) return false;
					continue;
				}
				// is 'DESCRIPTION='
				tmplength = tmplength - 12;*/
