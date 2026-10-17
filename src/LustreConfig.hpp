#pragma once
#include <array>

// The shipped look lives in the LUSTRE_*_DEFAULTS seeds below. These members
// are inert fallbacks, used only where a seed omits a field. The tint fallback
// is Hyprland's own default glow blue (`decoration:glow:color`, `33ccffee`).
struct SLustreTintConfig {
    std::array<float, 3> color = {51.F / 255.F, 204.F / 255.F, 1.F};
    float strength = 0.F;
};

struct SLustreStateConfig {
    float lightness  = 0.F;
    float saturation = 1.F;
    float chroma     = 0.F;
    float hue        = 0.F;
    float opacity    = 0.F;
    SLustreTintConfig tint;
};

struct SLustreStyle {
    float lightness  = 0.F;
    float saturation = 1.F;
    float chroma     = 0.F;
    float hue        = 0.F;
    float opacity    = 0.F;
    std::array<float, 3> tintColor = {51.F / 255.F, 204.F / 255.F, 1.F};
    float tintStrength = 0.F;
};

struct SLustreMotionConfig {
    float amount = 0.F;
    int cycles = 1;
};

struct SLustreOrbitProfile {
    bool enabled = false;
    bool clockwise = true;
    // Structural default: the animation divides by this, so it must stay positive.
    float timeMs = 12000.F;
    float length = 0.F;
    float lightness = 0.F;
    float saturation = 1.F;
    float chroma = 0.F;
    float hue = 0.F;
    float opacity = 0.F;
    SLustreTintConfig tint;
    SLustreMotionConfig motion;
    // Flare only: presence ramps in over fadeInMs and out over fadeOutMs, both
    // measured from the ends of the single ignition lap.
    float fadeInMs = 0.F;
    float fadeOutMs = 0.F;
};

struct SLustreOrbitConfig {
    SLustreOrbitProfile active;
    SLustreOrbitProfile urgent;
    SLustreOrbitProfile inactive;
    // One-shot ignition: on focus gain the orbit fades in over `fade.inMs`, laps
    // exactly once at this profile's speed, and fades out over `fade.outMs`
    // (anchored to the end of that lap), then hands back to the focused state's
    // profile — or stays gone if that profile is disabled.
    SLustreOrbitProfile flare;
};

struct SLustreOrbitStyle {
    float timeMs = 12000.F;
    float length = 0.F;
    float lightness = 0.F;
    float saturation = 1.F;
    float chroma = 0.F;
    float hue = 0.F;
    float opacity = 0.F;
    float phase = 0.F;
    float amount = 0.F;
    std::array<float, 3> tintColor = {51.F / 255.F, 204.F / 255.F, 1.F};
    float tintStrength = 0.F;
};

struct SLustrePulseConfig {
    bool enabled = false;
    float timeMs = 333.F;
    float lightness = 0.F;
    float saturation = 1.F;
    float chroma = 0.F;
    float hue = 0.F;
    float opacity = 0.F;
    SLustreTintConfig tint;
};

struct SLustreBorderConfig {
    bool enabled = false;
    int size = 1;
    SLustreStateConfig active;
    SLustreStateConfig inactive;
    float transitionTimeMs = 333.F;
    SLustrePulseConfig pulse;
    SLustreOrbitConfig orbit;
};

