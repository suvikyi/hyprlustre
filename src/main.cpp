#define WLR_USE_UNSTABLE

#include "HyprionConfig.hpp"
#include "HyprionDecoration.hpp"

#include <chrono>
#include <cmath>
#include <optional>
#include <string>
#include <stdexcept>
#include <vector>
#include <unordered_map>

#include <hyprland/src/Compositor.hpp>
#include <hyprland/src/desktop/state/FocusState.hpp>
#include <hyprland/src/desktop/state/WindowState.hpp>
#include <hyprland/src/event/EventBus.hpp>
#include <hyprland/src/managers/eventLoop/EventLoopManager.hpp>
#include <hyprland/src/managers/eventLoop/EventLoopTimer.hpp>
#include <hyprland/src/managers/fullscreen/FullscreenController.hpp>
#include <hyprland/src/render/Renderer.hpp>
#include <hyprland/src/render/decorations/CHyprDropShadowDecoration.hpp>
#include <hyprland/src/plugins/HookSystem.hpp>
#include <hyprland/src/plugins/PluginAPI.hpp>

namespace {
    using namespace Hyprutils::Memory;

    constexpr auto ANIMATION_TICK = std::chrono::milliseconds(16);

    HANDLE                            pluginHandle = nullptr;
    bool                              pluginActive = false;
    std::vector<CHyprSignalListener>   listeners;
    SP<CEventLoopTimer>               animationTimer;
    PHLWINDOWREF                      lastFocusedWindow;
    CFunctionHook* shadowHook = nullptr;
    CFunctionHook* shadowDrawHook = nullptr;
    std::unordered_map<CHyprDropShadowDecoration*, PHLWINDOWREF> shadowWindows;

    SShadowRenderData shadowRenderData(CHyprDropShadowDecoration* shadow, PHLMONITOR monitor, const float& alpha) {
        using Original = SShadowRenderData (*)(CHyprDropShadowDecoration*, PHLMONITOR, const float&);
        auto data = reinterpret_cast<Original>(shadowHook->m_original)(shadow, monitor, alpha);
        if (!data.valid || data.rounding <= 0.F)
            return data;
        const auto config = hyprionConfig();
        if (!config.enabled || !config.outset.enabled)
            return data;
        const auto owner = shadowWindows.find(shadow);
        if (owner == shadowWindows.end())
            return data;
        const auto window = owner->second.lock();
        if (!window || !window->m_isMapped)
            return data;
        // The native shadow box already includes our reserved outset, but its
        // radius only includes the native border. Expand both by the same amount.
        const float correction = (std::sqrt(2.F) - 1.F) * std::max(2.F - data.roundingPower, 0.F);
        data.rounding += config.outset.size * (1.F - correction);
        return data;
    }

    CHyprionDecoration* hyprionDecoration(const PHLWINDOW& window) {
        if (!window || !window->m_isMapped)
            return nullptr;
        for (const auto& decoration : window->m_windowDecorations) {
            if (auto* hyprion = dynamic_cast<CHyprionDecoration*>(decoration.get()))
                return hyprion;
        }
        return nullptr;
    }

    void drawShadow(CHyprDropShadowDecoration* shadow, PHLMONITOR monitor, const float& alpha) {
        const auto owner = shadowWindows.find(shadow);
        if (owner != shadowWindows.end()) {
            if (auto* decoration = hyprionDecoration(owner->second.lock()))
                decoration->drawOutset(monitor, alpha);
        }
        using Original = void (*)(CHyprDropShadowDecoration*, PHLMONITOR, const float&);
        reinterpret_cast<Original>(shadowDrawHook->m_original)(shadow, monitor, alpha);
    }

    void onAnimationTimer(SP<CEventLoopTimer> timer, void*) {
        if (!pluginActive)
            return;

        const auto config = hyprionConfig();
        const auto now = CHyprionAnimation::Clock::now();
        const auto focused = Desktop::focusState()->window();
        bool running = false;
        for (const auto& window : Desktop::windowState()->windows()) {
            auto* decoration = hyprionDecoration(window);
            if (!decoration)
                continue;
            if (!config.enabled || (!config.inset.enabled && !config.outset.enabled))
                decoration->resetAnimation(window == focused, window->m_isUrgent, now);
            else
                running = decoration->animate(config, now) || running;
        }
        timer->updateTimeout(running ? std::optional{ANIMATION_TICK} : std::nullopt);
    }

