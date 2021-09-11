#include "jine.h"

#include <boost/algorithm/string.hpp>




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

void Jine::createJingle(std::string field, int hour, int min)
{
    int game_min = 60 * hour + min;
    
    m_jingles.push_back({"pre5", field, "pre5_kurz.mp3", game_min - 5});
    m_jingles.push_back({"start", field, "start_kurz.mp3", game_min});
    m_jingles.push_back({"5min", field, "5min_kurz.mp3", game_min + 85});
    m_jingles.push_back({"end", field, "end_kurz.mp3", game_min + 90});
}

void Jine::run()
{
    //Samstag
    createJingle("1 2", 9, 0);
    createJingle("1  ", 10, 40);
    createJingle("  2", 11, 40);
    createJingle("1 2", 13, 20);
    createJingle("1  ", 15, 30);
    createJingle("1 2", 17, 10);
    
    
    //Sonntag
//     createJingle("1 2", 9, 0);
//     createJingle("1  ", 10, 40);
//     createJingle("  2", 11, 40);
//     createJingle("1 2", 13, 50);
    
    
    
//     m_jingles.push_back({"test1", "1",  "end_kurz.mp3", 2*60 + 55});
    
    std::sort(m_jingles.begin(), m_jingles.end(), [](Jingle a, Jingle b) {
        return a.min < b.min;
    });
    
    printConsole();   
	
	while(true)
	{
        if(m_jingleID == m_jingles.size())
        {
            std::cout << "day is over. No jingles left. JingleID: " << m_jingleID << "\n";
            return;
        }
        
        //current time
        m_now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());      
        int h = (m_now / 3600) % 24 + 2;
        int m = (m_now / 60) % 60;
        int s = m_now % 60;
    
//         std::cout << h << ":" << m << ":" << s << "\n";
        
        int s_today = h * 3600 + 60 * m + s;
        
        printConsole();
        
        if(s_today - m_jingles[m_jingleID].min * 60 > 10)
        {
            m_jingleID++;
            continue;
        }
        
        if(m_jingles[m_jingleID].min * 60 - 2 < s_today)
        {
            fadeVol(m_max_musik_vol, 0, 2);
            playJingle(m_jingles[m_jingleID]);
            fadeVol(0, m_max_musik_vol, 2);
            m_jingleID++;
        }
        
        usleep(1000 * 1000);
        
    }
}


void Jine::printConsole()
{
    system("clear");
    int h = (m_now / 3600) % 24 + 2;
    int m = (m_now / 60) % 60;
    int s = m_now % 60;
    
    std::string s_zero = s < 10? "0": "";
    std::string m_zero = m < 10? "0": "";
    
    std::cout << "Time: " << h << ":" << m_zero << m << ":" << s_zero << s ;
    std::cout << "\t musik vol: " << m_vol << "\n";
    auto nextJingle = m_jingles[m_jingleID];
    std::cout << "\nNext Jingle: " << nextJingle.name << " at " << nextJingle.time() << "\n";
    
    std::cout << "\n\n\tJingle list:\n\n ";
    std::cout << "Field\tJingle\tTime\n";
    
    for(unsigned int i=0;i<m_jingles.size();i++)
    {
        auto& j = m_jingles[i];
        if(i == m_jingleID)
            std::cout << "--------------------\n";
        std::cout << j.field << "\t" << j.name << "\t" << j.time() << "\n";
    }
    
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

void Jine::changeVol()
{
	std::string cmd = "pactl -- set-sink-input-volume " + std::to_string(m_sinkID) + " " + std::to_string(m_vol) + "%";
	system(cmd.c_str());
// 	std::cout << "set id: " << id << "\t to " << m_vol << "% vol.\n";
}

void Jine::fadeVol(int start, int end, float secs)
{
    float delta_per_sec = (end-start) / secs;
       
    while(m_vol != end)
    {
        int delta_loop = (int) delta_per_sec / 10;
        m_vol += delta_loop;
        if(std::abs(m_vol - end) < std::abs(delta_loop))
            m_vol = end;
        
        changeVol();
        printConsole();
        usleep(100 * 1000);
        
        if(m_vol < 0 || m_vol > 100)
            return;
    }    
}


std::string Jine::Jingle::time()
{
    int h = min / 60;
    int m = min % 60;
    
    std::string zero = "";
    if(m<10) zero ="0";
    
    return std::to_string(h) + ":" + std::to_string(m) + zero;
}


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
