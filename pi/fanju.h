/**
 * @file fanju.h
 * @author Andreas Daasch
 * @brief Decoder for FanJu outdoor temperature/humidity sensor (MIT Licence)
 * 
 * @date 2022-01-10
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#ifndef FANJU_H
#define FANJU_H

#include <stdint.h>

namespace fanju
{

    typedef void (*FanJuCBFunc_t)(float temp, uint8_t hum, bool bat_ok, bool tx_req, uint8_t chan);

    int setup(FanJuCBFunc_t cb);

    void loop();
}

extern "C"
{
    typedef fanju::FanJuCBFunc_t FanJuCBFunc_t;
    int fanju_setup(FanJuCBFunc_t cb){return fanju::setup(cb);};
    void fanju_loop(){fanju::loop();};
}

#endif // FANJU_H
