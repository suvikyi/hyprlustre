# Hyprlustre

Hyprlustre is a Hyprland visual-effects plugin. The border is its first effect; the feature-scoped API leaves room for window effects and animations.

Rounded inset and outset borders sample and transform the pixels beneath them in OKLab, producing a reactive glaze. Each placement has independent active, inactive, pulse, tint and orbit settings.

The inset border overlays the window surface; the outset border occupies reserved space immediately outside it and samples the composited background before the window's own shadow, not an extension of the window texture. Its rounded coverage is composited afterward using Hyprland's native border antialiasing. Both can be enabled together. Hyprland's native border is optional: when enabled, it wraps the outset border; set `general.border_size = 0` to omit it. Native shadow extents and corner radii include the outset.

Requires **Hyprland 0.56.2**, matching development headers, and the OpenGL renderer.

## Install

Add the published Git repository, then enable Hyprlustre:

```sh
hyprpm add https://github.com/suvikyi/hyprlustre
hyprpm enable lustre
```

Hyprpm builds the plugin using headers matching your Hyprland installation. Run `hyprpm update` to refresh its headers and plugins when needed.

### Manual build

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

The output is `build/liblustre.so`. A manual build does not register the plugin with Hyprpm.

## Demo

Run the showcase from a terminal inside Omarchy/Hyprland:

```sh
python omarchy_showcase.py
```

It opens three `foot` terminals running `cliamp`, Neovim, and `btop`. Neovim displays a temporary Hyprlustre notes file in `--clean` mode. `foot`, `cliamp`, `nvim`, `btop`, and `hyprctl` must be available.

The demo cycles the full Omarchy theme through **Miasma**, **Tokyo Night**, and **Catppuccin** using the native `omarchy theme set` command—not per-app palette overrides. Each switch applies Omarchy's normal backgrounds and configured theme integrations, including remote mirroring if enabled. The starting theme is restored on exit unless you changed to a different theme yourself while the demo was running. Set the dwell time with `--theme-seconds 10` (minimum 5).

For a recorded focus cycle, run `python omarchy_showcase.py --record --cycle-focus`. This uses Omarchy's native fullscreen screen recorder on the focused monitor and stops after 45 seconds; change the limit with `--record-seconds 60`. Focus cycling is optional because Hyprland's focus dispatcher may move the pointer. Orbit and outset blur still require their settings in your existing config; urgent orbit needs an actual client urgency request, which the demo does not fabricate. `Ctrl+C` closes the demo terminals.

## Configure

Hyprlustre registers settings under `plugin.lustre`. These are the defaults:

```lua
hl.config({
  plugin = {
    lustre = {
      enabled = true,
      border = {
        inset = {
          enabled = true,
          size = 1,
          transition = { time = 333 },
          active = {
            lightness = 0.175,
            saturation = 3.5,
            opacity = 1.0,
            tint = {
              enabled = false,
              color = "rgb(ff8066)",
              strength = 0.18,
            },
          },
          inactive = {
            lightness = 0.1,
            saturation = 3.5,
            opacity = 1.0,
            tint = {
              enabled = false,
              color = "rgb(ff8066)",
              strength = 0.18,
            },
          },
          pulse = {
            time = 333,
            lightness = 0.375,
            saturation = 5.0,
            tint = {
              enabled = false,
              color = "rgb(ff8066)",
              strength = 0.18,
            },
          },
          orbit = {
            active = {
              enabled = false,
              clockwise = true,
              time = 12000,
              motion = { amount = 0.35, cycles = 1 },
              length = 0.12,
              lightness = 0.25,
              saturation = 4.0,
              opacity = 1.0,
              tint = {
                enabled = false,
                color = "rgb(ff8066)",
                strength = 0.25,
              },
            },
            urgent = {
              enabled = false,
              clockwise = true,
              time = 5000,
              motion = { amount = 0.15, cycles = 1 },
              length = 0.18,
              lightness = 0.375,
              saturation = 5.0,
              opacity = 1.0,
              tint = {
                enabled = false,
                color = "rgb(ff8066)",
                strength = 0.25,
              },
            },
          },
        },
        outset = { enabled = false, blur = false },
      },
    },
  },
})
```

