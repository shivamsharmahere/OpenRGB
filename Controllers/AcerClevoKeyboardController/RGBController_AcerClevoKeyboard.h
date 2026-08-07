/*---------------------------------------------------------*\
| RGBController_AcerClevoKeyboard.h                         |
|                                                           |
|   RGBController for Acer Clevo keyboard backlight         |
|   Uses TUXEDO driver sysfs interface                      |
|                                                           |
|   This file is part of the OpenRGB project                |
|   SPDX-License-Identifier: GPL-2.0-or-later               |
\*---------------------------------------------------------*/

#pragma once

#include "RGBController.h"
#include "AcerClevoKeyboardController.h"
#include <thread>
#include <atomic>
#include <chrono>
#include <cmath>

class RGBController_AcerClevoKeyboard : public RGBController
{
public:
    RGBController_AcerClevoKeyboard(AcerClevoKeyboardController* controller_ptr);
    ~RGBController_AcerClevoKeyboard();

    void SetupZones();

    void DeviceUpdateLEDs() override;
    void DeviceUpdateZoneLEDs(int zone) override;
    void DeviceUpdateSingleLED(int led) override;
    void DeviceUpdateMode() override;
    void DeviceSaveMode() override;

private:
    AcerClevoKeyboardController*    controller;

    void                            StartAnimation();
    void                            StopAnimation();
    void                            AnimationThreadFunction();

    std::thread*                    animation_timer;
    std::atomic<bool>               animation_running;
    std::chrono::steady_clock::time_point
                                    animation_start_time;
};
