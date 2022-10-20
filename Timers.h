// This library is a copy of https://github.com/nettigo/Timers  version 16.4.1
// Used by AsyncSMS library
#ifndef Timers_h
#define Timers_h

#include <Arduino.h>
#include <inttypes.h>

class Timers
{
  public:
    Timers() {}
	
	void start(uint32_t waitTime);
	void restart();
	void stop();
	bool available();	
	
  private:
	uint32_t _waitTime;
	uint32_t _destTime;
};
#endif
