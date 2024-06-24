// -------------------------------------------------------------------------------------------------------
// Handle encoders, both CW/CCW and Quadrature A/B types are supported
#pragma once

#include "../../Common.h"
#include "../../userInterface/UserInterface.h"

// encoder polling rate in milli-seconds
#ifndef ENCODER_POLLING_RATE_MS
  #define ENCODER_POLLING_RATE_MS 50
#endif


#define ENCODER_PREV_POSITIONS 3

class Encoders {
  public:
    // prepare encoders for operation, init NV if necessary
    void init(UI *ui_instance);

    #if ENCODER_SLEW_CONTROL == ON
      void poll();
    #endif
  private:
    bool enFault = false;

    u_short extraTicks = ENCODER_PREV_POSITIONS;
    long prevPos;
    long prevDiff1;
    long prevDiff2;
    long prevDiff3;

    double prevSpd1;
    double prevSpd2;
    double prevSpd3;

    bool is_slewing;
    bool should_slew;
    short slew_dir;
    uint8_t orig_guide_rate;
    unsigned long prevTime;
    unsigned long lastGuide;

    UI ui;
};

extern Encoders encoders;