// Shipped defaults, one set per placement. These are the source of truth for
// every `plugin.lustre.border.*` default: the registration reads them directly
// and a partially missing config falls back to them. `inset` is a translucent
// 1px glaze; `outset` is a 2px blurred one that tints toward the accent.
inline const SLustreBorderConfig LUSTRE_INSET_DEFAULTS = {
    .enabled = false,
    .size = 1,
    .active = {
        .lightness = 0.175F,
        .saturation = 1.F,
        .chroma = 0.05F,
        .hue = 0.F,
        .opacity = 1.F,
        .tint = {.strength = 0.F},
    },
    .inactive = {
        .lightness = 0.12F,
        .saturation = 1.F,
        .chroma = 0.05F,
        .hue = 0.F,
        .opacity = 1.F,
        .tint = {.strength = 0.F},
    },
    .transitionTimeMs = 333.F,
    .pulse = {
        .enabled = true,
        .timeMs = 333.F,
        .lightness = 0.45F,
        .saturation = 1.F,
        .chroma = 0.1F,
        .hue = 0.F,
        .opacity = 1.F,
        .tint = {.strength = 0.F},
    },
    .orbit = {
        .active = {
            .enabled = true,
            .clockwise = true,
            .timeMs = 30000.F,
            .length = 0.154F,
            .lightness = 0.5F,
            .saturation = 1.F,
            .chroma = 0.1F,
            .hue = 0.F,
            .opacity = 0.618F,
            .tint = {.strength = 0.F},
            .motion = {.amount = 0.75F, .cycles = 2},
        },
        .urgent = {
            .enabled = true,
            .clockwise = true,
            .timeMs = 3000.F,
            .length = 0.154F,
            .lightness = 0.5F,
            .saturation = 1.F,
            .chroma = 0.05F,
            .hue = 0.F,
            .opacity = 0.5F,
            .tint = {.strength = 0.F},
            .motion = {.amount = 0.75F, .cycles = 2},
        },
        .inactive = {
            .enabled = false,
            .clockwise = true,
            .timeMs = 30000.F,
            .length = 0.154F,
            .lightness = 0.5F,
            .saturation = 1.F,
            .chroma = 0.1F,
            .hue = 0.F,
            .opacity = 0.618F,
            .tint = {.strength = 0.F},
            .motion = {.amount = 0.75F, .cycles = 2},
        },
        .flare = {
            .enabled = true,
            .clockwise = true,
            .timeMs = 667.F,
            .length = 0.309F,
            .lightness = 0.5F,
            .saturation = 1.F,
            .chroma = 0.1F,
            .hue = 0.F,
            .opacity = 0.75F,
            .tint = {.strength = 0.F},
            .motion = {.amount = 0.75F, .cycles = 2},
            .fadeInMs = 0.F,
            .fadeOutMs = 0.F,
        },
    },
};

inline const SLustreBorderConfig LUSTRE_OUTSET_DEFAULTS = {
    .enabled = true,
    .size = 2,
    .active = {
        .lightness = 0.4F,
        .saturation = 1.F,
        .chroma = 0.05F,
        .hue = 0.F,
        .opacity = 0.85F,
        .tint = {.strength = 1.F},
    },
    .inactive = {
        .lightness = 0.225F,
        .saturation = 1.F,
        .chroma = 0.05F,
        .hue = 0.F,
        .opacity = 0.75F,
        .tint = {.strength = 1.F},
    },
    .transitionTimeMs = 333.F,
    .pulse = {
        .enabled = true,
        .timeMs = 367.F,
        .lightness = 0.375F,
        .saturation = 1.F,
        .chroma = 0.05F,
        .hue = 0.F,
        .opacity = 1.F,
        .tint = {.strength = 1.F},
    },
    .orbit = {
        .active = {
            .enabled = true,
            .clockwise = true,
            .timeMs = 30000.F,
            .length = 0.154F,
            .lightness = 0.425F,
            .saturation = 1.F,
            .chroma = 0.05F,
            .hue = 0.F,
            .opacity = 1.F,
            .tint = {.strength = 1.F},
            .motion = {.amount = 0.5F, .cycles = 2},
        },
        .urgent = {
            .enabled = true,
            .clockwise = true,
            .timeMs = 3000.F,
            .length = 0.154F,
            .lightness = 0.425F,
            .saturation = 1.F,
            .chroma = 0.05F,
            .hue = 0.F,
            .opacity = 1.F,
            .tint = {.strength = 1.F},
            .motion = {.amount = 0.5F, .cycles = 2},
        },
        .inactive = {
            .enabled = false,
            .clockwise = true,
            .timeMs = 30000.F,
            .length = 0.154F,
            .lightness = 0.425F,
            .saturation = 1.F,
            .chroma = 0.05F,
            .hue = 0.F,
            .opacity = 1.F,
            .tint = {.strength = 1.F},
            .motion = {.amount = 0.5F, .cycles = 2},
        },
        .flare = {
            .enabled = true,
            .clockwise = true,
            .timeMs = 667.F,
            .length = 0.309F,
            .lightness = 0.475F,
            .saturation = 1.F,
            .chroma = 0.1F,
            .hue = 0.F,
            .opacity = 1.F,
            .tint = {.strength = 1.F},
            .motion = {.amount = 0.618F, .cycles = 2},
            .fadeInMs = 0.F,
            .fadeOutMs = 0.F,
        },
    },
};

struct SLustreFrameStyle {
    SLustreStyle border;
    SLustreOrbitStyle orbit;
};

struct SLustreConfig {
    bool                enabled = true;
    SLustreBorderConfig inset = LUSTRE_INSET_DEFAULTS;
    SLustreBorderConfig outset = LUSTRE_OUTSET_DEFAULTS;
    bool                outsetBlur = true;
};

bool         registerLustreConfig(void* handle);
SLustreConfig lustreConfig();
