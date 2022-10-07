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
	./jine <audio source name> <jingle file>
	```
For example:
	```sh
	./jine spotify ../mixed_sa.txt
	```


## Jingle file
The jingle file contains the jingles to be played per game and all games for a day.

For each jingle add a line containing:
	`<Jingle name>;<jingle time relativ to game start>;<jingle path>`

Separate jingles and games by an empty line.

Add a line for each game containing:
	`<Game name>;<game start time>`
