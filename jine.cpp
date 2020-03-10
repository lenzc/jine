
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <unistd.h>
#include <chrono>
#include <thread>


std::string exec(const char* cmd) {
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

void changeVol(int id, int volPercent)
{
	std::string cmd = "pactl -- set-sink-input-volume " + std::to_string(id) + " " + std::to_string(volPercent) + "%";
	system(cmd.c_str());
	std::cout << "set id: " << id << "\t to " << volPercent << "% vol.\n";
}


void listInputs()
{
	std::string cmd_name = R"EOS(pacmd list-sink-inputs | tr '\n' '\r' | perl -pe 's/ *index: ([0-9]+).+?application\.name = "([^\r]+)"\r.+?(?=index:|$)/\2\r/g' | tr '\r' '\n')EOS";

	std::string cmd_id = R"EOS(pacmd list-sink-inputs | tr '\n' '\r' | perl -pe 's/ *index: ([0-9]+).+?application\.name = "([^\r]+)"\r.+?(?=index:|$)/\1\r/g' | tr '\r' '\n')EOS";


	std::string name = exec(cmd_name.c_str());
	std::string id_str = exec(cmd_id.c_str());

	std::size_t pos = id_str.find("\n");
	id_str = id_str.substr(pos + 1, id_str.size() - pos - 2);

	pos = name.find("\n");
	name = name.substr(pos + 1, name.size() - pos - 2);

	int id = std::stoi(id_str);

	std::cout << "I got: " << name << "\nid: " << id << std::endl;

	changeVol(id, 80);
}


// perl -pe 's/ *index: ([0-9]+).+?application\.name = "([^\r]+)"\r.+?(?=index:|$)/\2:\1\r/g'



int main(int argc, char** argv) {

	listInputs();


	int vol = 0;
	int change = 1;

// 	while(1)
// 	{
// 		std::string cmd = "pactl -- set-sink-input-volume 161 " + std::to_string(vol) + "%";
// 		std::cout << cmd << std::endl;
// 		system(cmd.c_str());
//
// 		vol += change;
// 		if(vol < 20)
// 		{
// 			vol = 20;
// 			change *=-1;
// 		}
// 		if(vol > 70)
// 		{
// 			vol = 70;
// 			change *= -1;
// 		}
//
// 		std::this_thread::sleep_for(std::chrono::milliseconds(10));
// 	}



	return 0;
}
