// This library is a copy of https://github.com/nettigo/Timers  version 16.4.1
// Used by AsyncSMS library
#include "Timers.h"

void Timers::start(uint32_t waitTime) {
	_waitTime = waitTime;
	restart();
}

void Timers::restart() {
	_destTime = millis() + _waitTime;
}

void Timers::stop() {
	_waitTime = 0;
}

bool Timers::available() {
	if (_waitTime == 0) {
		return false;
	}
	return millis() >= _destTime;
}
