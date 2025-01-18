# Latency investigations

We can control frame pacing with swapchain frame latency and present (at time or vsync interval) ?

## Frame latency control cpu/gpu parallelism:
If we target 60 fps and cpu + gpu work can be done in less that 16 ms, a frame latency of 1 is good.
If we target 60 fps and the sum cpu + gpu work is greater than 16 ms but each is less than 16 ms a frame latency of 2 is good.

If one of the component is more than 16 ms we should target a lower frame rate thanks to present parameters.

## Present parameters

There are used to control the vsync intervals at wich we will present.
On MacOS, this is done by changing the CAMetalLayerDisplayLink refreshrate range.
On Windows, this is done with present(MULTIPLE OF REFRESH RATE) ? In this case, we need to compute the present interval correctly based
on the last present timing.

We can only choose a refresh which is a multiple of the screen max refresh rate.

## Warnings check

If we detect that the update swapchain method or present have missed the deadline based on the 2 parameters we should
output a warning.

## Ideas

We could detect the CPU update part of the rendering loop automatically. When the user code is calling present() we can compute it by substracting:
presenttime - time of the start of update.

## Parameters
**Frametime: 8ms**
**Latency: 1 frame**

### Scenarios:

1) CPU + GPU Work done in 4 MS 
2) CPU 2MS + GPU 10 MS => Miss next vsync for one frame
3) Same but for several frames
4) CPU 6MS + GPU 4 MS => Miss next vsync

Do the same for 2 frame latency

Also if we know we can do 30 FPS. how can we do that on a 120hz screen
