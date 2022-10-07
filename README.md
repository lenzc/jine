## About The Project
Jine is a small jingle player for Ultimate Frisbee tournaments or other sport events.

## Install
 1. clone project
    ```sh
	git clone https://github.com/lenzc/jine.git
	```
 2. Compile
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
	'./jine <audio source name> <jingle file>'
	```

## Jingle file
The jingle file contains the jingles to be played per game and all games for a day.

For each Jingle add a line containing:
	`<Jingle name>;<jingle time relativ to game start>;<jingle path>`

Separate Jingles and games by an empty line.

Add a line for each game containing:
	`<Game name>;<game start time>`
