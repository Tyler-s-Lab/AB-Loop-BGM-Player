#pragma once

#include <SFML/Audio/Music.hpp>
#include <filesystem>
#include <memory>

namespace sf {

class InputStream;

class Bgm final : private Music {
public:
	using Base = Music;

	Bgm();
	explicit Bgm(const std::filesystem::path& filename);
	Bgm(const void* data, std::size_t sizeInBytes);
	explicit Bgm(InputStream& stream);

	~Bgm() override;

	Bgm(Bgm&&) noexcept;
	Bgm& operator=(Bgm&&) noexcept;

	[[nodiscard]] bool openFromFile(const std::filesystem::path& filename);
	[[nodiscard]] bool openFromMemory(const void* data, std::size_t sizeInBytes);
	[[nodiscard]] bool openFromStream(InputStream& stream);

	////////////////////////////////////////// Music
	using Base::getDuration;
	using Base::getLoopPoints;
	//using Base::setLoopPoints;
	////////////////////////////////////////// Music

	////////////////////////////////////////// SoundStream
	using Base::play;
	using Base::pause;
	using Base::stop;
	using Base::getChannelCount;
	using Base::getSampleRate;
	using Base::getChannelMap;
	using Base::getStatus;
	using Base::setPlayingOffset;
	using Base::getPlayingOffset;
	using Base::setLooping;
	using Base::isLooping;
	using Base::setEffectProcessor;
	////////////////////////////////////////// SoundStream

	////////////////////////////////////////// SoundSource
	using Base::setPitch;
	using Base::setPan;
	using Base::setVolume;
	using Base::setSpatializationEnabled;
	using Base::setPosition;
	using Base::setDirection;
	using Base::setCone;
	using Base::setVelocity;
	using Base::setDopplerFactor;
	using Base::setDirectionalAttenuationFactor;
	using Base::setRelativeToListener;
	using Base::setMinDistance;
	using Base::setMaxDistance;
	using Base::setMinGain;
	using Base::setMaxGain;
	using Base::setAttenuation;
	using Base::getPitch;
	using Base::getPan;
	using Base::getVolume;
	using Base::isSpatializationEnabled;
	using Base::getPosition;
	using Base::getDirection;
	using Base::getCone;
	using Base::getVelocity;
	using Base::getDopplerFactor;
	using Base::getDirectionalAttenuationFactor;
	using Base::isRelativeToListener;
	using Base::getMinDistance;
	using Base::getMaxDistance;
	using Base::getMinGain;
	using Base::getMaxGain;
	using Base::getAttenuation;
	////////////////////////////////////////// SoundSource

protected:
	[[nodiscard]] bool onGetData(Chunk& data) override;
	void onSeek(Time timeOffset) override;
	std::optional<std::uint64_t> onLoop() override;

private:
	[[nodiscard]] std::uint64_t timeToSamples(Time position) const;

	struct Impl;
	std::unique_ptr<Impl> m_impl;
};

} // namespace sf
