# Hello Inputs

This sample keeps the simple triangle from Hello Triangle and adds cross-platform input handling.

The important part is not the triangle itself: it is the path from Elemental's raw input event stream to application actions that can be shared across keyboard, mouse, gamepad and touch input.

## What it demonstrates

- reading `ElemInputStream` every frame;
- consuming `ElemInputEvent` values;
- mapping several physical inputs to the same logical action;
- keyboard, mouse, gamepad and touch input;
- press/release, toggle and double-release behavior;
- cursor visibility and application exit;
- driving shader parameters from input.

The small action-binding layer in `main.c` belongs to this sample. It is an example of how application code can consume Elemental input events, not a required high-level input model imposed by Elemental.

## Controls

| Input | Action |
|:--|:--|
| `W` / `S` | Rotate up / down |
| `A` / `D` | Rotate left / right |
| `Q` / `E` | Roll |
| `Z` / `X` | Zoom |
| `Space` | Change triangle color |
| `F1` | Toggle cursor visibility |
| `Escape` | Exit |
| Mouse drag / wheel | Rotate / zoom |
| Mouse double-click | Change triangle color |

Gamepad and touch bindings provide equivalent rotation, zoom and action controls. Two-finger touch input is used for pinch/rotation gestures.

## Key files

- [`main.c`](main.c) — input bindings, action updates and triangle rendering.
- [`Data/Triangle.hlsl`](Data/Triangle.hlsl) — mesh and pixel shaders driven by the current input state.

## Build

```bash
cmake --build --preset default --target HelloInputs
```

The sample also accepts `--vulkan`, primarily to select Vulkan instead of the default Direct3D 12 backend on Windows.

[Back to samples](../../README.md)
