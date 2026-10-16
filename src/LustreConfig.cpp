#include "LustreConfig.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <deque>
#include <string>
#include <utility>

#include <hyprland/src/plugins/PluginAPI.hpp>
#include <hyprland/src/config/ConfigValue.hpp>
#include <hyprland/src/config/values/types/BoolValue.hpp>
#include <hyprland/src/config/values/types/ColorValue.hpp>
#include <hyprland/src/config/values/types/FloatValue.hpp>
#include <hyprland/src/config/values/types/IntValue.hpp>

namespace {
    using Config::Values::CBoolValue;
    using Config::Values::CColorValue;
    using Config::Values::CFloatValue;
    using Config::Values::CIntValue;

    struct STintConfigValues {
        SP<CBoolValue> enabled;
        SP<CColorValue> color;
        SP<CFloatValue> strength;
    };

    struct STintConfigNames {
        std::string enabled;
        std::string color;
        std::string strength;
    };
    struct SOrbitConfigNames {
        std::string enabled;
        std::string clockwise;
        std::string time;
        std::string length;
        std::string lightness;
        std::string saturation;
        std::string opacity;
        STintConfigNames tint;
        std::string motionAmount;
        std::string motionCycles;
    };

    struct SOrbitConfigValues {
        SP<CBoolValue> enabled;
        SP<CBoolValue> clockwise;
        SP<CFloatValue> time;
        SP<CFloatValue> length;
        SP<CFloatValue> lightness;
        SP<CFloatValue> saturation;
        SP<CFloatValue> opacity;
        STintConfigValues tint;
        SP<CFloatValue> motionAmount;
        SP<CIntValue> motionCycles;
    };

    // Hyprland stores borrowed name pointers; deque preserves them until plugin unload.
    std::deque<std::string> configNames;
    const char* configName(std::string name) {
        configNames.push_back(std::move(name));
        return configNames.back().c_str();
    }
    SP<CBoolValue> enabled;
    SP<CBoolValue> outsetBlur;
    struct SBorderValues {
        SP<CBoolValue>  enabled;
        SP<CIntValue>   size;
        SP<CFloatValue> activeLightness;
        SP<CFloatValue> activeSaturation;
        SP<CFloatValue> activeOpacity;
        SP<CFloatValue> inactiveLightness;
        SP<CFloatValue> inactiveSaturation;
        SP<CFloatValue> inactiveOpacity;
        SP<CFloatValue> transitionTime;
        SP<CFloatValue> pulseTime;
        SP<CFloatValue> pulseLightness;
        SP<CFloatValue> pulseSaturation;
        STintConfigValues activeTint;
        STintConfigValues inactiveTint;
        STintConfigValues pulseTint;
        SOrbitConfigValues activeOrbit;
        SOrbitConfigValues urgentOrbit;

    };
    SBorderValues insetValues, outsetValues;

    SP<CIntValue> intValue(HANDLE handle, const std::string& name, const char* description, int value, int minimum, int maximum) {
        auto option = Config::Values::SIntValueOptions{.min = minimum, .max = maximum};
        auto result = makeShared<CIntValue>(configName(name), description, value, std::move(option));
        if (!HyprlandAPI::addConfigValueV2(handle, result))
            return nullptr;
        return result;
    }

    SP<CColorValue> colorValue(HANDLE handle, const std::string& name, const char* description, Config::INTEGER value) {
        auto result = makeShared<CColorValue>(configName(name), description, value);
        if (!HyprlandAPI::addConfigValueV2(handle, result))
            return nullptr;
        return result;
    }

    SP<CFloatValue> floatValue(HANDLE handle, const std::string& name, const char* description, float value, float minimum, float maximum) {
        auto option = Config::Values::SFloatValueOptions{.min = minimum, .max = maximum};
        auto result = makeShared<CFloatValue>(configName(name), description, value, std::move(option));
        if (!HyprlandAPI::addConfigValueV2(handle, result))
            return nullptr;
        return result;
    }

    float boundedFloat(Config::FLOAT value, float fallback, float minimum, float maximum) {
        if (!std::isfinite(value))
            return fallback;

        return static_cast<float>(std::clamp(static_cast<double>(value), static_cast<double>(minimum), static_cast<double>(maximum)));
    }

