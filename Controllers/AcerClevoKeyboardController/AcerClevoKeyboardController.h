/*---------------------------------------------------------*\
| AcerClevoKeyboardController.h                             |
|                                                           |
|   Hardware driver for Acer Clevo keyboard backlight       |
|   Communicates via Linux sysfs (TUXEDO drivers)           |
|                                                           |
|   This file is part of the OpenRGB project                |
|   SPDX-License-Identifier: GPL-2.0-or-later               |
\*---------------------------------------------------------*/

#pragma once

#include <string>
#include <vector>
#include <mutex>
#include "RGBController.h"

class AcerClevoKeyboardController
{
public:
    AcerClevoKeyboardController(const std::string& sysfs_path);
    ~AcerClevoKeyboardController();

    std::string     GetDeviceName();
    std::string     GetLocation();
    std::string     GetVendor();

    unsigned int    GetZoneCount();

    void            SetBrightness(unsigned char brightness);
    unsigned char   GetBrightness();

    void            SetColor(unsigned char red, unsigned char green, unsigned char blue);
    void            GetColor(unsigned char& red, unsigned char& green, unsigned char& blue);

private:
    std::string     sysfs_path;
    unsigned int    zone_count;
    unsigned char   brightness;
    unsigned char   color_r;
    unsigned char   color_g;
    unsigned char   color_b;
    std::mutex      mutex;

    bool            WriteSysfsFile(const std::string& filename, const std::string& value);
    std::string     ReadSysfsFile(const std::string& filename);
    unsigned int    DetectZoneCount();
};
