#include <stdio.h>
#include <string.h>
#include <iostream>
#include <unistd.h>
#include <chrono>
#include <thread>
#include <vector>

class Jine
{
public:

	Jine();

	bool init(std::string app);
	void run();
	
private:	
	
	//execute command
	std::string exec(const char* cmd);
	
	void changeVol(int id, int volPercent);
	void fadeVol(int id, int start, int end, float secs);
	
	void listInputs();
	
	int m_sinkID = -1;
	
	float m_cycleMins;
		
	struct Jingle
	{
		std::string name;
		std::string path;
		float startTime;
		float duration;
		
	};
	
	std::vector<Jingle> m_jingles;
		
	void wait(float secs, std::string nextAction);
	
	void playJingle(Jingle jingle);
	
};
