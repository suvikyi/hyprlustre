#pragma once

#include "LustreAnimation.hpp"

#include <hyprland/src/desktop/view/Window.hpp>
#include <hyprland/src/output/Monitor.hpp>
#include <hyprland/src/render/decorations/IHyprWindowDecoration.hpp>

class CLustrePassElement;

bool  initializeLustreRenderer();
void  destroyLustreRenderer();
void  destroyLustreMonitor(PHLMONITOR monitor);

class CLustreDecoration final : public IHyprWindowDecoration {
  public:
    explicit CLustreDecoration(PHLWINDOW window);
    ~CLustreDecoration() override;

    SDecorationPositioningInfo getPositioningInfo() override;
    void                       onPositioningReply(const SDecorationPositioningReply& reply) override;
    void                       draw(PHLMONITOR monitor, float const& alpha) override;
    void                       drawOutset(PHLMONITOR monitor, float alpha);
    eDecorationType            getDecorationType() override;
    void                       updateWindow(PHLWINDOW window) override;
    void                       damageEntire() override;
    eDecorationLayer           getDecorationLayer() override;
    uint64_t                   getDecorationFlags() override;
    std::string                getDisplayName() override;

    void stateChanged(bool focused, bool urgent, const SLustreConfig& config, CLustreAnimation::Time now);
    void refreshAnimation(bool focused, bool urgent, const SLustreConfig& config, CLustreAnimation::Time now);
    bool animate(CLustreAnimation::Time now);
    void resetAnimation(bool focused, bool urgent, CLustreAnimation::Time now);

  private:
    CBox monitorLocalBox(PHLMONITOR monitor) const;
    void  drawPass(PHLMONITOR monitor, float alpha, const CBox& monitorLocalBox, bool outset, bool snapshotOnly);
    void  damageBorder();
    void  updateOrbitArcTable(float power);

    PHLWINDOWREF m_window;
    std::array<CLustreAnimation, 2> m_animations;
    int m_outsetExtent = 0;
    bool m_outsetSnapshotReady = false;
    std::array<float, 65> m_orbitArcTable{};
    float m_orbitArcPower = -1.F;

    friend class CLustrePassElement;
};