    void onFocusChanged(const PHLWINDOW& focusedWindow, const Desktop::eFocusReason&) {
        const auto previous = lastFocusedWindow.lock();
        if (previous == focusedWindow)
            return;
        lastFocusedWindow = focusedWindow;

        const auto config = hyprionConfig();
        const auto now = CHyprionAnimation::Clock::now();
        const bool effectsEnabled = config.enabled && (config.inset.enabled || config.outset.enabled);
        bool running = false;
        if (auto* decoration = hyprionDecoration(previous)) {
            if (effectsEnabled) {
                decoration->stateChanged(false, previous->m_isUrgent, config, now);
                running = decoration->animate(config, now) || running;
            } else {
                decoration->resetAnimation(false, previous->m_isUrgent, now);
            }
        }
        if (auto* decoration = hyprionDecoration(focusedWindow)) {
            if (effectsEnabled) {
                decoration->stateChanged(true, focusedWindow->m_isUrgent, config, now);
                running = decoration->animate(config, now) || running;
            } else {
                decoration->resetAnimation(true, focusedWindow->m_isUrgent, now);
            }
        }
        if (running && animationTimer)
            animationTimer->updateTimeout(ANIMATION_TICK);
    }

    void onUrgencyChanged(const PHLWINDOW& urgentWindow) {
        auto* decoration = hyprionDecoration(urgentWindow);
        if (!decoration)
            return;

        const auto config = hyprionConfig();
        const auto now = CHyprionAnimation::Clock::now();
        const bool focused = urgentWindow == Desktop::focusState()->window();
        if (!config.enabled || (!config.inset.enabled && !config.outset.enabled)) {
            decoration->resetAnimation(focused, urgentWindow->m_isUrgent, now);
            return;
        }

        decoration->stateChanged(focused, urgentWindow->m_isUrgent, config, now);
        if (animationTimer && decoration->animate(config, now))
            animationTimer->updateTimeout(ANIMATION_TICK);
    }

    void attachDecoration(const PHLWINDOW& window, bool animateFocus) {
        if (!window || !window->m_isMapped || hyprionDecoration(window))
            return;

        for (const auto& deco : window->m_windowDecorations) {
            if (auto* shadow = dynamic_cast<CHyprDropShadowDecoration*>(deco.get()))
                shadowWindows.insert_or_assign(shadow, window);
        }
        HyprlandAPI::addWindowDecoration(pluginHandle, window, makeUnique<CHyprionDecoration>(window));
        auto* decoration = hyprionDecoration(window);
        if (!decoration)
            return;

        const auto config = hyprionConfig();
        const auto focused = Desktop::focusState()->window();
        const bool isFocused = window == focused;
        const auto now = CHyprionAnimation::Clock::now();
        if (!config.enabled || (!config.inset.enabled && !config.outset.enabled)) {
            decoration->resetAnimation(isFocused, window->m_isUrgent, now);
            return;
        }
        if (animateFocus && isFocused) {
            decoration->resetAnimation(false, false, now);
            decoration->stateChanged(true, window->m_isUrgent, config, now);
        } else {
            decoration->stateChanged(isFocused, window->m_isUrgent, config, now);
        }
        if (animationTimer && decoration->animate(config, now))
            animationTimer->updateTimeout(ANIMATION_TICK);
    }

    void onConfigReloaded() {
        if (animationTimer)
            animationTimer->updateTimeout(std::nullopt);
        const auto config = hyprionConfig();
        const auto now = CHyprionAnimation::Clock::now();
        const auto focused = Desktop::focusState()->window();
        lastFocusedWindow = focused;
        bool running = false;
        for (const auto& window : Desktop::windowState()->windows()) {
            if (auto* decoration = hyprionDecoration(window)) {
                decoration->updateWindow(window);
                if (!config.enabled || (!config.inset.enabled && !config.outset.enabled))
                    decoration->resetAnimation(window == focused, window->m_isUrgent, now);
                else {
                    decoration->refreshAnimation(window == focused, window->m_isUrgent, config, now);
                    running = decoration->animate(config, now) || running;
                }
            }
        }
        if (running && animationTimer)
            animationTimer->updateTimeout(ANIMATION_TICK);
    }

}

APICALL EXPORT std::string PLUGIN_API_VERSION() {
    return HYPRLAND_API_VERSION;
}

