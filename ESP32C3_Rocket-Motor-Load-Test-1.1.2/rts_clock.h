#ifndef RTS_CLOCK_H
#define RTS_CLOCK_H

#include <Arduino.h>

class RTCClock
{
public:

    RTCClock();

    void setDateTime(const String& iso);

    String getDateTime() const;

    bool valid() const;

private:

    String currentDateTime;

    bool hasTime;
};

extern RTCClock rtcClock;

#endif