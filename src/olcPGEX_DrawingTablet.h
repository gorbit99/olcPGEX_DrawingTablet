#pragma once

#include "olcPixelGameEngine.h"

#include <climits>
#include <optional>
#include <string>
#include <vector>

namespace X11 {
#include <X11/extensions/XI2.h>
#include <X11/extensions/XInput2.h>
} // namespace X11

class DrawingTablet : public olc::PGEX {
public:
    DrawingTablet(X11::XIDeviceInfo &device_info,
                  std::shared_ptr<X11::Display> display);

    DrawingTablet(const DrawingTablet &) = delete;
    DrawingTablet &operator=(const DrawingTablet &) = delete;

    enum class DeviceType {
        Stylus,
        Eraser,
    };

    std::string GetName() const;
    int GetDeviceID() const;
    size_t GetPossibleButtonCount() const;
    bool SupportsPressure() const;
    bool SupportsTilt() const;
    DeviceType GetType() const;

    bool OnBeforeUserUpdate(float &fElapsedTime) override;

    const olc::vf2d GetNormalizedPosition() const;
    const float GetPressure() const;
    const olc::vf2d GetTilt() const;
    const olc::HWButton GetButton(size_t button) const;

private:
    void pollDevice();

    int device_id;
    std::string name;
    DeviceType type;

    bool supports_pressure;
    bool supports_tilt;

    olc::vf2d position;
    float pressure;
    olc::vf2d tilt;
    std::vector<olc::HWButton> button_states;

    std::shared_ptr<X11::Display> display;
};

class DrawingTabletManager {
public:
    static std::shared_ptr<DrawingTabletManager> Get();
    DrawingTabletManager(const DrawingTabletManager &) = delete;
    DrawingTabletManager &operator=(const DrawingTabletManager &) = delete;

    const size_t GetTabletCount() const;
    std::shared_ptr<DrawingTablet> GetTablet(size_t minimum_button = 0,
                                             bool supports_pressure = false,
                                             bool supports_tilt = false);

private:
    DrawingTabletManager();

    std::vector<std::shared_ptr<DrawingTablet>> tablets;

    static std::shared_ptr<DrawingTabletManager> instance;
};

// #ifdef OLC_PGEX_DRAWINGTABLET
// #undef OLC_PGEX_DRAWINGTABLET

DrawingTablet::DrawingTablet(X11::XIDeviceInfo &device_info,
                             std::shared_ptr<X11::Display> display)
        : olc::PGEX{true},
          device_id{device_info.deviceid},
          display{display} {
    name = device_info.name;

    if (name.find("stylus") != std::string::npos) {
        type = DeviceType::Stylus;
    } else if (name.find("eraser") != std::string::npos) {
        type = DeviceType::Eraser;
    }

    for (size_t i = 0; i < static_cast<size_t>(device_info.num_classes); i++) {
        switch (device_info.classes[i]->type) {
        case XIButtonClass: {
            X11::XIButtonClassInfo *button_info =
                    reinterpret_cast<X11::XIButtonClassInfo *>(
                            device_info.classes[i]);
            button_states.resize(button_info->num_buttons);
            break;
        }
        case XIValuatorClass: {
            X11::XIValuatorClassInfo *valuator_info =
                    reinterpret_cast<X11::XIValuatorClassInfo *>(
                            device_info.classes[i]);
            if (valuator_info->number == 2) {
                supports_pressure = true;
            } else if (valuator_info->number == 3 || valuator_info->number == 4)
                supports_tilt = true;
        } break;
        }
    }
}

std::string DrawingTablet::GetName() const {
    return name;
}

int DrawingTablet::GetDeviceID() const {
    return device_id;
}

size_t DrawingTablet::GetPossibleButtonCount() const {
    return button_states.size();
}

bool DrawingTablet::SupportsPressure() const {
    return supports_pressure;
}

bool DrawingTablet::SupportsTilt() const {
    return supports_tilt;
}

DrawingTablet::DeviceType DrawingTablet::GetType() const {
    return type;
}

