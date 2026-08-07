/*---------------------------------------------------------*\
| RGBController_AcerClevoKeyboard.cpp                       |
|                                                           |
|   RGBController for Acer Clevo keyboard backlight         |
|   Uses TUXEDO driver sysfs interface                      |
|                                                           |
|   This file is part of the OpenRGB project                |
|   SPDX-License-Identifier: GPL-2.0-or-later               |
\*---------------------------------------------------------*/

#include "RGBController_AcerClevoKeyboard.h"

enum AcerClevoKeyboardMode
{
    ACER_CLEVO_MODE_STATIC       = 0,
    ACER_CLEVO_MODE_BREATHING    = 1,
    ACER_CLEVO_MODE_WAVE         = 2,
    ACER_CLEVO_MODE_COLOR_CYCLE  = 3,
};

RGBController_AcerClevoKeyboard::RGBController_AcerClevoKeyboard(AcerClevoKeyboardController* controller_ptr)
{
    controller = controller_ptr;

    name        = controller->GetDeviceName();
    vendor      = controller->GetVendor();
    type        = DEVICE_TYPE_LAPTOP;
    description = "Acer Aspire A715-79G Keyboard Backlight";
    location    = controller->GetLocation();
    version     = "1.0.0";

    mode static_mode;
    static_mode.name            = "Static";
    static_mode.value           = ACER_CLEVO_MODE_STATIC;
    static_mode.flags           = MODE_FLAG_HAS_PER_LED_COLOR | MODE_FLAG_HAS_BRIGHTNESS;
    static_mode.color_mode      = MODE_COLORS_PER_LED;
    static_mode.speed_min       = 0;
    static_mode.speed_max       = 0;
    static_mode.speed           = 0;
    static_mode.brightness_min  = 0;
    static_mode.brightness_max  = 255;
    static_mode.brightness      = 255;
    modes.push_back(static_mode);

    mode breathing_mode;
    breathing_mode.name            = "Breathing";
    breathing_mode.value           = ACER_CLEVO_MODE_BREATHING;
    breathing_mode.flags           = MODE_FLAG_HAS_PER_LED_COLOR | MODE_FLAG_HAS_BRIGHTNESS | MODE_FLAG_HAS_SPEED;
    breathing_mode.color_mode      = MODE_COLORS_PER_LED;
    breathing_mode.speed_min       = 0;
    breathing_mode.speed_max       = 100;
    breathing_mode.speed           = 50;
    breathing_mode.brightness_min  = 0;
    breathing_mode.brightness_max  = 255;
    breathing_mode.brightness      = 255;
    modes.push_back(breathing_mode);

    mode wave_mode;
    wave_mode.name            = "Wave";
    wave_mode.value           = ACER_CLEVO_MODE_WAVE;
    wave_mode.flags           = MODE_FLAG_HAS_PER_LED_COLOR | MODE_FLAG_HAS_BRIGHTNESS | MODE_FLAG_HAS_SPEED | MODE_FLAG_HAS_DIRECTION_LR;
    wave_mode.color_mode      = MODE_COLORS_PER_LED;
    wave_mode.speed_min       = 0;
    wave_mode.speed_max       = 100;
    wave_mode.speed           = 50;
    wave_mode.brightness_min  = 0;
    wave_mode.brightness_max  = 255;
    wave_mode.brightness      = 255;
    modes.push_back(wave_mode);

    mode color_cycle_mode;
    color_cycle_mode.name            = "Color Cycle";
    color_cycle_mode.value           = ACER_CLEVO_MODE_COLOR_CYCLE;
    color_cycle_mode.flags           = MODE_FLAG_HAS_BRIGHTNESS | MODE_FLAG_HAS_SPEED;
    color_cycle_mode.color_mode      = MODE_COLORS_NONE;
    color_cycle_mode.speed_min       = 0;
    color_cycle_mode.speed_max       = 100;
    color_cycle_mode.speed           = 50;
    color_cycle_mode.brightness_min  = 0;
    color_cycle_mode.brightness_max  = 255;
    color_cycle_mode.brightness      = 255;
    modes.push_back(color_cycle_mode);

    animation_timer        = nullptr;
    animation_running      = false;
    animation_start_time   = std::chrono::steady_clock::now();

    SetupZones();
}