    bool registerTintValues(HANDLE handle, const STintConfigNames& names, STintConfigValues& tint, float defaultStrength = 0.18F) {
        tint.enabled = makeShared<CBoolValue>(configName(names.enabled), "Enable tinting for this state", false);
        if (!HyprlandAPI::addConfigValueV2(handle, tint.enabled))
            return false;
        tint.color = colorValue(handle, names.color, "Tint color for this state", 0xFFFF8066);
        if (!tint.color)
            return false;
        tint.strength = floatValue(handle, names.strength, "Tint blend strength for this state", defaultStrength, 0.F, 1.F);
        return static_cast<bool>(tint.strength);
    }
    bool registerOrbitValues(HANDLE handle, const SOrbitConfigNames& names, SOrbitConfigValues& values, float timeMs, float length, float lightness,
                             float saturation, float motionAmount) {
        values.enabled = makeShared<CBoolValue>(configName(names.enabled), "Enable the traveling orbit border highlight for this state", false);
        if (!HyprlandAPI::addConfigValueV2(handle, values.enabled))
            return false;
        values.clockwise = makeShared<CBoolValue>(configName(names.clockwise), "Travel clockwise instead of counter-clockwise", true);
        if (!HyprlandAPI::addConfigValueV2(handle, values.clockwise))
            return false;
        values.time = floatValue(handle, names.time, "Orbit lap duration in milliseconds", timeMs, 1.F, 120000.F);
        if (!values.time)
            return false;
        values.length = floatValue(handle, names.length, "Orbit highlight length as a fraction of the perimeter", length, 0.F, 1.F);
        if (!values.length)
            return false;
        values.lightness = floatValue(handle, names.lightness, "Orbit absolute OKLab lightness adjustment", lightness, -1.F, 1.F);
        if (!values.lightness)
            return false;
        values.saturation = floatValue(handle, names.saturation, "Orbit absolute OKLab chroma multiplier", saturation, 0.F, 255.F);
        if (!values.saturation)
            return false;
        values.motionAmount = floatValue(handle, names.motionAmount, "Orbit speed variation amplitude", motionAmount, 0.F, 0.95F);
        values.motionCycles = intValue(handle, names.motionCycles, "Speed variation cycles per lap", 1, 1, 4);
        if (!values.motionAmount || !values.motionCycles)
            return false;
        values.opacity = floatValue(handle, names.opacity, "Orbit transformed-pixel blend opacity", 1.F, 0.F, 1.F);
        return values.opacity && registerTintValues(handle, names.tint, values.tint, 0.25F);
    }

    bool tintValuesReady(const STintConfigValues& tint) {
        return tint.enabled && tint.color && tint.strength;
    }

    SLustreTintConfig tintConfig(const STintConfigValues& tint, float fallbackStrength = 0.18F) {
        const auto argb = static_cast<std::uint32_t>(tint.color->value());
        return {
            .enabled = tint.enabled->value(),
            .color = {
                static_cast<float>((argb >> 16) & 0xFFU) / 255.F,
                static_cast<float>((argb >> 8) & 0xFFU) / 255.F,
                static_cast<float>(argb & 0xFFU) / 255.F,
            },
            .strength = boundedFloat(tint.strength->value(), fallbackStrength, 0.F, 1.F),
        };
    }

    bool orbitValuesReady(const SOrbitConfigValues& values) {
        return values.enabled && values.clockwise && values.time && values.length && values.lightness && values.saturation && values.opacity &&
            values.motionAmount && values.motionCycles && tintValuesReady(values.tint);
    }

    SLustreOrbitProfile orbitProfileConfig(const SOrbitConfigValues& values, const SLustreOrbitProfile& fallback) {
        return {
            .enabled = values.enabled->value(),
            .clockwise = values.clockwise->value(),
            .timeMs = boundedFloat(values.time->value(), fallback.timeMs, 1.F, 120000.F),
            .length = boundedFloat(values.length->value(), fallback.length, 0.F, 1.F),
            .lightness = boundedFloat(values.lightness->value(), fallback.lightness, -1.F, 1.F),
            .saturation = boundedFloat(values.saturation->value(), fallback.saturation, 0.F, 255.F),
            .opacity = boundedFloat(values.opacity->value(), fallback.opacity, 0.F, 1.F),
            .tint = tintConfig(values.tint, 0.25F),
            .motion = {
                .amount = boundedFloat(values.motionAmount->value(), fallback.motion.amount, 0.F, 0.95F),
                .cycles = static_cast<int>(std::clamp<Config::INTEGER>(values.motionCycles->value(), 1, 4)),
            },
        };
    }

