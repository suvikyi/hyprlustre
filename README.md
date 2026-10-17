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

## Configure

Hyprlustre registers everything under `plugin.lustre`. `inset` and `outset` are configured independently and neither inherits from the other; anything you leave out keeps its shipped default.

```lua
hl.config({
  plugin = {
    lustre = {
      border = {
        outset = {
          enabled = true,
          size = 2,
          active = {
            lightness = 0.4,
            tint = { color = accent, strength = 1 },
          },
        },
      },
    },
  },
})
```

## Settings

Defaults are written `inset / outset` where the two placements differ. Both sample the pixels already behind the border, convert them to OKLab, transform them, and blend the result back over the live border.

### Placement

- **`enabled`** — draw this border.
- **`size`** — border thickness in logical pixels, `1..16`.
- **`blur`** — *outset only.* Take the border's backdrop from Hyprland's blur, using the same source Hyprland picks for that window: with `decoration.blur.new_optimizations` a tiled window reads the shared pre-window cache (desktop and layers, excluding application windows and the window's own shadow), otherwise the framebuffer is blurred live, which includes the windows underneath. Requires `decoration.blur.enabled`; explicit `xray` rules are honoured.
- **`transition.time`** — focus-transition duration in milliseconds, `0..2000`. Covers both directions, including opacity. A transition starts from the currently displayed style, so switching focus mid-transition or mid-pulse never snaps the border to a preset.

### State colour

These keys exist on every state. `active` and `inactive` style the focused and unfocused border, `pulse` is the peak of the one-shot focus pulse, and the four `orbit.*` profiles style the traveling highlight.

- **`lightness`** — additive OKLab lightness, `-1..1`. `0` unchanged, positive brightens.
- **`saturation`** — OKLab chroma multiplier, `0..255`. `0` greyscale, `1` unchanged, `1.2` +20%.
- **`chroma`** — additive OKLab chroma, `-1..1`. Positive adds chroma, negative drains it. Only reshapes chroma the sample already has, so a neutral pixel stays neutral; use `tint` to colour a neutral border.
- **`hue`** — OKLab hue rotation in degrees, `-180..180`. Turns the sampled colours without introducing new ones.
- **`opacity`** — blend strength of the transform, `0..1`.
- **`tint.color`** / **`tint.strength`** — blend the sample's chroma toward `tint.color` in OKLab while keeping its lightness. `strength` is `0..255`; `1` is the tint colour at the sample's lightness, `0` disables tinting. `color` uses Hyprland colour syntax.

`active.tint` and `inactive.tint` follow their state through focus transitions, interpolating colour and strength. `pulse.tint` is an absolute peak target like the rest of the pulse: it blends toward its own colour and strength and returns to the ongoing state tint, so setting it equal to the state's tint leaves the tint unchanged.

### Pulse

- **`pulse.enabled`** — run the one-shot focus pulse.
- **`pulse.time`** — pulse duration in milliseconds, `0..2000`.
- **`pulse.lightness`** / **`pulse.saturation`** / **`pulse.chroma`** / **`pulse.hue`** / **`pulse.opacity`** — absolute peak targets, reached halfway through the pulse and faded back toward the ongoing active transition.

Setting `pulse.enabled = false`, or a duration of zero, disables the pulse. For a white pulse even over black, set `pulse.lightness = 1.0` and `pulse.saturation = 0.0` with both `opacity` values at `1.0`, so the pulse is not blended with the underlying pixels.

### Orbit

- **`orbit.<profile>.enabled`** — run that profile. `active` is the focused window, `inactive` is non-urgent unfocused windows, `urgent` takes precedence for any urgent window, and `flare` ignites once when a window gains focus.
- **`orbit.<profile>.clockwise`** — travel clockwise instead of counter-clockwise.
- **`orbit.<profile>.time`** — milliseconds per full lap, `1..120000`.
- **`orbit.<profile>.length`** — highlighted fraction of the perimeter, `0..1`. The segment follows rounded and superellipse corners.
- **`orbit.<profile>.motion.amount`** — perimeter speed variation, `0..0.95`. `0` is uniform travel; larger values strengthen acceleration and deceleration.
- **`orbit.<profile>.motion.cycles`** — integer `1..4`, speed waves per lap.
- **`orbit.flare.fade.inMs`** / **`orbit.flare.fade.outMs`** — *flare only.* Fade the single ignition lap in and out, in milliseconds. `outMs` is anchored to the end of the lap, so the orbit reaches full transparency exactly as the lap completes rather than lingering afterwards; both fades are scaled down together if they would overrun the lap.

Each profile also takes the state-colour keys above, as absolute targets blended over the ongoing border transform. Activating a profile fades in with `transition.time` without resetting position, and changing `clockwise` or `motion` preserves the current position. `flare` laps exactly once, then hands back to the focused window's profile — or stays gone if that profile is disabled — so it works even when `orbit.active.enabled` is `false`.

### Defaults

`saturation` is `1` and `hue` is `0` in every state. `tint.color` is `rgb(33ccff)` — Hyprland's own default glow blue — in every state, and `tint.strength` is `0` for every `inset` state (untinted) and `1` for every `outset` state.

| State | `lightness` | `chroma` | `opacity` |
| --- | ---: | ---: | ---: |
| `active` | 0.175 / 0.4 | 0.05 | 1 / 0.85 |
| `inactive` | 0.12 / 0.225 | 0.05 | 1 / 0.75 |
| `pulse` | 0.45 / 0.375 | 0.1 / 0.05 | 1 |
| `orbit.active` | 0.5 / 0.425 | 0.1 / 0.05 | 0.618 / 1 |
| `orbit.urgent` | 0.5 / 0.425 | 0.05 | 0.5 / 1 |
| `orbit.inactive` | 0.5 / 0.425 | 0.1 / 0.05 | 0.618 / 1 |
| `orbit.flare` | 0.5 / 0.475 | 0.1 | 0.75 / 1 |

| Placement | `enabled` | `size` | `transition.time` | `blur` |
| --- | --- | ---: | ---: | --- |
| `inset` | false | 1 | 333 | — |
| `outset` | true | 2 | 333 | true |

`pulse.enabled` is `true` in both placements.

| Orbit profile | `enabled` | `time` | `length` | `motion.amount` | `motion.cycles` |
| --- | --- | ---: | ---: | ---: | ---: |
| `orbit.active` | true | 30000 | 0.154 | 0.75 / 0.5 | 2 |
| `orbit.urgent` | true | 3000 | 0.154 | 0.75 / 0.5 | 2 |
| `orbit.inactive` | false | 30000 | 0.154 | 0.75 / 0.5 | 2 |
| `orbit.flare` | true | 667 | 0.309 | 0.75 / 0.618 | 2 |

`clockwise` is `true` in every orbit profile, and `orbit.flare.fade.inMs` and `fade.outMs` are both `0`.

## Credits

[Suvikyi](https://github.com/suvikyi)  
GPT-6.1 Sol  
GPT-6 Luna
