#pragma once

#include <SFML/Audio/Music.hpp>
#include <SFML/System/InputStream.hpp>
#include <SFML/System/fileInputStream.hpp>

#include <memory>

namespace ohms {
namespace audio {

class BGM final {
public:
	BGM();
	~BGM();

	[[nodiscard]] bool openFromFile(const std::filesystem::path& filename);

	void play();
	void pause();
	void stop();
	sf::Music::Status getStatus() const;

	bool getLoop() const;
	void setLoop(bool loop);
	sf::Music::TimeSpan getLoopPoints() const;

	float getVolume() const;
	void setVolume(float volume);

	sf::Time getPlayingOffset() const;
	void setPlayingOffset(sf::Time timeOffset);

	float getPitch() const;
	void setPitch(float pitch);

	sf::Vector3f getPosition() const;
	void setPosition(float x, float y, float z);
	void setPosition(const sf::Vector3f& position);

	bool isRelativeToListener() const;
	void setRelativeToListener(bool relative);

	float getMinDistance() const;
	void setMinDistance(float distance);

	float getAttenuation() const;
	void setAttenuation(float attenuation);

	sf::Time getDuration() const;
	unsigned int getChannelCount() const;
	unsigned int getSampleRate() const;

protected:
	std::unique_ptr<sf::Music> m_music;
	std::unique_ptr<sf::FileInputStream> m_stream;
};

} // namespace audio
} // namespace ohms
