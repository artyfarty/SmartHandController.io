// -------------------------------------------------------------------------------------------------------
// Handle encoders, both CW/CCW and Quadrature A/B types are supported

#include "Encoders.h"
#include "../../lib/tasks/OnTask.h"

#if defined(ESP32)
    #include <Esp.h>
#endif

#if ENCODER_SLEW_CONTROL == ON
void pollEncoders() { encoders.poll(); }
    
    #if SLEW_ENCODER_TYPE == AB
        #include "../../lib/encoder/quadrature/Quadrature.h"
Quadrature encControlAxis(SLEW_ENCODER_A_PIN, SLEW_ENCODER_B_PIN, 1);
    #elif SLEW_ENCODER_TYPE == AB_ESP32
        
        #include "../../lib/encoder/quadratureEsp32/QuadratureEsp32.h"

QuadratureEsp32 encControlAxis(SLEW_ENCODER_A_PIN, SLEW_ENCODER_B_PIN, 1);
    #elif SLEW_ENCODER_TYPE == CW_CCW
        #include "../../lib/encoder/cwCcw/CwCcw.h"
CwCcw encControlAxis(SLEW_ENCODER_A_PIN, SLEW_ENCODER_B_PIN, 1);
    #elif SLEW_ENCODER_TYPE == PULSE_DIR
        #include "../../lib/encoder/pulseDir/PulseDir.h"
PulseDir encControlAxis(SLEW_ENCODER_A_PIN, SLEW_ENCODER_B_PIN, 1);
    #elif SLEW_ENCODER_TYPE == AS37_H39B_B
        #include "../../lib/encoder/bissc/As37h39bb.h"
As37h39bb encControlAxis(SLEW_ENCODER_A_PIN, SLEW_ENCODER_B_PIN, 1);
    #elif SLEW_ENCODER_TYPE == JTW_24BIT
        #include "../../lib/encoder/bissc/Jtw24.h"
Jtw24 encControlAxis(SLEW_ENCODER_A_PIN, SLEW_ENCODER_B_PIN, 1);
    #endif
#endif


// ----------------------------------------------------------------------------------------------------------------
// background process position/rate control for encoders 

void Encoders::init(UI *ui_instance) {
    #if ENCODER_SLEW_CONTROL == ON
        encControlAxis.init();
        should_slew = false;
        is_slewing = false;
        ui = *ui_instance;
        
        VF("MSG: Encoders, start polling task (priority 4)... ");
        if (tasks.add(ENCODER_POLLING_RATE_MS, 0, true, 4, pollEncoders, "EncPoll")) { VLF("success"); }
        else {
            VLF("FAILED!");
        }
    #endif
}

#if ENCODER_SLEW_CONTROL == ON

void Encoders::poll() {
    long newPos = encControlAxis.read();
    unsigned long newTime = millis();
    //VF("MSG: encPos = "); VL(newPos);
    if (newPos == INT32_MAX) {
        enFault = true;
        newPos = 0;
        return;
    } else enFault = false;
    
    newPos *= SLEW_ENCODER_DIR;
    
    #if ENCODER_CONTROL_MODE == EC_SPD
        prevSpd3 = prevSpd2;
        prevSpd2 = prevSpd1;
        prevSpd1 = (static_cast<double>(newPos) - static_cast<double>(prevPos)) / ((newTime - prevTime));
    
    
        if (prevSpd1 != 0) {
            VF("MSG: prevSpd calc ("); V(newPos); V("-"); V(prevPos); V(") / ("); V(newTime); V("-"); V(prevTime); V(") = "); VL(prevSpd1);
        }
    
        prevTime = newTime;
        prevPos = newPos;
    #endif
    
    if (extraTicks > 0) {
        extraTicks--;
        return;
    }
    
    #if ENCODER_CONTROL_MODE == EC_POS
        if (prevPos != newPos) {
            should_slew = true;
            posDiff = newPos - prevPos;
            prevPos = newPos;
        } else {
            should_slew = false;
        }
    #endif
    
    #if ENCODER_CONTROL_MODE == EC_SPD
        if (prevSpd1 > 0 && prevSpd2 > 0 && prevSpd3 > 0) {
            should_slew = true;
            slew_dir = 1;
            VF("MSG: EncSlew+ "); V(prevSpd1); V(","); V(prevSpd2); V(","); VL(prevSpd3);
        } else if (prevSpd1 < 0 && prevSpd2 < 0 && prevSpd3 < 0) {
            should_slew = true;
            slew_dir = -1;
            VF("MSG: EncSlew- "); V(prevSpd1); V(","); V(prevSpd2); V(","); VL(prevSpd3);
        } else {
            should_slew = false;
        }
    #endif
    
    if (should_slew) {
        #if ENCODER_CONTROL_MODE == EC_POS
        ui.focusPull(posDiff);
        VF("MSG: FocusPull "); VL(posDiff);
        #endif
    
        #if ENCODER_CONTROL_MODE == EC_SPD
        
        orig_guide_rate = ui.getGuideRate();
        float encoderSpeed = ((abs(prevSpd1) + abs(prevSpd2) + abs(prevSpd3)) / 3); // steps/ms
        VF("MSG: EncSlew spd ");
        VL(encoderSpeed);
        float guideSpeed = encoderSpeed / 600 * 360 / 10;
        
        ui.setCustomGuideRate(guideSpeed);
        
        /*if (newTime - lastGuide > 800) {
            VF("MSG: EncSlew guide issued");
            lastGuide = newTime;
            ui.guide(SLEW_DIR_EAST * slew_dir);
        }*/
        
        if (!is_slewing) {
            ui.guide(SLEW_DIR_EAST * slew_dir);
        }
        is_slewing = true;
        #endif
        
    } else if (is_slewing) {
        VF("MSG: EncSlew stop");
        is_slewing = false;
        ui.guide(SLEW_STOP);
        ui.setGuideRate(orig_guide_rate);
    }
}

#endif

Encoders encoders;

