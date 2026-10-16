#pragma once

#include "LustreConfig.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>

// Each decoration owns its animation: losing focus must not cancel another window's fade.
class CLustreAnimation {
  public:
    using Clock = std::chrono::steady_clock;
    using Time = Clock::time_point;

    explicit CLustreAnimation(bool focused = false, bool urgent = false) : m_focused(focused), m_urgent(urgent), m_orbitPhaseUpdated(Clock::now()) {}

    SLustreFrameStyle style(const SLustreBorderConfig& config, Time now) const {
        const auto& state = m_focused ? config.active : config.inactive;
        const SLustreStyle target = {
            .lightness = state.lightness,
            .saturation = state.saturation,
            .opacity = state.opacity,
            .tintColor = state.tint.color,
            .tintStrength = state.tint.enabled ? state.tint.strength : 0.F,
        };
        SLustreStyle border = target;
        if (m_running) {
            const float elapsed = elapsedMs(now);
            const float transition = progress(elapsed, m_transitionMs);
            border = {
                .lightness = std::lerp(m_from.lightness, target.lightness, transition),
                .saturation = std::lerp(m_from.saturation, target.saturation, transition),
                .opacity = std::lerp(m_from.opacity, target.opacity, transition),
                .tintColor = {},
                .tintStrength = std::lerp(m_from.tintStrength, target.tintStrength, transition),
            };
            for (std::size_t i = 0; i < border.tintColor.size(); ++i)
                border.tintColor[i] = std::lerp(m_from.tintColor[i], target.tintColor[i], transition);

            // The focus pulse reaches its peak halfway through its configured duration.
            if (m_pulseMs > 0.F) {
                const float pulse = 1.F - std::abs(2.F * progress(elapsed, m_pulseMs) - 1.F);
                border.lightness = std::lerp(border.lightness, config.pulse.lightness, pulse);
                border.saturation = std::lerp(border.saturation, config.pulse.saturation, pulse);
                if (config.pulse.tint.enabled) {
                    border.tintStrength = std::lerp(border.tintStrength, config.pulse.tint.strength, pulse);
                    for (std::size_t i = 0; i < border.tintColor.size(); ++i)
                        border.tintColor[i] = std::lerp(border.tintColor[i], config.pulse.tint.color[i], pulse);
                }
            }
        }
        return {.border = border, .orbit = orbitStyle(now)};
    }

    void stateChanged(bool focused, bool urgent, const SLustreBorderConfig& config, Time now) {
        if (focused != m_focused) {
            m_from = style(config, now).border;
            m_started = now;
            m_transitionMs = config.transitionTimeMs;
            m_pulseMs = focused ? config.pulse.timeMs : 0.F;
            m_running = m_transitionMs > 0.F || m_pulseMs > 0.F;
        }
        m_focused = focused;
        m_urgent = urgent;
        retargetOrbit(config, now);
    }

    void refresh(bool focused, bool urgent, const SLustreBorderConfig& config, Time now) {
        advanceOrbitPhase(now);
        m_focused = focused;
        m_urgent = urgent;
        m_running = false;
        m_pulseMs = 0.F;
        retargetOrbit(config, now);
    }

    void reset(bool focused, bool urgent, Time now) {
        advanceOrbitPhase(now);
        m_focused = focused;
        m_urgent = urgent;
        m_running = false;
        m_pulseMs = 0.F;
        m_orbitFrom = m_orbitTo;
        m_orbitTo.amount = 0.F;
        m_orbitTargetEnabled = false;
        m_orbitTransitionRunning = false;
        m_orbitPhaseUpdated = now;
    }

    bool advance(Time now) {
        const bool wasRunning = m_running || m_orbitTransitionRunning || m_orbitTo.amount > 0.F;
        advanceOrbitPhase(now);
        if (m_running && elapsedMs(now) >= std::max(m_transitionMs, m_pulseMs))
            m_running = false;
        if (m_orbitTransitionRunning && orbitElapsedMs(now) >= m_orbitTransitionMs) {
            m_orbitFrom = m_orbitTo;
            m_orbitTransitionRunning = false;
        }
        return wasRunning;
    }

  private:
    static float progress(float elapsed, float duration) {
        return duration > 0.F ? std::clamp(elapsed / duration, 0.F, 1.F) : 1.F;
    }

    float elapsedMs(Time now) const {
        return std::chrono::duration<float, std::milli>(now - m_started).count();
    }

    float orbitElapsedMs(Time now) const {
        return std::chrono::duration<float, std::milli>(now - m_orbitStarted).count();
    }

    SLustreOrbitStyle orbitStyle(Time now) const {
        SLustreOrbitStyle result = m_orbitTo;
        if (m_orbitTransitionRunning) {
            const float transition = progress(orbitElapsedMs(now), m_orbitTransitionMs);
            result.timeMs = std::lerp(m_orbitFrom.timeMs, m_orbitTo.timeMs, transition);
            result.length = std::lerp(m_orbitFrom.length, m_orbitTo.length, transition);
            result.lightness = std::lerp(m_orbitFrom.lightness, m_orbitTo.lightness, transition);
            result.saturation = std::lerp(m_orbitFrom.saturation, m_orbitTo.saturation, transition);
            result.opacity = std::lerp(m_orbitFrom.opacity, m_orbitTo.opacity, transition);
            result.amount = std::lerp(m_orbitFrom.amount, m_orbitTo.amount, transition);
            result.tintStrength = std::lerp(m_orbitFrom.tintStrength, m_orbitTo.tintStrength, transition);
            for (std::size_t i = 0; i < result.tintColor.size(); ++i)
                result.tintColor[i] = std::lerp(m_orbitFrom.tintColor[i], m_orbitTo.tintColor[i], transition);
        }
        result.phase = static_cast<float>(motionPhase(m_orbitPhase));
        return result;
    }

