---
title: OpenCL/OpenGL Interop Particle System
description: Notes + walkthrough for a particle system where OpenCL updates and OpenGL renders using shared GPU buffers (interop).
---

<!-- This page uses Zensical front matter (--- ... ---) and admonitions (!!! tip, !!! note, etc.). [web:37][web:42] -->

# OpenCL/OpenGL Interop Particle System

This site is a dev-log + tutorial for a GPU particle system where **OpenCL does the sim** and **OpenGL does the drawing**, and the two share buffers so nothing has to bounce through the CPU.

!!! tip "What “interop” means here"
    OpenGL owns the VBOs (for rendering), OpenCL temporarily acquires those same buffers, writes new particle data, releases them back, then OpenGL draws the updated data.

!!! note "Goal"
    Keep particle positions/colors on the GPU the whole time: simulate in OpenCL, render in OpenGL, repeat.

---

## Getting started

### What you need

### Build
```bash
cmake -B build
cmake --build build
```

### Run 
```bash
build\bin\Debug\OpenGLApp.exe
./build/bin/OpenGLApp
```

### Controls

---

## Project layout

Suggested structure for a Zensical docs site + code repo:

```text
├─ src/				# Libraries, Utils, and Shaders
├─ .clang-format 	# Style Guide
├─ .clangd				
├─ CMakeLists.txt
├─ main.cpp			<---- You will edit this file
└─ particles.cl 	<---- You will edit this file	
```

!!! tip "How I write pages"
    One page = one idea. Keep the main loop readable and link out to deeper dives (buffers, sync, kernel math).

---

## The core idea

### Data per particle
We’ll keep three arrays on the GPU:

- Position: `float4 (x, y, z, 1)`
- Velocity: `float4 (vx, vy, vz, 0)`
- Color: `float4 (r, g, b, a)`

!!! info "Why float4?"
    OpenCL has `float4` built-in, and the extra component is handy for alignment and convenience.

### Simulation step (high level)
Every frame (or fixed timestep):

1. Apply gravity
2. Integrate velocity + position
3. Handle collisions (sphere “bumpers” to start)
4. Optionally update color based on “events” (like collisions or speed)

---

## OpenCL/OpenGL buffer sharing

### What gets shared
OpenGL creates VBOs like:
- positions VBO (rendered as points / billboards)
- colors VBO (optional)

OpenCL gets a handle to those buffers so the kernel can write directly into them.

### Create a shared VBO (sketch)
```cpp
// OpenGL: create VBO
GLuint posVbo = 0;
glGenBuffers(1, &posVbo);
glBindBuffer(GL_ARRAY_BUFFER, posVbo);
glBufferData(GL_ARRAY_BUFFER, numParticles * sizeof(float) * 4, nullptr, GL_DYNAMIC_DRAW);

// OpenCL: wrap GL buffer
cl_int err = CL_SUCCESS;
cl_mem clPos = clCreateFromGLBuffer(clContext, CL_MEM_READ_WRITE, posVbo, &err);
```

!!! tip "Rule of thumb"
    Let OpenGL “own” the buffer lifetime. OpenCL just borrows it via acquire/release each frame.

### The per-frame or multiple “interop dance”
```cpp
// Acquire GL buffers for OpenCL
clEnqueueAcquireGLObjects(queue, 1, &clPos, 0, nullptr, nullptr);

// Run kernel(s) that update positions/colors
// clSetKernelArg(...)
// clEnqueueNDRangeKernel(...)

// Release buffers back to OpenGL
clEnqueueReleaseGLObjects(queue, 1, &clPos, 0, nullptr, nullptr);

// Make sure CL is done before GL draws from the buffer
clFinish(queue);
```

!!! warning "Sync matters"
    If you see flicker, one-frame lag, or random garbage: it’s usually missing/incorrect sync around acquire/release (or you’re writing out of bounds).

---

## The kernel (simple version)

### Inputs/outputs
- `dPos` (shared buffer): read+write
- `dVel` (OpenCL-only buffer): read+write
- `dCol` (shared buffer): optional write

### Pseudocode
```c
kernel void StepParticles(
    global float4* dPos,
    global float4* dVel,
    global float4* dCol,
    float dt,
    float4 gravity,
    float4 sphere0,   // (cx, cy, cz, r)
    float4 sphere1
) {
    int id = get_global_id(0);

    float4 p = dPos[id];
    float4 v = dVel[id];

    // Integrate
    v += gravity * dt;
    p += v * dt;
    p.w = 1.0f;
    v.w = 0.0f;

    // Collide with spheres (outline)
    // if (insideSphere(p, sphere0)) v = bounce(...);
    // if (insideSphere(p, sphere1)) v = bounce(...);

    // Optional: color update
    // dCol[id] = ...

    dPos[id] = p;
    dVel[id] = v;
}
```

!!! note "Start with something boring"
    First make particles fall correctly. Then add one sphere collision. Then add the second sphere. Then add colors.

---
