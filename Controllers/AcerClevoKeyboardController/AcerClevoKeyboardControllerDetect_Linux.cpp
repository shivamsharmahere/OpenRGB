/*---------------------------------------------------------*\
| AcerClevoKeyboardControllerDetect_Linux.cpp               |
|                                                           |
|   Detector for Acer Clevo keyboard backlight              |
|   Requires TUXEDO driver sysfs interface                  |
|                                                           |
|   This file is part of the OpenRGB project                |
|   SPDX-License-Identifier: GPL-2.0-or-later               |
\*---------------------------------------------------------*/

#include <string>
#include <unistd.h>
#include "DetectionManager.h"
#include "AcerClevoKeyboardController.h"
#include "RGBController_AcerClevoKeyboard.h"

DetectedControllers DetectAcerClevoKeyboardControllers()
{
    DetectedControllers detected_controllers;

    std::string sysfs_base = "/sys/class/leds/rgb:kbd_backlight";

    if(access(sysfs_base.c_str(), F_OK) != 0)
    {
        return detected_controllers;
    }

    if(access(sysfs_base.c_str(), W_OK) != 0)
    {
        return detected_controllers;
    }

    AcerClevoKeyboardController*     controller     = new AcerClevoKeyboardController(sysfs_base);
    RGBController_AcerClevoKeyboard* rgb_controller = new RGBController_AcerClevoKeyboard(controller);

    detected_controllers.push_back(rgb_controller);

    return detected_controllers;
}

REGISTER_DETECTOR("Acer Clevo Keyboard", DetectAcerClevoKeyboardControllers);
