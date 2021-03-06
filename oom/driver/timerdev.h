//=========================================================
//  OOMidi
//  OpenOctave Midi and Audio Editor
//  $Id: timerdev.h,v 1.1.2.3 2005/08/21 18:11:28 spamatica Exp $
//
//  Plenty of code borrowed from timer.c example in 
//  alsalib 1.0.7
//
//  (C) Copyright 2004 Robert Jonsson (rj@spamatica.se)
//=========================================================

#ifndef __TIMERDEV_H__
#define __TIMERDEV_H__

#include "alsa/asoundlib.h"

#define TIMER_DEBUG 0

//---------------------------------------------------------
//   AlsaTimer
//---------------------------------------------------------

class Timer
{
public:

    Timer()
    {
    };

    virtual ~Timer()
    {
    };

    virtual signed int initTimer() = 0;
    virtual unsigned int setTimerResolution(unsigned int resolution) = 0;
    virtual unsigned int getTimerResolution() = 0;
    virtual unsigned int setTimerFreq(unsigned int freq) = 0;
    virtual unsigned int getTimerFreq() = 0;

    virtual bool startTimer() = 0;
    virtual bool stopTimer() = 0;
    virtual unsigned int getTimerTicks(bool printTicks = false) = 0;

};

#endif //__ALSATIMER_H__