void DrawingTablet::pollDevice() {
    int num_devices;
    X11::XIDeviceInfo *device_info =
            X11::XIQueryDevice(display.get(), device_id, &num_devices);

    const auto normalizeRange = [](float value, float min, float max) {
        return (value - min) / (max - min);
    };

    for (size_t i = 0; i < static_cast<size_t>(device_info->num_classes); i++) {
        switch (device_info->classes[i]->type) {
        case XIValuatorClass: {
            X11::XIValuatorClassInfo *valuator_info =
                    reinterpret_cast<X11::XIValuatorClassInfo *>(
                            device_info->classes[i]);
            if (valuator_info->number == 0) {
                position.x = normalizeRange(valuator_info->value,
                                            valuator_info->min,
                                            valuator_info->max);
            } else if (valuator_info->number == 1) {
                position.y = normalizeRange(valuator_info->value,
                                            valuator_info->min,
                                            valuator_info->max);
            } else if (valuator_info->number == 2) {
                pressure = normalizeRange(valuator_info->value,
                                          valuator_info->min,
                                          valuator_info->max);
            } else if (valuator_info->number == 3) {
                tilt.x = normalizeRange(valuator_info->value,
                                        valuator_info->min,
                                        valuator_info->max);
            } else if (valuator_info->number == 4) {
                tilt.y = normalizeRange(valuator_info->value,
                                        valuator_info->min,
                                        valuator_info->max);
            }
            break;
        }
        case XIButtonClass: {
            X11::XIButtonClassInfo *button_info =
                    reinterpret_cast<X11::XIButtonClassInfo *>(
                            device_info->classes[i]);
            for (size_t j = 0; j < button_states.size(); j++) {
                button_states[j].bPressed = false;
                button_states[j].bReleased = false;

                auto byte_index = j / CHAR_BIT;
                auto bit_index = j % CHAR_BIT;
                auto current_mask = button_info->state.mask[byte_index];
                auto current_bit = (current_mask >> bit_index) & 1;
                if (!current_bit && button_states[j].bHeld) {
                    button_states[j].bHeld = false;
                    button_states[j].bReleased = true;
                } else if (current_bit && !button_states[j].bHeld) {
                    button_states[j].bHeld = true;
                    button_states[j].bPressed = true;
                }
            }
            break;
        }
        }
    }
    X11::XIFreeDeviceInfo(device_info);
}

bool DrawingTablet::OnBeforeUserUpdate(float &fElapsedTime) {
    pollDevice();
    return false;
}

const olc::vf2d DrawingTablet::GetNormalizedPosition() const {
    return position;
}

const float DrawingTablet::GetPressure() const {
    return pressure;
}

const olc::vf2d DrawingTablet::GetTilt() const {
    return tilt;
}

const olc::HWButton DrawingTablet::GetButton(size_t button) const {
    return button_states[button];
}

std::shared_ptr<DrawingTabletManager> DrawingTabletManager::Get() {
    if (!instance) {
        instance = std::unique_ptr<DrawingTabletManager>(
                new DrawingTabletManager());
    }
    return instance;
}

std::shared_ptr<DrawingTabletManager> DrawingTabletManager::instance;

const size_t DrawingTabletManager::GetTabletCount() const {
    return tablets.size();
}

DrawingTabletManager::DrawingTabletManager() {
    X11::Display *display = X11::XOpenDisplay(NULL);

    std::shared_ptr<X11::Display> display_ptr{display, X11::XCloseDisplay};

    int device_count;
    X11::XIDeviceInfo *device_info =
            X11::XIQueryDevice(display, XIAllDevices, &device_count);

    for (int i = 0; i < device_count; i++) {
        std::string name = device_info[i].name;
        if (name.find("stylus") == std::string::npos
            && name.find("eraser") == std::string::npos) {
            continue;
        }

        tablets.emplace_back(
                std::make_unique<DrawingTablet>(device_info[i], display_ptr));
    }
    X11::XIFreeDeviceInfo(device_info);
}

std::shared_ptr<DrawingTablet>
        DrawingTabletManager::GetTablet(size_t minimum_button,
                                        bool supports_pressure,
                                        bool supports_tilt) {
    for (auto &tablet : tablets) {
        if (tablet->GetPossibleButtonCount() >= minimum_button
            && (!supports_pressure || tablet->SupportsPressure())
            && (!supports_tilt || tablet->SupportsTilt() == supports_tilt)) {
            return tablet;
        }
    }

    return nullptr;
}

// #endif