APICALL EXPORT PLUGIN_DESCRIPTION_INFO PLUGIN_INIT(HANDLE handle) {
    pluginHandle = handle;
    pluginActive = false;

    const std::string serverHash = __hyprland_api_get_hash();
    const std::string clientHash = __hyprland_api_get_client_hash();
    if (serverHash != clientHash) {
        const std::string message = "hyprion: API mismatch (server=" + serverHash + ", client=" + clientHash + ")";
        HyprlandAPI::addNotification(handle, message, CHyprColor{1.0, 0.2, 0.2, 1.0}, 5000);
        throw std::runtime_error(message);
    }

    if (!registerHyprionConfig(handle))
        throw std::runtime_error("hyprion: failed to register configuration values");

    if (g_pHyprRenderer && g_pHyprRenderer->type() != Render::IHyprRenderer::RT_GL)
        throw std::runtime_error("hyprion requires Hyprland's OpenGL renderer");
    if (g_pHyprRenderer && !initializeHyprionRenderer())
        throw std::runtime_error("hyprion: failed to initialize the GPU sampling shader");

    auto shadowMethods = HyprlandAPI::findFunctionsByName(handle, "getRenderData");
    std::erase_if(shadowMethods, [](const auto& match) { return !match.demangled.starts_with("CHyprDropShadowDecoration::getRenderData("); });
    if (shadowMethods.size() != 1)
        throw std::runtime_error("hyprion: cannot locate native shadow rendering");
    shadowHook = HyprlandAPI::createFunctionHook(handle, shadowMethods.front().address, reinterpret_cast<void*>(&shadowRenderData));
    if (!shadowHook || !shadowHook->hook())
        throw std::runtime_error("hyprion: cannot hook native shadow rendering");

    auto shadowDrawMethods = HyprlandAPI::findFunctionsByName(handle, "draw");
    std::erase_if(shadowDrawMethods, [](const auto& match) { return !match.demangled.starts_with("CHyprDropShadowDecoration::draw("); });
    if (shadowDrawMethods.size() != 1)
        throw std::runtime_error("hyprion: cannot locate native shadow pass scheduling");
    shadowDrawHook = HyprlandAPI::createFunctionHook(handle, shadowDrawMethods.front().address, reinterpret_cast<void*>(&drawShadow));
    if (!shadowDrawHook || !shadowDrawHook->hook())
        throw std::runtime_error("hyprion: cannot hook native shadow pass scheduling");

    if (g_pEventLoopManager) {
        animationTimer = makeShared<CEventLoopTimer>(std::nullopt, onAnimationTimer, nullptr);
        g_pEventLoopManager->addTimer(animationTimer);
    }

    listeners.emplace_back(Event::bus()->m_events.window.open.listen([](const PHLWINDOW& window) { attachDecoration(window, true); }));
    listeners.emplace_back(Event::bus()->m_events.window.close.listen([](const PHLWINDOW& window) {
        std::erase_if(shadowWindows, [&](const auto& entry) { return entry.second.expired() || entry.second.lock() == window; });
    }));
    listeners.emplace_back(Event::bus()->m_events.window.active.listen(onFocusChanged));
    listeners.emplace_back(Event::bus()->m_events.window.urgent.listen(onUrgencyChanged));
    listeners.emplace_back(Event::bus()->m_events.monitor.destroyMon.listen([](const PHLMONITOR& monitor) { destroyHyprionMonitor(monitor); }));
    listeners.emplace_back(Event::bus()->m_events.config.reloaded.listen(onConfigReloaded));
    listeners.emplace_back(Event::bus()->m_events.render.pre.listen([](PHLMONITOR monitor) {
        if (!pluginActive || !monitor || !monitor->m_blurFBDirty || monitor->m_blurFBShouldRender || !hyprionConfig().outsetBlur)
            return;
        // Native preRender skips opaque windows. Hyprion is an independent
        // consumer, so request the same shared cache even without window blur.
        for (const auto& window : Desktop::windowState()->windows()) {
            if (!window->m_isMapped || !window->visible() || window->m_X11DoesntWantBorders ||
                !window->m_ruleApplicator->decorate().valueOrDefault() ||
                Fullscreen::controller()->getFullscreenModes(window).internal == Fullscreen::FSMODE_FULLSCREEN ||
                !g_pHyprRenderer->shouldRenderWindow(window, monitor))
                continue;
            monitor->m_blurFBShouldRender = true;
            g_pHyprRenderer->damageMonitor(monitor);
            break;
        }
    }));

    pluginActive = true;
    lastFocusedWindow = Desktop::focusState()->window();
    HyprlandAPI::reloadConfig();

    for (const auto& window : Desktop::windowState()->windows())
        attachDecoration(window, false);

    return {"hyprion", "Hyprion visual effects for Hyprland", "Suvikyi", "0.2.0"};
}

APICALL EXPORT void PLUGIN_EXIT() {
    pluginActive = false;
    if (animationTimer) {
        animationTimer->updateTimeout(std::nullopt);
        if (g_pEventLoopManager)
            g_pEventLoopManager->removeTimer(animationTimer);
        animationTimer.reset();
    }

    listeners.clear();
    if (shadowDrawHook) {
        HyprlandAPI::removeFunctionHook(pluginHandle, shadowDrawHook);
        shadowDrawHook = nullptr;
    }
    if (shadowHook) {
        HyprlandAPI::removeFunctionHook(pluginHandle, shadowHook);
        shadowHook = nullptr;
    }
    shadowWindows.clear();
    lastFocusedWindow = PHLWINDOWREF{};
    if (g_pHyprRenderer)
        g_pHyprRenderer->m_renderPass.removeAllOfType("hyprion");
    destroyHyprionRenderer();
    pluginHandle = nullptr;
}