`lightness` adds directly to OKLab lightness and is clamped to `-1..1`: `0` is unchanged, positive values brighten, and negative values darken. `saturation` multiplies OKLab chroma and is clamped to `0..255`: `0` is grayscale, `1` is unchanged, and `1.2` increases chroma by 20%. `opacity` is the transformed-pixel blend strength, clamped to `0..1`.

`active.tint` and `inactive.tint` independently blend sampled pixel chroma toward their configured colors in OKLab while preserving lightness. Focus transitions interpolate tint color and strength. `pulse.tint` is an optional peak tint: when enabled, the one-shot focus pulse blends toward its color and strength, then returns to the ongoing state tint; when disabled, the pulse leaves the state tint unchanged. Each has an independent `enabled`, `color`, and `strength`; strength ranges from `0` (none) to `1` (full target chroma). All three tints are disabled by default.

`transition.time` controls both focus directions, including opacity. A focus change starts from the currently displayed style, so switching focus during a transition or pulse does not snap the border to a preset.

`border.inset.orbit.active` selects the orbit for the focused window. The urgent profile takes precedence for any urgent window, focused or not. Nonurgent inactive windows have no orbit. `time` is milliseconds per full lap and `length` is the highlighted fraction of the complete perimeter. The segment follows rounded and superellipse corners. Lightness, saturation, opacity, and tint are absolute targets blended over the ongoing border transform. Activation fades with `transition.time` without resetting position.

Each orbit profile defaults to `clockwise = true`; set it to `false` for counter-clockwise travel. Changing direction reverses motion immediately without resetting the highlight's position.

`motion` smoothly varies perimeter speed without stopping or changing lap duration. `amount` is clamped to `0..0.95`: `0` gives uniform travel; larger values strengthen acceleration and deceleration. `cycles` is an integer `1..4`, controlling speed waves per lap. Active defaults to `0.35`, urgent to `0.15`, both with one cycle. Changing motion settings preserves the current position, and both directions use the same wave.

`pulse.time` controls a one-shot pulse on focus gain. Pulse lightness and saturation are absolute peak transformation targets, reached halfway through the pulse, then faded back toward the ongoing active transition. Pulse does not change opacity. Setting its duration to zero disables it; durations are in milliseconds.

For a white pulse even over black, set `pulse.lightness = 1.0` and `pulse.saturation = 0.0`, with both style opacities at `1.0` so the pulse is not blended with the underlying pixels.

## Settings

`inset` and `outset` expose the shared settings below and do not inherit from each other. Defaults are identical except `enabled`: inset is `true`, outset is `false`. Replace `inset` with `outset` in the table paths to configure the latter. Each placement animates independently.

Only outset supports `blur = true`. It samples Hyprland's shared background/bottom-layer blur cache before applying the border's effects, excluding application windows and the window's own shadow. It inherits native blur settings and requires both `decoration.blur.enabled` and `decoration.blur.new_optimizations`; otherwise it leaves the border unblurred. There is no per-window live-blur fallback. Blur is disabled by default.

