# OpenGL Particle System

A cross-platform particle system built with OpenGL and OpenCL.
For a full breakdown of the project, see the [write-up](https://johnklucinec.github.io/OpenCL-OpenGL-Particle-System/#platform-setup).

---

### Stack

| | |
|---|---|
| **Graphics** | OpenGL 4.1 Core |
| **Compute** | OpenCL 1.2 |
| **Windowing** | GLFW + GLAD |
| **UI** | ImGui |
| **Build** | CMake |

---

### What's Already Done
Joe Parallel has already:
- Set up the OpenGL window, shaders, VAOs, and VBOs
- Created and connected the OpenCL context to those buffers
- Randomized all initial positions, colors, and velocities
- Written the main loop that calls the `particles.cl` kernel every frame and draws the result

> [!NOTE]
> As explained [here](https://johnklucinec.github.io/OpenCL-OpenGL-Particle-System/#1-set-local-work-group-size), some variables like `LOCAL_SIZE` are set to nonoptimal values. 
---

### Build

```bash
cmake -B build
cmake --build build --config
```

### Rebuild and Run

```bash
# Windows
cmake --build build && build\bin\Debug\OpenGLApp.exe

# macOS / Linux
cmake --build build && ./build/bin/OpenGLApp
```

For a more detailed setup guide, see the [platform-setup](https://johnklucinec.github.io/OpenCL-OpenGL-Particle-System/#platform-setup) part of the write up.

---

### Contributing

To publish a release with the source ZIP attached, tag your commit and push the tag:

```bash
git tag v1.0
git push origin v1.0
```

GitHub Actions will automatically build and attach `source.zip` to the release.
