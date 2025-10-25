#pragma once

using namespace System;

#include "Bgm.h"

namespace PlayerKernel {

public ref class BgmWrapper {
private:
	bgm::Music* m_bgm;
	bool m_ok;

public:
	BgmWrapper();

	~BgmWrapper();

	void open(String^ filename);

	bool isOpened();

	void play();
	void pause();
	void stop();

	float getDuration();
	float getOffset();
	void setOffset(float seconds);

	float getLoopPointA();
	float getLoopPointB();

};

}