| Setting | Default | Range |
| --- | ---: | ---: |
| `plugin.lustre.enabled` | `true` | — |
| `plugin.lustre.border.outset.blur` | `false` | boolean; requires native optimized blur |
| `plugin.lustre.border.inset.enabled` | `true` | — |
| `plugin.lustre.border.inset.size` | `1` | `1..16` logical pixels |
| `plugin.lustre.border.inset.active.lightness` | `0.175` | `-1..1` |
| `plugin.lustre.border.inset.active.saturation` | `3.5` | `0..255` |
| `plugin.lustre.border.inset.active.opacity` | `1.0` | `0..1` |
| `plugin.lustre.border.inset.active.tint.enabled` | `false` | — |
| `plugin.lustre.border.inset.active.tint.color` | `rgb(ff8066)` | Hyprland color |
| `plugin.lustre.border.inset.active.tint.strength` | `0.18` | `0..1` |
| `plugin.lustre.border.inset.inactive.lightness` | `0.1` | `-1..1` |
| `plugin.lustre.border.inset.inactive.saturation` | `3.5` | `0..255` |
| `plugin.lustre.border.inset.inactive.opacity` | `1.0` | `0..1` |
| `plugin.lustre.border.inset.inactive.tint.enabled` | `false` | — |
| `plugin.lustre.border.inset.inactive.tint.color` | `rgb(ff8066)` | Hyprland color |
| `plugin.lustre.border.inset.inactive.tint.strength` | `0.18` | `0..1` |
| `plugin.lustre.border.inset.transition.time` | `333` ms | `0..2000` |
| `plugin.lustre.border.inset.pulse.time` | `333` ms | `0..2000` |
| `plugin.lustre.border.inset.pulse.lightness` | `0.375` | `-1..1` |
| `plugin.lustre.border.inset.pulse.saturation` | `5.0` | `0..255` |
| `plugin.lustre.border.inset.pulse.tint.enabled` | `false` | — |
| `plugin.lustre.border.inset.pulse.tint.color` | `rgb(ff8066)` | Hyprland color |
| `plugin.lustre.border.inset.pulse.tint.strength` | `0.18` | `0..1` |
| `plugin.lustre.border.inset.orbit.active.enabled` | `false` | — |
| `plugin.lustre.border.inset.orbit.active.clockwise` | `true` | Boolean |
| `plugin.lustre.border.inset.orbit.active.time` | `12000` ms | `1..120000` ms per lap |
| `plugin.lustre.border.inset.orbit.active.motion.amount` | `0.35` | `0..0.95` |
| `plugin.lustre.border.inset.orbit.active.motion.cycles` | `1` | Integer `1..4` |
| `plugin.lustre.border.inset.orbit.active.length` | `0.12` | `0..1` perimeter fraction |
| `plugin.lustre.border.inset.orbit.active.lightness` | `0.25` | `-1..1` |
| `plugin.lustre.border.inset.orbit.active.saturation` | `4.0` | `0..255` |
| `plugin.lustre.border.inset.orbit.active.opacity` | `1.0` | `0..1` |
| `plugin.lustre.border.inset.orbit.active.tint.enabled` | `false` | — |
| `plugin.lustre.border.inset.orbit.active.tint.color` | `rgb(ff8066)` | Hyprland color |
| `plugin.lustre.border.inset.orbit.active.tint.strength` | `0.25` | `0..1` |
| `plugin.lustre.border.inset.orbit.urgent.enabled` | `false` | — |
| `plugin.lustre.border.inset.orbit.urgent.clockwise` | `true` | Boolean |
| `plugin.lustre.border.inset.orbit.urgent.time` | `5000` ms | `1..120000` ms per lap |
| `plugin.lustre.border.inset.orbit.urgent.motion.amount` | `0.15` | `0..0.95` |
| `plugin.lustre.border.inset.orbit.urgent.motion.cycles` | `1` | Integer `1..4` |
| `plugin.lustre.border.inset.orbit.urgent.length` | `0.18` | `0..1` perimeter fraction |
| `plugin.lustre.border.inset.orbit.urgent.lightness` | `0.375` | `-1..1` |
| `plugin.lustre.border.inset.orbit.urgent.saturation` | `5.0` | `0..255` |
| `plugin.lustre.border.inset.orbit.urgent.opacity` | `1.0` | `0..1` |
| `plugin.lustre.border.inset.orbit.urgent.tint.enabled` | `false` | — |
| `plugin.lustre.border.inset.orbit.urgent.tint.color` | `rgb(ff8066)` | Hyprland color |
| `plugin.lustre.border.inset.orbit.urgent.tint.strength` | `0.25` | `0..1` |

## Credits

[Suvikyi](https://github.com/suvikyi)  
GPT-6.1 Sol  
GPT-6 Luna
