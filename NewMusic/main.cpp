

#include <SFML/System/Clock.hpp>

#include <SFML/Audio.hpp>

#include "BGM.h"

#include <Windows.h>


void timeTo3Nums(sf::Time time, int& min, int& sec, int& subsec) {
	sf::Int32 t = time.asMilliseconds();
	t /= 10;
	t = t / 10 + ((t % 10) >= 5);

	subsec = t % 10;
	t /= 10;
	sec = t % 60;
	t /= 60;
	min = t;
	return;
}


int main(int argc, char* argv[]) {
	HANDLE hStdOut;
	hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);

	ohms::audio::BGM bgm;

	if (argc >= 2) {
		if (!bgm.openFromFile(argv[1])) {
			MessageBoxW(NULL, L"Open music failed!", L"NewMusic.exe: ERROR", MB_ICONERROR);
			return 1;
		}
		system("cls");
		printf_s("Playing: \'%s\'\n\n", argv[1]);
	}
	else {
		char tfn[256] = { 0 };
		printf_s("Input music filename:\n");
		scanf_s("%s", tfn, 256);
		if (!bgm.openFromFile(tfn)) {
			MessageBoxW(NULL, L"Open music failed!", L"NewMusic.exe: ERROR", MB_ICONERROR);
			return 1;
		}
		system("cls");
		printf_s("Playing: \'%s\'\n\n", tfn);
	}

	int t[6] = { 0 };
	timeTo3Nums(bgm.getDuration(), t[0], t[1], t[2]);

	wchar_t wbuffer[64] = { 0 };

	sf::Music::TimeSpan loopPoints = bgm.getLoopPoints();
	long long testp[2];
	testp[0] = loopPoints.offset.asMicroseconds();
	testp[1] = testp[0] + loopPoints.length.asMicroseconds();
	printf("Loop Points: %02lld:%02lld.%03lld.%03lld : %02lld:%02lld.%03lld.%03lld \n\n",
		   testp[0] / 1000000 / 60, testp[0] / 1000000 % 60, testp[0] / 1000 % 1000, testp[0] % 1000,
		   testp[1] / 1000000 / 60, testp[1] / 1000000 % 60, testp[1] / 1000 % 1000, testp[1] % 1000);

	bgm.play();
#ifdef O_TEST_3
	bgm.setPlayingOffset(bgm.getDuration() - sf::seconds(4.0f + 3.0f));
#endif
#ifdef O_TEST_10
	bgm.setPlayingOffset(bgm.getDuration() - sf::seconds(4.0f + 10.0f));
#endif


	while (bgm.getStatus() == sf::Music::Playing) {
		timeTo3Nums(bgm.getPlayingOffset(), t[3], t[4], t[5]);
			
		swprintf_s(wbuffer, 64, L"\rPosition: %02d:%02d.%d / %02d:%02d.%d ", t[3], t[4], t[5], t[0], t[1], t[2]);
		WriteConsoleW(hStdOut, wbuffer, static_cast<DWORD>(wcslen(wbuffer)), NULL, NULL);

		Sleep(100);
	}

	system("pause");

	return 0;
}
