#include "PlayerKernel.h"

PlayerKernel::BgmWrapper::BgmWrapper() {
	m_bgm = new bgm::Music;
	m_ok = false;
}

PlayerKernel::BgmWrapper::~BgmWrapper() {
	delete m_bgm;
	m_bgm = nullptr;
}

void PlayerKernel::BgmWrapper::open(String^ filename) {
	std::wstring str;
	if (filename != nullptr) {
		cli::array<wchar_t>^ wArray = filename->ToCharArray();
		int len = wArray->Length;
		str.resize(len);
		IntPtr pcstr(str.data());
		Runtime::InteropServices::Marshal::Copy(wArray, 0, pcstr, len);
	}

	if (!m_bgm->openFromFile(str)) {
		throw gcnew System::Exception(L"Failed to open");
	}

	m_ok = true;
}

bool PlayerKernel::BgmWrapper::isOpened() {
	return m_ok;
}

void PlayerKernel::BgmWrapper::play() {
	if (m_ok)
		m_bgm->play();
}

void PlayerKernel::BgmWrapper::pause() {
	m_bgm->pause();
}

void PlayerKernel::BgmWrapper::stop() {
	m_bgm->stop();
}

float PlayerKernel::BgmWrapper::getDuration() {
	return m_bgm->getDuration().asSeconds();
}

float PlayerKernel::BgmWrapper::getOffset() {
	return m_bgm->getPlayingOffset().asSeconds();
}

void PlayerKernel::BgmWrapper::setOffset(float seconds) {
	m_bgm->setPlayingOffset(sf::seconds(seconds));
}

UInt64 PlayerKernel::BgmWrapper::getLoopPointA() {
	return m_bgm->getLoopPoints().offset.asMicroseconds();
}

UInt64 PlayerKernel::BgmWrapper::getLoopPointB() {
	auto l = m_bgm->getLoopPoints();
	return (l.offset + l.length).asMicroseconds();
}
