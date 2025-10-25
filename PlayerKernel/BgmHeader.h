#pragma once

#include <SFML/Audio/Music.hpp>
#include <SFML/Audio/SoundStream.hpp>
#include <SFML/System/Time.hpp>
#include <any>
#include <ostream>

namespace bgm {

using Time = sf::Time;
using InputStream = sf::InputStream;
using InputSoundFile = sf::InputSoundFile;

using SoundStream = sf::SoundStream;
template <typename T>
using Span = sf::Music::Span<T>;

[[nodiscard]] std::ostream& err();
constexpr Time microseconds(std::int64_t amount) {
	return sf::microseconds(amount);
}

std::any readLoopPoints(InputStream& stream);

struct BGM_STREAM_EMPTY_DELETER {
	void operator()(InputStream* object) const;
};

}