    bool registerBorder(HANDLE handle, SBorderValues& values, const std::string& prefix, bool defaultEnabled) {
        const SOrbitConfigNames ACTIVE_ORBIT_NAMES = {
            .enabled = prefix + ".orbit.active.enabled",
            .clockwise = prefix + ".orbit.active.clockwise",
            .time = prefix + ".orbit.active.time",
            .length = prefix + ".orbit.active.length",
            .lightness = prefix + ".orbit.active.lightness",
            .saturation = prefix + ".orbit.active.saturation",
            .opacity = prefix + ".orbit.active.opacity",
            .tint = {
                .enabled = prefix + ".orbit.active.tint.enabled",
                .color = prefix + ".orbit.active.tint.color",
                .strength = prefix + ".orbit.active.tint.strength",
            },
            .motionAmount = prefix + ".orbit.active.motion.amount",
            .motionCycles = prefix + ".orbit.active.motion.cycles",
        };
        const SOrbitConfigNames URGENT_ORBIT_NAMES = {
            .enabled = prefix + ".orbit.urgent.enabled",
            .clockwise = prefix + ".orbit.urgent.clockwise",
            .time = prefix + ".orbit.urgent.time",
            .length = prefix + ".orbit.urgent.length",
            .lightness = prefix + ".orbit.urgent.lightness",
            .saturation = prefix + ".orbit.urgent.saturation",
            .opacity = prefix + ".orbit.urgent.opacity",
            .tint = {
                .enabled = prefix + ".orbit.urgent.tint.enabled",
                .color = prefix + ".orbit.urgent.tint.color",
                .strength = prefix + ".orbit.urgent.tint.strength",
            },
            .motionAmount = prefix + ".orbit.urgent.motion.amount",
            .motionCycles = prefix + ".orbit.urgent.motion.cycles",
        };

        const STintConfigNames ACTIVE_TINT_NAMES = {
            .enabled = prefix + ".active.tint.enabled",
            .color = prefix + ".active.tint.color",
            .strength = prefix + ".active.tint.strength",
        };
        const STintConfigNames INACTIVE_TINT_NAMES = {
            .enabled = prefix + ".inactive.tint.enabled",
            .color = prefix + ".inactive.tint.color",
            .strength = prefix + ".inactive.tint.strength",
        };
        const STintConfigNames PULSE_TINT_NAMES = {
            .enabled = prefix + ".pulse.tint.enabled",
            .color = prefix + ".pulse.tint.color",
            .strength = prefix + ".pulse.tint.strength",
        };

        values.enabled = makeShared<CBoolValue>(configName(prefix + ".enabled"), "Enable this sampled window border", defaultEnabled);
        if (!HyprlandAPI::addConfigValueV2(handle, values.enabled))
            return false;

        values.size = intValue(handle, prefix + ".size", "Border thickness in logical pixels", 1, 1, 16);
        if (!values.size)
            return false;

        values.activeLightness = floatValue(handle, prefix + ".active.lightness", "Active additive OKLab lightness adjustment", 0.175F, -1.F, 1.F);
        if (!values.activeLightness)
            return false;
        values.activeSaturation = floatValue(handle, prefix + ".active.saturation", "Active OKLab chroma multiplier (1 is unchanged)", 3.5F, 0.F, 255.F);
        if (!values.activeSaturation)
            return false;
        values.activeOpacity = floatValue(handle, prefix + ".active.opacity", "Active transformed-pixel blend opacity", 1.F, 0.F, 1.F);
        if (!values.activeOpacity || !registerTintValues(handle, ACTIVE_TINT_NAMES, values.activeTint))
            return false;

        values.inactiveLightness = floatValue(handle, prefix + ".inactive.lightness", "Inactive additive OKLab lightness adjustment", 0.1F, -1.F, 1.F);
        if (!values.inactiveLightness)
            return false;
        values.inactiveSaturation = floatValue(handle, prefix + ".inactive.saturation", "Inactive OKLab chroma multiplier (1 is unchanged)", 3.5F, 0.F, 255.F);
        if (!values.inactiveSaturation)
            return false;
        values.inactiveOpacity = floatValue(handle, prefix + ".inactive.opacity", "Inactive transformed-pixel blend opacity", 1.F, 0.F, 1.F);
        if (!values.inactiveOpacity || !registerTintValues(handle, INACTIVE_TINT_NAMES, values.inactiveTint))
            return false;

        values.transitionTime = floatValue(handle, prefix + ".transition.time", "Focus-state transition duration in milliseconds", 333.F, 0.F, 2000.F);
        if (!values.transitionTime)
            return false;

        values.pulseTime = floatValue(handle, prefix + ".pulse.time", "Active border pulse duration in milliseconds", 333.F, 0.F, 2000.F);
        if (!values.pulseTime)
            return false;
        values.pulseLightness = floatValue(handle, prefix + ".pulse.lightness", "Peak pulse additive OKLab lightness adjustment", 0.375F, -1.F, 1.F);
        if (!values.pulseLightness)
            return false;
        values.pulseSaturation = floatValue(handle, prefix + ".pulse.saturation", "Peak pulse OKLab chroma multiplier (1 is unchanged)", 5.F, 0.F, 255.F);
        if (!values.pulseSaturation || !registerTintValues(handle, PULSE_TINT_NAMES, values.pulseTint))
            return false;
        return registerOrbitValues(handle, ACTIVE_ORBIT_NAMES, values.activeOrbit, 12000.F, 0.12F, 0.25F, 4.F, 0.35F) &&
            registerOrbitValues(handle, URGENT_ORBIT_NAMES, values.urgentOrbit, 5000.F, 0.18F, 0.375F, 5.F, 0.15F);
    }

