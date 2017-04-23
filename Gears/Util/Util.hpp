#pragma once

#define DISPLAY_VERSION "v4.2.0"

#include <string>
#include <vector>
#include "../../ScriptHookV_SDK/inc/types.h"

static std::vector<std::string> GameVersionString = {
	"VER_1_0_335_2_STEAM", // 00
	"VER_1_0_335_2_NOSTEAM", // 01

	"VER_1_0_350_1_STEAM", // 02
	"VER_1_0_350_2_NOSTEAM", // 03

	"VER_1_0_372_2_STEAM", // 04
	"VER_1_0_372_2_NOSTEAM", // 05

	"VER_1_0_393_2_STEAM", // 06
	"VER_1_0_393_2_NOSTEAM", // 07

	"VER_1_0_393_4_STEAM", // 08
	"VER_1_0_393_4_NOSTEAM", // 09

	"VER_1_0_463_1_STEAM", // 10
	"VER_1_0_463_1_NOSTEAM", // 11

	"VER_1_0_505_2_STEAM", // 12
	"VER_1_0_505_2_NOSTEAM", // 13
	
	"VER_1_0_573_1_STEAM", // 14
	"VER_1_0_573_1_NOSTEAM", // 15

	"VER_1_0_617_1_STEAM", // 16
	"VER_1_0_617_1_NOSTEAM", // 17

	"VER_1_0_678_1_STEAM", // 18
	"VER_1_0_678_1_NOSTEAM", // 19

	"VER_1_0_757_2_STEAM", // 20
	"VER_1_0_757_2_NOSTEAM", // 21

	"VER_1_0_757_4_STEAM", // 22
	"VER_1_0_757_4_NOSTEAM", // 23

	"VER_1_0_791_2_STEAM", // 24
	"VER_1_0_791_2_NOSTEAM", // 25

	"VER_1_0_877_1_STEAM", // 26
	"VER_1_0_877_1_NOSTEAM", // 27

	"VER_1_0_944_2_STEAM", // 28
	"VER_1_0_944_2_NOSTEAM", // 29

	"VER_1_0_1011_1_STEAM", // 30
	"VER_1_0_1011_1_NOSTEAM", // 31

	"VER_1_0_1032_1_STEAM", // 32
	"VER_1_0_1032_1_NOSTEAM", // 33
};

enum G_GameVersion : int {
	G_VER_1_0_335_2_STEAM, // 00
	G_VER_1_0_335_2_NOSTEAM, // 01
	
	G_VER_1_0_350_1_STEAM, // 02
	G_VER_1_0_350_2_NOSTEAM, // 03
	
	G_VER_1_0_372_2_STEAM, // 04
	G_VER_1_0_372_2_NOSTEAM, // 05
	
	G_VER_1_0_393_2_STEAM, // 06
	G_VER_1_0_393_2_NOSTEAM, // 07
	
	G_VER_1_0_393_4_STEAM, // 08
	G_VER_1_0_393_4_NOSTEAM, // 09
	
	G_VER_1_0_463_1_STEAM, // 10
	G_VER_1_0_463_1_NOSTEAM, // 11
	
	G_VER_1_0_505_2_STEAM, // 12
	G_VER_1_0_505_2_NOSTEAM, // 13
	
	G_VER_1_0_573_1_STEAM, // 14
	G_VER_1_0_573_1_NOSTEAM, // 15
	
	G_VER_1_0_617_1_STEAM, // 16
	G_VER_1_0_617_1_NOSTEAM, // 17
	
	G_VER_1_0_678_1_STEAM, // 18
	G_VER_1_0_678_1_NOSTEAM, // 19
	
	G_VER_1_0_757_2_STEAM, // 20
	G_VER_1_0_757_2_NOSTEAM, // 21
	
	G_VER_1_0_757_4_STEAM, // 22
	G_VER_1_0_757_4_NOSTEAM, // 23
	
	G_VER_1_0_791_2_STEAM, // 24
	G_VER_1_0_791_2_NOSTEAM, // 25
	
	G_VER_1_0_877_1_STEAM, // 26
	G_VER_1_0_877_1_NOSTEAM, // 27
	
	G_VER_1_0_944_2_STEAM, // 28
	G_VER_1_0_944_2_NOSTEAM, // 29

	G_VER_1_0_1011_1_STEAM, // 30
	G_VER_1_0_1011_1_NOSTEAM, // 31

	G_VER_1_0_1032_1_STEAM, // 32
	G_VER_1_0_1032_1_NOSTEAM, // 33
};

static std::string eGameVersionToString(int version) {
	if (version > GameVersionString.size() - 1) {
		return std::to_string(version);
	}
	return GameVersionString[version];
}

struct Color {
	int R;
	int G;
	int B;
	int A;
};

// Natives called
void showText(float x, float y, float scale, const char* text, int font = 0, const Color &rgba = { 255, 255, 255, 255 });
void showNotification(const char* message, int *prevNotification);
void showSubtitle(std::string message, int duration = 2500);

//http://stackoverflow.com/questions/36789380/how-to-store-a-const-char-to-a-char
class CharAdapter
{
public:
	CharAdapter(const char* s) : m_s(::_strdup(s)) { }
	CharAdapter(const CharAdapter& other) = delete; // non construction-copyable
	CharAdapter& operator=(const CharAdapter&) = delete; // non copyable
	
	~CharAdapter() /*free memory on destruction*/
	{
		::free(m_s); /*use free to release strdup memory*/
	}
	operator char*() /*implicit cast to char* */
	{
		return m_s;
	}

private:
	char* m_s;
};

//https://github.com/CamxxCore/AirSuperiority
class GameSound {
public:
	GameSound(char *sound, char *soundSet);
	~GameSound();
	void Load(char *audioBank);
	void Play(Entity ent);
	void Stop();

	bool Active;

private:
	char *m_soundSet;
	char *m_sound;
	int m_soundID;
	int m_prevNotification;
};