    double motionPhase(double phase) const {
        constexpr double TAU = 6.2831853071795864769;
        const double frequency = TAU * m_motionCycles;
        return phase - m_motionAmount * std::sin(frequency * phase) / frequency;
    }

    void setMotion(const SLustreMotionConfig& motion) {
        if (m_motionAmount == motion.amount && m_motionCycles == motion.cycles)
            return;
        const double position = motionPhase(m_orbitPhase);
        m_motionAmount = motion.amount;
        m_motionCycles = motion.cycles;
        // The warp is strictly monotonic: preserve position when a profile changes.
        double lo = 0.0, hi = 1.0;
        for (int i = 0; i < 32; ++i) {
            const double mid = (lo + hi) * 0.5;
            if (motionPhase(mid) < position)
                lo = mid;
            else
                hi = mid;
        }
        m_orbitPhase = (lo + hi) * 0.5;
    }

    void advanceOrbitPhase(Time now) {
        const double elapsed = std::chrono::duration<double, std::milli>(now - m_orbitPhaseUpdated).count();
        if (elapsed > 0.0 && m_orbitLapTimeMs > 0.F)
            m_orbitPhase = m_orbitPhase + elapsed * m_orbitDirection / static_cast<double>(m_orbitLapTimeMs);
        m_orbitPhase -= std::floor(m_orbitPhase);
        m_orbitPhaseUpdated = now;
    }

    static bool sameOrbit(const SLustreOrbitStyle& lhs, const SLustreOrbitStyle& rhs) {
        return lhs.timeMs == rhs.timeMs && lhs.length == rhs.length && lhs.lightness == rhs.lightness && lhs.saturation == rhs.saturation &&
            lhs.opacity == rhs.opacity && lhs.amount == rhs.amount && lhs.tintColor == rhs.tintColor && lhs.tintStrength == rhs.tintStrength;
    }

    void retargetOrbit(const SLustreBorderConfig& config, Time now) {
        advanceOrbitPhase(now);
        const SLustreOrbitProfile* profile = m_urgent ? &config.orbit.urgent : m_focused ? &config.orbit.active : nullptr;
        const bool targetEnabled = profile && profile->enabled;
        if (targetEnabled) {
            m_orbitDirection = profile->clockwise ? -1.0 : 1.0;
            setMotion(profile->motion);
        }
        SLustreOrbitStyle target = orbitStyle(now);
        if (targetEnabled) {
            target = {
                .timeMs = profile->timeMs,
                .length = profile->length,
                .lightness = profile->lightness,
                .saturation = profile->saturation,
                .opacity = profile->opacity,
                .phase = static_cast<float>(m_orbitPhase),
                .amount = 1.F,
                .tintColor = profile->tint.color,
                .tintStrength = profile->tint.enabled ? profile->tint.strength : 0.F,
            };
        } else {
            target.amount = 0.F;
        }
        if (targetEnabled == m_orbitTargetEnabled && sameOrbit(target, m_orbitTo))
            return;

        m_orbitFrom = orbitStyle(now);
        if (!m_orbitTargetEnabled && targetEnabled && m_orbitFrom.amount == 0.F) {
            m_orbitFrom = target;
            m_orbitFrom.amount = 0.F;
        }
        m_orbitTo = target;
        m_orbitStarted = now;
        m_orbitTransitionMs = config.transitionTimeMs;
        m_orbitTargetEnabled = targetEnabled;
        if (targetEnabled)
            m_orbitLapTimeMs = target.timeMs;
        m_orbitTransitionRunning = m_orbitTransitionMs > 0.F && !sameOrbit(m_orbitFrom, m_orbitTo);
        if (!m_orbitTransitionRunning)
            m_orbitFrom = m_orbitTo;
    }

    bool m_focused = false;
    bool m_urgent = false;
    bool m_running = false;
    SLustreStyle m_from;
    Time m_started;
    float m_transitionMs = 0.F;
    float m_pulseMs = 0.F;

    SLustreOrbitStyle m_orbitFrom = {.opacity = 0.F};
    SLustreOrbitStyle m_orbitTo = {.opacity = 0.F};
    Time m_orbitStarted;
    Time m_orbitPhaseUpdated;
    float m_orbitTransitionMs = 0.F;
    float m_orbitLapTimeMs = 12000.F;
    double m_orbitPhase = 0.0;
    double m_orbitDirection = -1.0;
    double m_motionAmount = 0.35;
    int m_motionCycles = 1;
    bool m_orbitTargetEnabled = false;
    bool m_orbitTransitionRunning = false;
};