    SLustreBorderConfig readBorder(const SBorderValues& values, bool defaultEnabled) {
        SLustreBorderConfig config;
        config.enabled = defaultEnabled;
        if (!values.enabled || !values.size || !values.activeLightness || !values.activeSaturation || !values.activeOpacity || !values.inactiveLightness || !values.inactiveSaturation ||
            !values.inactiveOpacity || !values.transitionTime || !values.pulseTime || !values.pulseLightness || !values.pulseSaturation || !tintValuesReady(values.activeTint) || !tintValuesReady(values.inactiveTint) ||
            !tintValuesReady(values.pulseTint) || !orbitValuesReady(values.activeOrbit) || !orbitValuesReady(values.urgentOrbit))
            return config;

        config.enabled = values.enabled->value();
        config.size = static_cast<int>(std::clamp<Config::INTEGER>(values.size->value(), 1, 16));
        config.active = {
            .lightness = boundedFloat(values.activeLightness->value(), 0.175F, -1.F, 1.F),
            .saturation = boundedFloat(values.activeSaturation->value(), 3.5F, 0.F, 255.F),
            .opacity = boundedFloat(values.activeOpacity->value(), 1.F, 0.F, 1.F),
            .tint = tintConfig(values.activeTint),
        };
        config.inactive = {
            .lightness = boundedFloat(values.inactiveLightness->value(), 0.1F, -1.F, 1.F),
            .saturation = boundedFloat(values.inactiveSaturation->value(), 3.5F, 0.F, 255.F),
            .opacity = boundedFloat(values.inactiveOpacity->value(), 1.F, 0.F, 1.F),
            .tint = tintConfig(values.inactiveTint),
        };
        config.transitionTimeMs = boundedFloat(values.transitionTime->value(), 333.F, 0.F, 2000.F);
        config.pulse = {
            .timeMs = boundedFloat(values.pulseTime->value(), 333.F, 0.F, 2000.F),
            .lightness = boundedFloat(values.pulseLightness->value(), 0.375F, -1.F, 1.F),
            .saturation = boundedFloat(values.pulseSaturation->value(), 5.F, 0.F, 255.F),
            .tint = tintConfig(values.pulseTint),
        };
        config.orbit.active = orbitProfileConfig(values.activeOrbit, config.orbit.active);
        config.orbit.urgent = orbitProfileConfig(values.urgentOrbit, config.orbit.urgent);
        return config;
    }
}

bool registerLustreConfig(HANDLE handle) {
    enabled = makeShared<CBoolValue>("plugin:lustre:enabled", "Enable Hyprlustre visual effects", true);
    outsetBlur = makeShared<CBoolValue>("plugin:lustre:border.outset.blur", "Sample Hyprland's cached background blur (requires blur and new_optimizations)", false);
    return HyprlandAPI::addConfigValueV2(handle, enabled) && HyprlandAPI::addConfigValueV2(handle, outsetBlur) &&
        registerBorder(handle, insetValues, "plugin:lustre:border.inset", true) &&
        registerBorder(handle, outsetValues, "plugin:lustre:border.outset", false);
}

SLustreConfig lustreConfig() {
    SLustreConfig config;
    if (enabled) config.enabled = enabled->value();
    config.inset = readBorder(insetValues, true);
    config.outset = readBorder(outsetValues, false);
    static const auto NATIVE_BLUR = CConfigValue<Config::INTEGER>("decoration:blur:enabled");
    static const auto NATIVE_CACHE = CConfigValue<Config::INTEGER>("decoration:blur:new_optimizations");
    config.outsetBlur = config.enabled && config.outset.enabled && outsetBlur && outsetBlur->value() && *NATIVE_BLUR && *NATIVE_CACHE;
    return config;
}
