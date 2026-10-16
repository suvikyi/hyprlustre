#pragma once
#include <array>

struct SLustreTintConfig {
    bool enabled = false;
    std::array<float, 3> color = {1.F, 128.F / 255.F, 102.F / 255.F};
    float strength = 0.18F;
};

struct SLustreStateConfig {
    float lightness  = 0.F;
    float saturation = 1.F;
    float opacity    = 0.F;
    SLustreTintConfig tint;
};

struct SLustreStyle {
    float lightness  = 0.F;
    float saturation = 1.F;
    float opacity    = 0.F;
    std::array<float, 3> tintColor = {1.F, 128.F / 255.F, 102.F / 255.F};
    float tintStrength = 0.F;
};

struct SLustreMotionConfig {
    float amount = 0.35F;
    int cycles = 1;
};

struct SLustreOrbitProfile {
    bool enabled = false;
    bool clockwise = true;
    float timeMs = 12000.F;
    float length = 0.12F;
    float lightness = 0.25F;
    float saturation = 4.F;
    float opacity = 1.F;
    SLustreTintConfig tint = {.strength = 0.25F};
    SLustreMotionConfig motion;
};

struct SLustreOrbitConfig {
    SLustreOrbitProfile active;
    SLustreOrbitProfile urgent = {
        .enabled = false,
        .timeMs = 5000.F,
        .length = 0.18F,
        .lightness = 0.375F,
        .saturation = 5.F,
        .opacity = 1.F,
        .tint = {.strength = 0.25F},
        .motion = {.amount = 0.15F},
    };
};

struct SLustreOrbitStyle {
    float timeMs = 12000.F;
    float length = 0.F;
    float lightness = 0.F;
    float saturation = 1.F;
    float opacity = 0.F;
    float phase = 0.F;
    float amount = 0.F;
    std::array<float, 3> tintColor = {1.F, 128.F / 255.F, 102.F / 255.F};
    float tintStrength = 0.F;
};

struct SLustrePulseConfig {
    float timeMs = 333.F;
    float lightness = 0.375F;
    float saturation = 5.F;
    SLustreTintConfig tint;
};

struct SLustreBorderConfig {
    bool enabled = true;
    int size = 1;
    SLustreStateConfig active = {.lightness = 0.175F, .saturation = 3.5F, .opacity = 1.F};
    SLustreStateConfig inactive = {.lightness = 0.1F, .saturation = 3.5F, .opacity = 1.F};
    float transitionTimeMs = 333.F;
    SLustrePulseConfig pulse;
    SLustreOrbitConfig orbit;
};

struct SLustreFrameStyle {
    SLustreStyle border;
    SLustreOrbitStyle orbit;
};

struct SLustreConfig {
    bool               enabled = true;
    SLustreBorderConfig inset;
    SLustreBorderConfig outset = {.enabled = false};
    bool outsetBlur = false;
};

bool         registerLustreConfig(void* handle);
SLustreConfig lustreConfig();