RGBController_AcerClevoKeyboard::~RGBController_AcerClevoKeyboard()
{
    StopAnimation();
    Shutdown();

    delete controller;
}

void RGBController_AcerClevoKeyboard::SetupZones()
{
    unsigned int count = controller->GetZoneCount();

    zones.resize(1);
    zones[0].type       = (count <= 1) ? ZONE_TYPE_SINGLE : ZONE_TYPE_LINEAR;
    zones[0].name       = "Keyboard";
    zones[0].leds_min   = count;
    zones[0].leds_max   = count;
    zones[0].leds_count = count;

    leds.resize(count);

    for(unsigned int i = 0; i < count; i++)
    {
        leds[i].name = (count <= 1) ? "Keyboard Backlight" : ("Zone " + std::to_string(i + 1));
        leds[i].value = i;
    }

    SetupColors();
}

void RGBController_AcerClevoKeyboard::DeviceUpdateLEDs()
{
    if(colors.empty())
    {
        return;
    }

    int mode = modes[active_mode].value;

    if(mode == ACER_CLEVO_MODE_STATIC)
    {
        RGBColor color = colors[0];
        unsigned char r = RGBGetRValue(color);
        unsigned char g = RGBGetGValue(color);
        unsigned char b = RGBGetBValue(color);

        controller->SetColor(r, g, b);
        controller->SetBrightness(modes[active_mode].brightness);
    }
    else
    {
        if(!animation_running)
        {
            StartAnimation();
        }
    }
}

void RGBController_AcerClevoKeyboard::DeviceUpdateZoneLEDs(int /*zone*/)
{
    DeviceUpdateLEDs();
}

void RGBController_AcerClevoKeyboard::DeviceUpdateSingleLED(int /*led*/)
{
    DeviceUpdateLEDs();
}

void RGBController_AcerClevoKeyboard::DeviceUpdateMode()
{
    int mode = modes[active_mode].value;

    if(mode == ACER_CLEVO_MODE_STATIC)
    {
        StopAnimation();
        DeviceUpdateLEDs();
    }
    else
    {
        animation_start_time = std::chrono::steady_clock::now();
        DeviceUpdateLEDs();
    }
}

void RGBController_AcerClevoKeyboard::DeviceSaveMode()
{
}

void RGBController_AcerClevoKeyboard::StartAnimation()
{
    if(animation_running)
    {
        return;
    }

    animation_running = true;
    animation_timer = new std::thread(&RGBController_AcerClevoKeyboard::AnimationThreadFunction, this);
}

void RGBController_AcerClevoKeyboard::StopAnimation()
{
    animation_running = false;

    if(animation_timer && animation_timer->joinable())
    {
        animation_timer->join();
    }

    delete animation_timer;
    animation_timer = nullptr;
}

