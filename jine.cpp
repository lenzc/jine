#include "jine.h"

#include <boost/algorithm/string.hpp>

#include <chrono>
#include <ctime>    


Jine::Jine()
{
}


bool Jine::init(std::string app)
{
	boost::algorithm::to_lower(app); 
	
	std::string cmd_name = R"EOS(pacmd list-sink-inputs | tr '\n' '\r' | perl -pe 's/ *index: ([0-9]+).+?application\.name = "([^\r]+)"\r.+?(?=index:|$)/\2\r/g' | tr '\r' '\n')EOS";
	std::string cmd_id = R"EOS(pacmd list-sink-inputs | tr '\n' '\r' | perl -pe 's/ *index: ([0-9]+).+?application\.name = "([^\r]+)"\r.+?(?=index:|$)/\1\r/g' | tr '\r' '\n')EOS";


	std::string name = exec(cmd_name.c_str());
	std::string id_str = exec(cmd_id.c_str());

	std::vector<std::string> sinks;
	while(true)
	{
		size_t pos = name.find("\n");
		if((int)pos < 0)
			break;
		
		sinks.push_back(name.substr(0, pos));
		name = name.substr(pos + 1, name.size() - pos);
	}
	
	std::vector<std::string> ids;
	while(true)
	{
		size_t pos = id_str.find("\n");
		if((int)pos < 0)
			break;
		
		ids.push_back(id_str.substr(0, pos));
		id_str = id_str.substr(pos + 1, id_str.size() - pos);
	}
	
	if(ids.size() != sinks.size())
		std::runtime_error("not matching sinks and ids");
		
	std::cout << "I found the following sinks:\n\n";
	int targetIdx = -1;
	for(unsigned int i=1;i<sinks.size();i++)
	{
		std::string sink = sinks[i];
		
		boost::algorithm::to_lower(sink); 
		
		if((int)sink.find(app) >= 0)
			targetIdx = i;
		std::cout << "id: " << ids[i] << " : " << sinks[i] << "\n";
	}
	std::cout << "\n\n";
	
	if(targetIdx > -1)
	{
		m_sinkID = std::stoi(ids[targetIdx]);
		std::cout << "found target sink: " << sinks[targetIdx] << " with id: " << m_sinkID << "\n";
		return true;
	}
	else
	{
		std::cout << "did not find target sink!\n";
		return false;
	}	
}

void Jine::playJingle(Jingle jingle)
{
	std::cout << "starting jingle '" << jingle.name << "'\n";
	std::string cmd_name = "mpg321 -q -v jingles/" + jingle.path;	
	exec(cmd_name.c_str());		
}


void Jine::run()
{
	m_cycleMins = 1;
	
	m_jingles.push_back({"Jingle 1" , "j1.mp3", 0.1, 0.1});
	m_jingles.push_back({"Ende" , "j2.mp3", 0.3, 0.3});
	
	
	auto start = std::chrono::system_clock::now();
    std::time_t start_time = std::chrono::system_clock::to_time_t(start);
	std::cout << "programm startet at " << std::ctime(&start_time) << "\n";

	
	while(true)
	{
		for(auto& jingle : m_jingles)
		{
			std::chrono::duration<double> elapsed_seconds = std::chrono::system_clock::now() - start;
			float sleepSecs = jingle.startTime * 60 - elapsed_seconds.count();
			
			std::cout << "next Jingle: " << jingle.name << " sleep: " << sleepSecs << "\n";
			
			wait(sleepSecs, jingle.name);
// 			usleep((sleepSecs - 2 ) * 1000000);
			
			fadeVol(m_sinkID, 100, 0, 2);
			
			//start jingle
			playJingle(jingle);
// 			wait(jingle.duration, "Jingle end");
			
			fadeVol(m_sinkID, 0, 100, 2);
			
		}
		std::chrono::duration<double> elapsed_seconds = std::chrono::system_clock::now() - start;
		float sleepEndCycle = m_cycleMins * 60 - elapsed_seconds.count();
		
		wait(sleepEndCycle, "Cycle End");
			
		
		start = std::chrono::system_clock::now();
		
	}
	
}


void Jine::wait(float secs, std::string nextAction)
{
	auto start = std::chrono::system_clock::now();
	double elapsedSecs = 0;
	do
	{
		usleep(500 * 1000);
		std::chrono::duration<double> elapsed_seconds = std::chrono::system_clock::now() - start;
		elapsedSecs = elapsed_seconds.count();
		
		std::cout << nextAction << " in " << secs - elapsedSecs << "sec\n";
		
	}while(elapsedSecs < secs);
		
	
}


std::string Jine::exec(const char* cmd) {
	std::array<char, 128> buffer;
	std::string result;
	std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
	if (!pipe) {
		throw std::runtime_error("popen() failed!");
	}
	while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
		result += buffer.data();
	}
	return result;
}

void Jine::changeVol(int id, int volPercent)
{
	std::string cmd = "pactl -- set-sink-input-volume " + std::to_string(id) + " " + std::to_string(volPercent) + "%";
	system(cmd.c_str());
	std::cout << "set id: " << id << "\t to " << volPercent << "% vol.\n";
}

void Jine::fadeVol(int id, int start, int end, float secs)
{
    float delta_per_sec = (end-start) / secs;
    
//     std::cout << "delta: " << delta_per_sec << "\n";
    
    int vol = start;
    
    
    while(vol != end)
    {
        int delta_loop = (int) delta_per_sec / 10;
        vol += delta_loop;
        if(std::abs(vol - end) < std::abs(delta_loop))
            vol = end;
        
      
        changeVol(id, vol);
        usleep(100 * 1000);
        
        if(vol < 0 || vol > 100)
            return;
    }    
}

void Jine::listInputs()
{
	
	
	
	
/*	
	int id = std::stoi(id_str);

	std::cout << "I got: " << name << "\nid: " << id << std::endl;

// 	changeVol(id, 0);
//     sleep(1);
// 	changeVol(id, 80);
    while(true)
    {
		fadeVol(id, 100, 10, 2);
		fadeVol(id, 10, 100, 2);
	}*/
}


// perl -pe 's/ *index: ([0-9]+).+?application\.name = "([^\r]+)"\r.+?(?=index:|$)/\2:\1\r/g'



int main(int argc, char** argv) {

	if(argc < 2)
	{
		std::cout << "please specify target sink name!\n";
		return 0;
	}
	
	Jine jine;
	
	if(!jine.init(argv[1]))
		return 0;
	
	//read timetable
	
	//run
	jine.run();


	return 0;
}
