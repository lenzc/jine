//Jingle player for Ultimate Frisbee tournaments
//Author: Christian Lenz <chrislenz@mailbox.org>

#pragma once

#include <stdio.h>
#include <string>
#include <iostream>
#include <unistd.h>
#include <chrono>
#include <thread>
#include <vector>
#include <mutex>

#include <chrono>
#include <ctime>


class Jine
{
public:

	Jine();

	bool init(std::string app, std::string jingleFilePath, int maxVol, int httpPort = 8080);
	void run();

private:

	//execute command
	std::string exec(const char* cmd);

	std::string m_jingleFilePath;
	void load_jingles();
	int stringToMin(std::string s);

	void changeVol();
	void fadeVol(int start, int end, float secs);

	struct SinkInput
	{
		int index;
		std::string appName;
	};
	std::vector<SinkInput> listSinkInputs();

	//web UI
	void startServer();
	std::string statusJson();
	void adjustVolume(int delta);
	void playJingleNow(int index);
	void setPlaying(bool playing);
	void toggleJingleMute();

	int m_httpPort = 8080;
	std::recursive_mutex m_mutex;
	std::mutex m_playbackMutex;
	bool m_playing = false;
	bool m_jingleMuted = false;

	int m_sinkID = -1;

	int m_rollover_min = 0;

	struct Game
	{
		std::string jingle;
		std::string path;
		int relativeMin;
	};


	struct Jingle
	{
		std::string name;
		std::string gameName;
		std::string path;
        int min;

        std::string time();
	};

	std::vector<Game> m_gameInfo;
	std::vector<Jingle> m_jingles;

	void playJingle(Jingle jingle);
	void playWithFade(Jingle jingle);

    void createJingle(std::string gameName, int hour, int min);

	std::time_t m_now;
	int m_jingleID = 0;
	int m_maxMusikVol = 50;
	int m_vol = m_maxMusikVol;


	void printConsole();

};