void RGBController_AcerClevoKeyboard::AnimationThreadFunction()
{
    while(animation_running)
    {
        std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
        float elapsed = std::chrono::duration<float>(now - animation_start_time).count();

        int mode = modes[active_mode].value;
        float speed = static_cast<float>(modes[active_mode].speed) / 100.0f;
        float period = 2.0f + (1.0f - speed) * 4.0f;
        float phase = fmod(elapsed / period, 1.0f);

        RGBColor base_color = colors.empty() ? 0x00FFFFFF : colors[0];
        unsigned char r = RGBGetRValue(base_color);
        unsigned char g = RGBGetGValue(base_color);
        unsigned char b = RGBGetBValue(base_color);

        float brightness_factor = 1.0f;

        switch(mode)
        {
            case ACER_CLEVO_MODE_BREATHING:
            {
                brightness_factor = 0.5f * (1.0f + sinf(phase * 2.0f * (float)M_PI));
                break;
            }

            case ACER_CLEVO_MODE_WAVE:
            {
                float hue_shift = phase * 360.0f;

                float rf = r / 255.0f;
                float gf = g / 255.0f;
                float bf = b / 255.0f;

                float cmax = fmaxf(rf, fmaxf(gf, bf));
                float cmin = fminf(rf, fminf(gf, bf));
                float diff = cmax - cmin;

                float hue = 0.0f;
                if(diff > 0.001f)
                {
                    if(cmax == rf)
                    {
                        hue = 60.0f * fmodf((gf - bf) / diff, 6.0f);
                    }
                    else if(cmax == gf)
                    {
                        hue = 60.0f * ((bf - rf) / diff + 2.0f);
                    }
                    else
                    {
                        hue = 60.0f * ((rf - gf) / diff + 4.0f);
                    }
                    if(hue < 0)
                    {
                        hue += 360.0f;
                    }
                }

                hue = fmodf(hue + hue_shift, 360.0f);

                float sat = (cmax > 0.001f) ? (diff / cmax) : 0.0f;
                float val = cmax;

                float c = val * sat;
                float x = c * (1.0f - fabsf(fmodf(hue / 60.0f, 2.0f) - 1.0f));
                float m = val - c;

                if(hue < 60)
                {
                    r = (unsigned char)((c + m) * 255);
                    g = (unsigned char)((x + m) * 255);
                    b = (unsigned char)(m * 255);
                }
                else if(hue < 120)
                {
                    r = (unsigned char)((x + m) * 255);
                    g = (unsigned char)((c + m) * 255);
                    b = (unsigned char)(m * 255);
                }
                else if(hue < 180)
                {
                    r = (unsigned char)(m * 255);
                    g = (unsigned char)((c + m) * 255);
                    b = (unsigned char)((x + m) * 255);
                }
                else if(hue < 240)
                {
                    r = (unsigned char)(m * 255);
                    g = (unsigned char)((x + m) * 255);
                    b = (unsigned char)((c + m) * 255);
                }
                else if(hue < 300)
                {
                    r = (unsigned char)((x + m) * 255);
                    g = (unsigned char)(m * 255);
                    b = (unsigned char)((c + m) * 255);
                }
                else
                {
                    r = (unsigned char)((c + m) * 255);
                    g = (unsigned char)(m * 255);
                    b = (unsigned char)((x + m) * 255);
                }

                brightness_factor = 1.0f;
                break;
            }

            case ACER_CLEVO_MODE_COLOR_CYCLE:
            {
                float hue = phase * 360.0f;
                float c = 1.0f;
                float x = c * (1.0f - fabsf(fmodf(hue / 60.0f, 2.0f) - 1.0f));

                if(hue < 60)
                {
                    r = 255;
                    g = (unsigned char)(x * 255);
                    b = 0;
                }
                else if(hue < 120)
                {
                    r = (unsigned char)(x * 255);
                    g = 255;
                    b = 0;
                }
                else if(hue < 180)
                {
                    r = 0;
                    g = 255;
                    b = (unsigned char)(x * 255);
                }
                else if(hue < 240)
                {
                    r = 0;
                    g = (unsigned char)(x * 255);
                    b = 255;
                }
                else if(hue < 300)
                {
                    r = (unsigned char)(x * 255);
                    g = 0;
                    b = 255;
                }
                else
                {
                    r = 255;
                    g = 0;
                    b = (unsigned char)(x * 255);
                }

                brightness_factor = 1.0f;
                break;
            }

            default:
                break;
        }

        unsigned char final_r = (unsigned char)(r * brightness_factor);
        unsigned char final_g = (unsigned char)(g * brightness_factor);
        unsigned char final_b = (unsigned char)(b * brightness_factor);

        controller->SetColor(final_r, final_g, final_b);
        controller->SetBrightness(modes[active_mode].brightness);

        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
}
