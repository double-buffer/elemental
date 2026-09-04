# AI Training Demo

A small neural-network and automatic-differentiation experiment written in C.

This demo is currently a **CPU baseline**. It does not run training or tensor operations on the GPU yet. Its role is to keep the learning experiment small and understandable before exploring how the same kind of workload could use Elemental compute and future GPU linear-algebra capabilities.

## NeuralNetV0

`NeuralNetV0.c` implements a tiny scalar automatic-differentiation engine from scratch:

- values stored behind small integer handles;
- an operation graph with parent dependencies;
- reverse-mode gradient propagation;
- addition, multiplication, subtraction, division, power, exponential and `tanh` operations;
- neurons, layers and a small multilayer neural network;
- forward pass, squared-error loss, backward pass and parameter updates.

The current network is:

```text
3 inputs
    -> 4 neurons
    -> 4 neurons
    -> 1 output
```

It trains for 20 steps on four small input/output examples and prints the loss and final predictions to the console.

## NeuralNetV1

`NeuralNetV1.c` is currently only a skeleton for the next iteration. The forward, backward and parameter-update stages are intentionally unfinished.

## Why this is in Elemental

The interesting future experiment is not to turn Elemental into a machine-learning framework. It is to start from code this small and ask what the minimal GPU programming path should look like when selected tensor or neural-network operations move to modern GPU hardware.

That GPU version does not exist in this sample yet.

## Key files

- [`NeuralNetV0.c`](NeuralNetV0.c) — working CPU autodiff and neural-network experiment.
- [`NeuralNetV1.c`](NeuralNetV1.c) — next-iteration placeholder.
- [`main.c`](main.c) — sample application entry point.

## Build

```bash
cmake --build --preset default --target AITraining
```

[Back to samples](../../README.md)
