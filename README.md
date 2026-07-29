## About The Project
Jine is a small jingle player for Ultimate Frisbee tournaments or other sport events. It runs alongside PulseAudio or PipeWire (via its `pipewire-pulse` compatibility layer), ducking your music source's volume to play jingles at the right time, and it comes with a small web UI so you can watch the schedule and adjust volume, mute jingles, or trigger one directly from a phone or tablet on the same network.

## Install
 1. Install required packages:
	```sh
	sudo apt install libboost-dev
	sudo apt install mpg321
	```
	Also requires `pactl` and `paplay` (both from `pulseaudio-utils`), which are normally already installed alongside PulseAudio or PipeWire. Jingles are decoded by `mpg321` and piped into `paplay` for actual playback, since `mpg321`'s own audio output has been observed to hang after playback finishes on some PipeWire setups.

 2. clone project
    ```sh
	git clone https://github.com/lenzc/jine.git
	```
 3. Compile
	```sh
	cd jine
	mkdir build && cd build
	cmake ..
	make
	```

## Usage
1. Start any music source (e.g. spotify)
2. Start jine
	```sh
	./jine <audio source name> [jingle file] [max volume] [web UI port]
	```
For example:
	```
	./jine spotify ../mixed_sa.txt 50 8080
	```
	Defaults: jingle file `jingles.txt`, max volume `50`, web UI port `8080`.

3. Open the web UI from any browser on the same network, e.g. `http://<this-machine's-ip>:8080`, to see the schedule and control volume/skip jingles.


## Jingle file
The jingle file contains the jingles to be played per game and all games for a day.

For each jingle add a line containing:
	`<Jingle name>;<jingle time relativ to game start>;<jingle path>`

Separate jingles and games by an empty line.

Add a line for each game containing:
	`<Game name>;<game start time>`
