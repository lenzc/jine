//Jingle player for Ultimate Frisbee tournaments
//Author: Christian Lenz <chrislenz@mailbox.org>

#include "jine.h"

#include <boost/algorithm/string.hpp>
#include <fstream>
#include <sstream>

#define CPPHTTPLIB_THREAD_POOL_COUNT 2
#include "third_party/httplib.h"

#define SUMMERTIME true

static bool fileExists(const std::string& path)
{
    std::ifstream f(path);
    return f.good();
}

static const char* INDEX_HTML = R"HTML(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Jine</title>
<style>
  :root { color-scheme: dark; }
  body { background:#111; color:#eee; font-family:sans-serif; margin:0; padding:1rem; }
  #titlebar { display:flex; align-items:baseline; justify-content:space-between; margin-bottom:.5rem; }
  #titlebar h1 { font-size:1.3rem; margin:0; }
  #clock { font-size:1.6rem; font-variant-numeric: tabular-nums; }
  #stale, #missing { display:none; background:#5a2a2a; color:#fff; padding:.5rem .75rem; border-radius:.5rem; margin-bottom:.75rem; font-weight:bold; }
  .row { display:flex; align-items:center; gap:.75rem; margin:.75rem 0; flex-wrap:wrap; }
  button { font-size:1.3rem; padding:.6rem 1.1rem; border:none; border-radius:.5rem;
           background:#2a2a2a; color:#eee; }
  button:active { background:#3a3a3a; }
  button:disabled { opacity:.4; }
  #mute.active { background:#5a2a2a; }
  h2 { font-size:1.05rem; color:#bbb; margin:1.5rem 0 .25rem; }
  table { border-collapse:collapse; width:100%; margin-top:.5rem; font-size:.95rem; }
  td,th { text-align:left; padding:.3rem .5rem; border-bottom:1px solid #333; }
  tr.active td { background:#2a3a2a; font-weight:bold; }
  tr.missing td { color:#e88; }
  .remaining { color:#8f8; font-weight:normal; }
  .play-btn { font-size:.85rem; padding:.3rem .6rem; background:#254a25; }
  #vol { min-width:3.5rem; text-align:center; font-size:1.3rem; }
  #dayover { color:#e88; font-weight:bold; }
</style>
</head>
<body>
<div id="titlebar">
  <h1>Jine</h1>
  <span id="clock">--:--:--</span>
</div>
<div id="stale">No update from jine in a while &mdash; connection may be lost.</div>
<div id="missing"></div>
<div id="dayover"></div>
<div class="row">
  <button onclick="vol(-5)">vol -</button>
  <span id="vol">--</span>
  <button onclick="vol(5)">vol +</button>
  <button id="mute" onclick="mute()">mute jingles</button>
</div>
<table>
  <thead><tr><th>Game</th><th>Jingle</th><th>Time</th></tr></thead>
  <tbody id="jingles"></tbody>
</table>

<h2>Loaded Jingles</h2>
<table>
  <thead><tr><th>Name</th><th>Path</th><th></th></tr></thead>
  <tbody id="loaded"></tbody>
</table>

<script>
let lastSuccess = Date.now();

function fmtRemaining(s) {
  const m = Math.floor(s.remainingSec / 60);
  const sec = s.remainingSec % 60;
  return s.playing ? '▶ playing' : '(in ' + m + ':' + String(sec).padStart(2, '0') + ')';
}

async function refresh() {
  try {
    const r = await fetch('/status');
    if (!r.ok) throw new Error('bad status');
    render(await r.json());
    lastSuccess = Date.now();
  } catch (e) {}
  document.getElementById('stale').style.display =
    (Date.now() - lastSuccess > 5000) ? 'block' : 'none';
}

function render(s) {
  document.getElementById('clock').textContent = s.time;
  document.getElementById('vol').textContent = s.vol + '%';
  document.getElementById('dayover').textContent = s.dayOver ? 'Day is over, no jingles left.' : '';

  const muteBtn = document.getElementById('mute');
  muteBtn.textContent = s.muted ? 'jingles muted' : 'mute jingles';
  muteBtn.classList.toggle('active', s.muted);

  const tbody = document.getElementById('jingles');
  tbody.innerHTML = s.jingles.map(j =>
    `<tr class="${j.active ? 'active' : ''}"><td>${j.game}</td><td>${j.name}</td>` +
    `<td>${j.time}${j.active ? ' <span class="remaining">' + fmtRemaining(s) + '</span>' : ''}</td></tr>`
  ).join('');

  const missingBox = document.getElementById('missing');
  const missing = s.loadedJingles.filter(g => !g.exists);
  missingBox.style.display = missing.length ? 'block' : 'none';
  missingBox.textContent = missing.length
    ? 'Jingle file(s) not found: ' + missing.map(g => g.name + ' (' + g.path + ')').join(', ')
    : '';

  const loadedBody = document.getElementById('loaded');
  loadedBody.innerHTML = s.loadedJingles.map(g =>
    `<tr class="${g.exists ? '' : 'missing'}"><td>${g.name}</td><td>${g.path}</td>` +
    `<td><button class="play-btn" ${g.exists ? '' : 'disabled'} onclick="playNow(${g.index})">play now</button></td></tr>`
  ).join('');
}

async function vol(delta) {
  const r = await fetch('/volume?delta=' + delta, { method: 'POST' });
  render(await r.json());
}
async function mute() {
  const r = await fetch('/mute', { method: 'POST' });
  render(await r.json());
}
async function playNow(index) {
  const r = await fetch('/play?index=' + index, { method: 'POST' });
  render(await r.json());
}

refresh();
setInterval(refresh, 1000);
</script>
</body>
</html>
)HTML";

Jine::Jine()
{
}


bool Jine::init(std::string app, std::string jingleFilePath, int maxVol, int httpPort)
{
    m_maxMusikVol = m_vol = maxVol;
    m_httpPort = httpPort;

	//load jingle file
	m_jingleFilePath = jingleFilePath;
	load_jingles();

	//search music source sink
	boost::algorithm::to_lower(app);

	std::vector<SinkInput> sinks = listSinkInputs();

	std::cout << "I found the following sinks:\n\n";
	int targetID = -1;
	std::string targetName;
	for(auto& sink : sinks)
	{
		std::string name = sink.appName;
		boost::algorithm::to_lower(name);

		if(name.find(app) != std::string::npos)
		{
			targetID = sink.index;
			targetName = sink.appName;
		}
		std::cout << "id: " << sink.index << " : " << sink.appName << "\n";
	}
	std::cout << "\n\n";

	if(targetID > -1)
	{
		m_sinkID = targetID;
		std::cout << "found target sink: " << targetName << " with id: " << m_sinkID << "\n";
		changeVol();
		startServer();
		return true;
	}
	else
	{
		std::cout << "did not find target sink!\n";
		return false;
	}

}

std::vector<Jine::SinkInput> Jine::listSinkInputs()
{
	//works against real PulseAudio and PipeWire's pipewire-pulse compat layer alike
	std::string output = exec("pactl list sink-inputs");

	std::vector<SinkInput> result;
	std::istringstream stream(output);
	std::string line;
	int currentIndex = -1;

	const std::string indexPrefix = "Sink Input #";
	const std::string namePrefix = "application.name = \"";

	while(std::getline(stream, line))
	{
		size_t indexPos = line.find(indexPrefix);
		if(indexPos != std::string::npos)
		{
			currentIndex = std::stoi(line.substr(indexPos + indexPrefix.length()));
			continue;
		}

		size_t namePos = line.find(namePrefix);
		if(namePos != std::string::npos && currentIndex > -1)
		{
			size_t start = namePos + namePrefix.length();
			size_t end = line.find("\"", start);
			result.push_back({currentIndex, line.substr(start, end - start)});
			currentIndex = -1;
		}
	}

	return result;
}

int Jine::stringToMin(std::string s)
{
	int posColon = s.find(":");


	int sign = 1;
	if(std::stoi(s.substr(0,posColon)) == 0)
		sign = 1 - (s[0] == '-') * 2;

	return sign * (std::stoi(s.substr(0,posColon)) * 60 + std::stoi(s.substr(posColon + 1, s.length() - posColon)));
}

void Jine::load_jingles()
{
	std::ifstream file(m_jingleFilePath);
	std::string line;

	//roll over time
// 	if(!std::getline(file, line))
// 	{
// 		std::cout << "Can't read jingle file!\n";
// 		std::abort();
// 	}
//
// 	while(line.size() < 1 || line[0] == '#')
// 	{
// 		if(!std::getline(file,line))
// 		{
// 			std::cout << "Can't read jingle file!\n";
// 			std::abort();
// 		}
// 	}


// 	m_rollover_min = stringToMin(line);

	//skipp empty lines
// 	while(std::getline(file, line))
// 	{
// 		if(line.length() < 1)
// 			break;
// 	}

	//read jingles for a single game
	while(std::getline(file, line))
	{
		if(line.length() < 1)
			break;

		Game game;
		game.jingle = line.substr(0,line.find(";"));

		std::string s = line.substr(game.jingle.length() + 1,line.length() - game.jingle.length() - 1);
		std::string sMin = s.substr(0,s.find(";"));

		game.relativeMin = stringToMin(sMin);
		game.path = s.substr(sMin.length() + 1 , s.length() - sMin.length() -1);
		m_gameInfo.push_back(game);
	}

	//read game infos and create jingles
	while(std::getline(file, line))
	{

		if(line.length() < 1)
			break;

		std::string gameName = line.substr(0, line.find(";"));
		int dayTimeInMin = stringToMin(line.substr(gameName.length() + 1, line.length() - gameName.length() - 1));

		for(auto& g : m_gameInfo)
		{
			m_jingles.push_back({g.jingle, gameName, g.path, dayTimeInMin + g.relativeMin});
		}
	}

}


void Jine::playJingle(Jingle jingle)
{
	std::cout << "starting jingle '" << jingle.name << "'\n";
	//mpg321's own audio output has been observed to hang indefinitely after playback finishes
	//(stuck in its output-buffer thread) on some PipeWire setups. Decoding to a WAV stream on
	//stdout and piping it into paplay - a proper PipeWire/Pulse client - avoids mpg321's audio
	//backend entirely. stdin is /dev/null so mpg321 can't pick up stray terminal keystrokes.
	std::string cmd_name = "mpg321 -q -v -w - " + jingle.path + " < /dev/null | paplay";
	exec(cmd_name.c_str());
}

void Jine::createJingle(std::string gameName, int hour, int min)
{
    int game_min = 60 * hour + min;

    m_jingles.push_back({"pre5", gameName, "pre5_kurz.mp3", game_min - 5});
    m_jingles.push_back({"start", gameName, "start_kurz.mp3", game_min});
    m_jingles.push_back({"5min", gameName, "5min_kurz.mp3", game_min + 85});
    m_jingles.push_back({"end", gameName, "end_kurz.mp3", game_min + 90});
}

void Jine::run()
{

    std::sort(m_jingles.begin(), m_jingles.end(), [](Jingle a, Jingle b) {
        return a.min < b.min;
    });

    printConsole();

	while(true)
	{
        bool shouldPlay = false;
        Jingle jingleToPlay;

        {
            std::lock_guard<std::recursive_mutex> lock(m_mutex);

            if(m_jingleID == m_jingles.size())
            {
                printConsole();
                std::cout << "--------------------\n";
                std::cout << "day is over. No jingles left. JingleID: " << m_jingleID << "\n";
                return;
            }

            //current time
            m_now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
            int h = (m_now / 3600) + 1;
            if(SUMMERTIME)
                h+=1;
            h = h % 24;
            int m = (m_now / 60) % 60;
            int s = m_now % 60;

            int s_today = h * 3600 + 60 * m + s;

            printConsole();

            if(s_today - m_jingles[m_jingleID].min * 60 > 30)
            {
                m_jingleID++;
                continue;
            }

            if(m_jingles[m_jingleID].min * 60 - 2 < s_today)
            {
                shouldPlay = true;
                jingleToPlay = m_jingles[m_jingleID];
            }
        }

        if(shouldPlay)
        {
            bool muted;
            {
                std::lock_guard<std::recursive_mutex> lock(m_mutex);
                muted = m_jingleMuted;
            }

            if(!muted)
                playWithFade(jingleToPlay);

            std::lock_guard<std::recursive_mutex> lock(m_mutex);
            m_jingleID++;
        }

        usleep(1000 * 1000);

    }
}


void Jine::printConsole()
{
    system("clear");
    int h = (m_now / 3600) + 1;
	if(SUMMERTIME)
		h += 1;
    h = h % 24;
    int m = (m_now / 60) % 60;
    int s = m_now % 60;

    std::string s_zero = s < 10? "0": "";
    std::string m_zero = m < 10? "0": "";

    std::cout << "Time: " << h << ":" << m_zero << m << ":" << s_zero << s ;
    std::cout << "\t musik vol: " << m_vol << "\n";
    if((size_t)m_jingleID < m_jingles.size())
    {
        auto& nextJingle = m_jingles[m_jingleID];
        std::cout << "\nNext Jingle: '" << nextJingle.name << "' at " << nextJingle.time() << "\n";
    }
    else
    {
        std::cout << "\nNo jingles left.\n";
    }

// 	std::cout << "rollover time: " << m_rollover_min << " mins\n";

    std::cout << "\n\n\tJingle list:\n\n ";
    std::cout << "Game\tJingle\tTime\n";

    for(unsigned int i=0;i<m_jingles.size();i++)
    {
        auto& j = m_jingles[i];
        if(i == m_jingleID)
            std::cout << "--------------------\n";
        std::cout << j.gameName << "\t" << j.name << "\t" << j.time() << "\n";
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
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

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

void Jine::startServer()
{
    std::thread([this]() {
        httplib::Server svr;

        svr.Get("/", [](const httplib::Request&, httplib::Response& res) {
            res.set_content(INDEX_HTML, "text/html");
        });

        svr.Get("/status", [this](const httplib::Request&, httplib::Response& res) {
            res.set_content(statusJson(), "application/json");
        });

        svr.Post("/volume", [this](const httplib::Request& req, httplib::Response& res) {
            int delta = 0;
            if(req.has_param("delta"))
                delta = std::stoi(req.get_param_value("delta"));
            adjustVolume(delta);
            res.set_content(statusJson(), "application/json");
        });

        svr.Post("/mute", [this](const httplib::Request&, httplib::Response& res) {
            toggleJingleMute();
            res.set_content(statusJson(), "application/json");
        });

        svr.Post("/play", [this](const httplib::Request& req, httplib::Response& res) {
            //played on its own thread so the request returns immediately and the
            //regular /status polling can show the "playing" progress live
            if(req.has_param("index"))
            {
                int index = std::stoi(req.get_param_value("index"));
                std::thread([this, index]() { playJingleNow(index); }).detach();
            }
            res.set_content(statusJson(), "application/json");
        });

        std::cout << "Web UI available at http://localhost:" << m_httpPort << "\n";
        svr.listen("0.0.0.0", m_httpPort);
    }).detach();
}

std::string Jine::statusJson()
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    auto esc = [](const std::string& s) {
        std::string out;
        for(char c : s)
        {
            if(c == '"' || c == '\\')
                out += '\\';
            out += c;
        }
        return out;
    };

    int h = (m_now / 3600) + 1;
    if(SUMMERTIME)
        h += 1;
    h = h % 24;
    int m = (m_now / 60) % 60;
    int s = m_now % 60;

    char timeBuf[16];
    snprintf(timeBuf, sizeof(timeBuf), "%d:%02d:%02d", h, m, s);

    bool dayOver = m_jingleID >= (int)m_jingles.size();

    int s_today = h * 3600 + 60 * m + s;
    int remainingSec = 0;
    if(!dayOver)
    {
        remainingSec = m_jingles[m_jingleID].min * 60 - s_today;
        if(remainingSec < 0)
            remainingSec = 0;
    }

    std::ostringstream out;
    out << "{";
    out << "\"time\":\"" << timeBuf << "\",";
    out << "\"vol\":" << m_vol << ",";
    out << "\"maxVol\":" << m_maxMusikVol << ",";
    out << "\"dayOver\":" << (dayOver ? "true" : "false") << ",";
    out << "\"playing\":" << (m_playing ? "true" : "false") << ",";
    out << "\"muted\":" << (m_jingleMuted ? "true" : "false") << ",";
    out << "\"remainingSec\":" << remainingSec << ",";
    out << "\"nextGame\":\"" << (dayOver ? "" : esc(m_jingles[m_jingleID].gameName)) << "\",";
    out << "\"nextName\":\"" << (dayOver ? "" : esc(m_jingles[m_jingleID].name)) << "\",";
    out << "\"jingles\":[";
    for(size_t i = 0; i < m_jingles.size(); i++)
    {
        auto& j = m_jingles[i];
        if(i)
            out << ",";
        out << "{\"game\":\"" << esc(j.gameName) << "\","
            << "\"name\":\"" << esc(j.name) << "\","
            << "\"time\":\"" << j.time() << "\","
            << "\"active\":" << (i == (size_t)m_jingleID ? "true" : "false") << "}";
    }
    out << "],";
    out << "\"loadedJingles\":[";
    for(size_t i = 0; i < m_gameInfo.size(); i++)
    {
        auto& g = m_gameInfo[i];
        if(i)
            out << ",";
        out << "{\"index\":" << i << ","
            << "\"name\":\"" << esc(g.jingle) << "\","
            << "\"path\":\"" << esc(g.path) << "\","
            << "\"exists\":" << (fileExists(g.path) ? "true" : "false") << "}";
    }
    out << "]}";
    return out.str();
}

void Jine::adjustVolume(int delta)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    m_vol += delta;
    if(m_vol < 0)
        m_vol = 0;
    if(m_vol > 100)
        m_vol = 100;
    changeVol();
}

void Jine::toggleJingleMute()
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    m_jingleMuted = !m_jingleMuted;
}

void Jine::setPlaying(bool playing)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    m_playing = playing;
}

void Jine::playWithFade(Jingle jingle)
{
    //serializes against the scheduled loop and other manual triggers so two jingles never overlap
    std::lock_guard<std::mutex> playbackLock(m_playbackMutex);

    setPlaying(true);
    fadeVol(m_maxMusikVol, 0, 2);
    playJingle(jingle);
    fadeVol(0, m_maxMusikVol, 2);
    setPlaying(false);
}

void Jine::playJingleNow(int index)
{
    Jingle jingle;
    {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        if(index < 0 || (size_t)index >= m_gameInfo.size())
            return;
        auto& g = m_gameInfo[index];
        jingle.name = g.jingle;
        jingle.path = g.path;
        jingle.min = 0;
    }

    playWithFade(jingle);
}


std::string Jine::Jingle::time()
{
    int h = min / 60;
    int m = min % 60;

    std::string zero = "";
    if(m<10) zero ="0";
    return std::to_string(h) + ":" + zero + std::to_string(m);
}


int main(int argc, char** argv) {

	if(argc < 2)
	{
		std::cout << "Usage: jine <music source name> [jingle file (default: jingles.txt)] [max volume (default: 50)] [web UI port (default: 8080)]\n";
		return 0;
	}


	std::string jingleFilePath = "jingles.txt";
	if(argc > 2)
		jingleFilePath = argv[2];

	Jine jine;

    int maxVol = 50;
	if(argc > 3)
        maxVol = std::stoi(argv[3]);

    int httpPort = 8080;
    if(argc > 4)
        httpPort = std::stoi(argv[4]);

	if(!jine.init(argv[1], jingleFilePath, maxVol, httpPort))
		return 0;

	jine.run();


	return 0;
}
