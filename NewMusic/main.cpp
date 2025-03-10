

#include <SFML/System/Clock.hpp>

#include <SFML/Audio.hpp>

#include "BGM.h"
#include "Console.h"

#include <iostream>
#include <format>

using namespace std;

struct Time {
	Time(sf::Time time) {
		auto t = (int)time.asMicroseconds();

		mic = t % 1000;
		t /= 1000;

		mil = t % 1000;

		t /= 10;
		t = t / 10 + (((t % 10) >= 5) ? 1 : 0);

		sub = t % 10;
		t /= 10;

		sec = t % 60;
		t /= 60;

		min = t;
	}

	int min;
	int sec;
	int sub;
	int mil;
	int mic;
};

int main(int argc, char* argv[]) {
	Console con;

	ohms::audio::BGM bgm;
	filesystem::path path;

	if (argc >= 2) {
		path = argv[1];
	}
	else {
		cout << "Input music filename:\n";
		cin >> path;
	}

	if (!bgm.openFromFile(path)) {
		con.ErrorMessage("Open music failed!");
		return 1;
	}
	con.Clear();
	cout << "Playing " << path << "\n\n";

	Time duration = bgm.getDuration();

	// Print loop interval.
	{
		sf::Music::TimeSpan loopPoints = bgm.getLoopPoints();
		Time offset = loopPoints.offset;
		Time finish = loopPoints.offset + loopPoints.length;
		cout << format(
			"Loop Points: {0:02}:{1:02}.{2:03}.{3:03} : {4:02}:{5:02}.{6:03}.{7:03}\n\n",
			offset.min, offset.sec, offset.mil, offset.mic,
			finish.min, finish.sec, finish.mil, finish.mic
		);
	}

	bgm.play();
#ifdef O_TEST_3
	bgm.setPlayingOffset(bgm.getDuration() - sf::seconds(3.0f + 3.0f));
#endif
#ifdef O_TEST_10
	bgm.setPlayingOffset(bgm.getDuration() - sf::seconds(3.0f + 10.0f));
#endif

	while (bgm.getStatus() == sf::Music::Status::Playing) {
		Time now = bgm.getPlayingOffset();

		con.Write(format(
			"\rPosition: {:02}:{:02}.{} / {:02}:{:02}.{} ",
			now.min, now.sec, now.sub,
			duration.min, duration.sec, duration.sub
		));

		sf::sleep(sf::milliseconds(100));
	}

	con.Pause();

	return 0;
}
