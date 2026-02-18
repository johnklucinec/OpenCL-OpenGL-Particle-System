---
title: OpenCL/OpenGL Interop Particle System
description: Combine OpenCL and OpenGL to create a GPU-accelerated particle system with shared buffers.
---

# OpenCL/OpenGL Interop Particle System

## Introduction

Particle systems are used in games, films, and visual effects to simulate natural phenomena such as clouds, dust, fireworks, fire, explosions, water flow, sand, swarms of insects, and even herds of animals. Once you understand how they work, you'll notice them everywhere.

A particle system operates by controlling an extensive collection of individual 3D particles to exhibit some behavior. For a deeper technical overview, see [Mike Bailey's slide](https://web.engr.oregonstate.edu/~mjb/cs491/Handouts/particlesystems.2pp.pdf).

!!! quote "Fun Fact"
    Particle systems were first seen in [Star Trek II: The Wrath of Khan Genesis Demo](https://www.youtube.com/watch?v=Qe9qSLYK5q4), animated over 40 years ago. While the technology has evolved significantly since then, this groundbreaking sequence introduced the concept to computer graphics.

---

## Goal

In this project, you will combine OpenCL and OpenGL to create your own particle system.  The project template includes a complete solution for one million (1024 x 1024) particles, one sphere bumper, and no color changes. The files you need to change are `sample.cpp` and `particles.cl`. The degree of "cool-ness" is up to you, but here is a minimum checklist you must complete to get full credit. For more in-depth instructions, see the Requirements section.

- [x] Choose an appropriate `LOCAL_SIZE` value
- [x] Select an optimal `STEPS_PER_FRAME` value  
- [x] Add at least one additional bumper object (sphere)
- [x] Implement dynamic particle color changes
- [x] Test performance with varying particle counts
- [x] Create a graph to visualize your results
- [x] Demonstrate your program in action

!!! tip "What interoperability or "interop" means here"
    OpenGL owns the VBOs (*for rendering*), OpenCL temporarily acquires those same buffers, writes new particle data, releases them back, and then OpenGL draws the updated data.

---


## Getting Started

This project uses CMake as the build system to ensure it runs on Windows, macOS, and Linux. It's written in C++ 17 and uses OpenGL 4.1 and OpenCL 1.2, as these are the latest versions supported on macOS. OpenMP is used for kernel benchmarking. If you need to install any of these, detailed instructions for each operating system are provided below.

#### Project Layout

```text
├─ src/             	# Libraries, utils, and shaders
├─ .clang-format    	# Style Guide
├─ .clangd              
├─ CMakeLists.txt
├─ main.cpp         	# You will edit this file
└─ particles.cl     	# You will edit this file   
```

---

#### Platform Setup

??? info "Windows Installation"
	!!! warning "Requires [Visual Studio](https://visualstudio.microsoft.com/) with the **Desktop development with C++** workload installed"
	
	**Install Dependencies**
	
	Install [CMake](https://cmake.org/download/) (and add to PATH during install)
	
	**Build & Run**
	
	```bash
	cmake -B build
	cmake --build build
	
	build\bin\Debug\OpenGLApp.exe
	```
	
	!!! success "Quick rebuild: `cmake --build build && build\bin\Debug\OpenGLApp.exe`"

??? info "MacOS Installation"

	!!! warning "Assumes you have [Homebrew](https://brew.sh/) installed"
	
	**Configuration**
	
	Uncomment the OpenMP line in `.clangd`:
	
	```yaml title=".clangd" linenums="17" hl_lines="2"
	- "-IopenCL"
	- # "-I/opt/homebrew/opt/libomp/include"  
	- "-DIMGUIZMO_IMGUI_FOLDER=.."
	```
	
	**Install Dependencies**
	
	```bash
	brew install cmake libomp
	export OpenMP_ROOT=$(brew --prefix)/opt/libomp
	```
	
	**Build & Run**
	
	```bash
	cmake -B build
	cmake --build build
	
	./build/bin/OpenGLApp
	```
	
	!!! success "Quick rebuild: `cmake --build build && ./build/bin/OpenGLApp`"
	

??? info "Linux Installation"
    TODO

---

## Requirements

#### 1. Set Local Work-Group Size

Set the local work-group size (`LOCAL_SIZE`) to some fixed value. You can determine an appropriate value by setting `PRINTINFO` to true, which shows the maximum work-group size your GPU supports. Since this particle system uses simple calculations, pick a reasonable power of 2 (like 128 or 256) within that maximum, set it once, and leave it.

___

#### 2. Set Optimal Steps Per Frame

First, it's important to understand how the default program flow works

??? "Program Execution Flow"
	<figure markdown="span">
	  ![Image title](images\interop_light.png#only-light)
	  ![Image title](images\interop_dark.png#only-dark)
			
	<figcaption>
	[Source][source]
  [source]: https://web.engr.oregonstate.edu/~mjb/cs575/Handouts/opencl.opengl.vbo.1pp.pdf#page=2
  "Modified from Mike Bailey"
	</figcaption>

	</figure>
	
Looking at the graph, once we set up the data on the OpenGL side, OpenCL acquires it. As explained in the OpenCL/GL Interoperability notes, synchronization is required because only one can hold the buffer at a time. OpenCL runs a kernel that reads an x, y, and z value, updates it according to projectile motion laws, writes it back to the buffer, and releases it. OpenGL then draws the buffer, and the cycle repeats.

While this system works, it means only one physics update happens per visual frame. If you're locked to your monitor's refresh rate (*let's assume 60Hz*), you'll draw 60 frames per second, which is your framerate.

But here's the catch: **Just because you can only draw 60 frames per second doesn't mean you can only compute 60 times per second.**

`STEPS_PER_FRAME)` determines how many compute kernels are launched before drawing a frame. We want to know "How fast can my GPU update particles?" not "How many frames can I draw?" When `STEPS_PER_FRAME` is set higher than 1, you're running multiple physics updates per visual frame.

Think of your GPU like a delivery truck. Every frame is a "trip", and every kernel compute is a "package".

- **Make 1 delivery per trip** (`STEPS_PER_FRAME` = 1): You drive to one house, drop off a package, drive all the way back, then wait for the next scheduled trip. Inefficient!
- **Make 10 deliveries per trip** (`STEPS_PER_FRAME` = 10): You drive out once, hit 10 houses, then come back. You're doing 10 times more work in the same wall-clock time.

Your GPU has limits, so setting the value too high will hurt your framerate. Your goal is to find the value that stresses your GPU the most while maintaining fluid motion. An easy way is to increase the value until your FPS drops to around your monitor's refresh rate. Or just pick a really high value (e.g., 1024), and see what happens. You will be running tests with both `STEPS_PER_FRAME = 1;` and `STEPS_PER_FRAME = ???;`, where ??? is the value you chose. 

!!! note "Test with your maximum particle count when finding this value."

---

#### 3. Add Additional Bumper Object

Your simulation must have at least **two** "bumpers" in it for the particles to bounce off of. Each bumper needs to be geometrically designed such that, given particles XYX, you can quickly tell if that particle is inside or outside the bumper. To get the bounce right, each bumper must know its outward-facing surface normal everywhere.

What's the easiest shape for a bumper? A sphere! In computer graphics, we love spheres as they are computationally "nice". It's fast and straightforward to tell if something is inside or outside of a sphere. It's just as straightforward to determine a normal vector for a point on the surface of a sphere, too.

It is OK to assume that the two bumpers are separate from each other; that is, a particle cannot collide with both at the same time.

Your OpenCL `.cl` program must also handle bounces from your (>=2) bumpers. Be sure to draw these bumpers in your `.cpp` program in `#!cpp InitLists()` so that you can see where they are.

!!! warning
    Don't forget to update the `.cl` kernel code with the **same** values you set in your `.cpp` code.

--- 

#### 4. Implement Dynamic Color Changes

The sample OpenCL code does not retrieve the colors, modify them, or restore them. However, your `.cl` kernel needs to change the particles' colors dynamically. You could base this on position, velocity, time, bounce knowledge, etc. The way they change is up to you, but the color of each particle needs to change in some predictable way during the simulation.

I recommend reading [Explaining the Kernel](#explaining-kernel), as it explains the kernel code that you will be updating. Plus, it shows how the `color` type is defined and how color values are read from and written back to the global buffer.

!!! note "OpenGL defines the red, green, and blue components of a color each as a floating-point value between `0.0` and `1.0`"

---

#### 5. Test Performance

Vary the total number of particles from something small-ish (~1024) to something big-ish (~1024*1024) in some increments that will look good on the graph.

If you check the "show performance" box, you will see current, peak, and average measurements. Let the simulation run for a few seconds, and then write down your **Average** GigaParticles/Sec.

---

#### 6. Show Results

Create a table and a graph of Performance vs. Total Number of Particles. Test with at least five different particle counts, ranging from something small (e.g., 1024) to something large (e.g., 1024 × 8192). Run the full set of tests under both `STEPS_PER_FRAME = 1;` and `STEPS_PER_FRAME = ???;`, where ??? is the value you chose. 

??? note "That this will just be one graph with one curve."
	TODO: Add graph example + python code to make one?

	??? example "Want to make a graph with code?"
	
		I used [Python Playground](https://python-playground.com/) to make my graph. Here is the code I used if you want to do something similar. You might have to change the range and tick values to suit your results better. 
		
		```python
		import matplotlib.pyplot as plt
		import numpy as np
		
		# ─── FILL IN YOUR DATA HERE ───────────────────────────────────────────────────
		PARTICLES_1     = [1, 512, 1024, 2048, 3172, 4096, 5120, 6144, 7168, 8192]  # x1024
		GPS_1           = [0.008, 2.95, 4.68, 6.25, 6.74, 7.27, 6.95, 6.35, 9.06, 8.20]
		STEPS_LABEL_1   = "1 steps/frame"
		
		PARTICLES_2     = [1, 512, 1024, 2048, 3172, 4096, 5120, 6144, 7168, 8192]  # x1024
		GPS_2           = [0.033, 5.93, 6.87, 6.84, 6.89, 8.40, 8.42, 8.65, 8.85, 9.04]
		STEPS_LABEL_2   = "5 step/frame"
		# ──────────────────────────────────────────────────────────────────────────────
		
		fig, ax = plt.subplots(figsize=(8, 5))
		
		ax.plot(PARTICLES_1, GPS_1, marker='D', markersize=5,
		        color='orange', label=STEPS_LABEL_1)
		
		ax.plot(PARTICLES_2, GPS_2, marker='D', markersize=5,
		        color='blue', label=STEPS_LABEL_2)
		
		ax.set_xlabel("Number of Particles (x1024)")
		ax.set_ylabel("GigaParticles / Second")
		ax.set_xlim(0, 8192)    # Bottom range
		ax.set_ylim(0.0, 10.0)  # Left range
		ax.set_xticks([0, 2048, 4096, 6144, 8192])                      # Bottom ticks
		ax.set_yticks([round(v, 2) for v in np.arange(0.0, 10.0, 1.0)]) # Left ticks
		ax.yaxis.set_major_formatter(plt.FormatStrFormatter('%.2f'))
		ax.grid(axis='y', linestyle='-', alpha=0.5)
		ax.legend()
		
		plt.tight_layout()
		plt.show()
		```
---

#### 7. Demonstrate your program in action

Make a video of your program in action and be sure it's Unlisted. You can use any video-capture tool you want. If you have never done this before, I recommend Kaltura, for which OSU has a site license for you to use. You can access the Kaltura noteset here. If you use Kaltura, be sure your video is set to Unlisted. If it isn't, then we won't be able to see it, and we can't grade your project.

Sites like YouTube also work. Just make sure your video is either public or unlisted, not **private**.

!!! tip
    Copy and paste your video link into an incognito tab and see if it plays. If it does, you should be good to go.

---

#### 8. Commentary in the PDF file

Your commentary PDF should include:

1. A web link to the video showing your program in action -- be sure your video is Unlisted.
2. What machine did you run this on?
3. What predictable dynamic thing did you do with the particle colors (random changes are not good enough)?
4. Include at least one screenshot of your project in action
5. Show the table and graph.
6. What patterns are you seeing in the performance curve?
7. Why do you think the patterns look this way?
8. What does that mean for the proper use of GPU parallel computing?

---
## Tips

??? abstract "Explaining the Sample Code"
	<div style="display:none;">
	### Explaining Sample Code
	</div>
	
	Joe Parallel has already set up a complete OpenCL/OpenGL particle system for you. The sample code allocates the data, connects it to the GPU, talks to the OpenCL kernel each frame, and then draws the particles.
	
	---
	
	**The Particle Data**
	
	Each particle has a position, a color, and a velocity, all stored as 4-component floats:
	
	```cpp
	struct xyzw { float x, y, z, w; };   // positions and velocities
	struct rgba { float r, g, b, a; };   // colors
	```
	
	Joe reuses `xyzw` for both positions and velocities (it is just four floats). The `w` in positions is the homogeneous coordinate (almost always 1.0). The `a` in colors is alpha (transparency), which is set but not used by the shaders right now.
	
	---
	
	**Where the Data Lives**
	
	There are three main pieces of particle storage:
	```as3
	particle.posGL   // An OpenGL VBO for positions
	particle.colorGL // An OpenGL VBO for colors
	particle.velHost // A host-side array for velocities
	particle.velCL   // A matching OpenCL buffer for velocities
	```

	The position and color VBOs are also turned into OpenCL buffers (`#!as3 particle.posCL`, `#!as3 particle.colorCL`) using `clCreateFromGLBuffer`. That means OpenCL and OpenGL share the same memory for positions and colors, so there is no copying back and forth.
	
	---
	
	**How Particles Get Their Starting Values**
	
	The function `ResetParticles()` is Joe's “initial conditions” step. It:
  
	1. Maps the position VBO with `glMapBuffer`, fills GPU memory directly with random `(x,y,z)` positions in a 3D box, and sets `w = 1.`
	2. Maps the color VBO, fills GPU memory with random bright colors, and sets `a = 1.`
	3. Fills the host velocity array `#!as3 particle.velHost` with random `(vx, vy, vz)` values, then uses `clEnqueueWriteBuffer` to copy those velocities into the OpenCL buffer `#!as3 particle.velCL`.
	
	At this point, all particles have a randomized position, color, and velocity, and both APIs agree on where that data lives.
	
	---
	
	**How OpenCL and OpenGL Share the Work**
	
	Initialization in `InitCL()` does the heavy lifting Joe does not want you to worry about:
	
	- Picks an OpenCL device and creates a context that can share with the current OpenGL context.
	- Creates the OpenCL views of the OpenGL position and color VBOs with `clCreateFromGLBuffer`.
	- Creates the pure-OpenCL velocity buffer, builds the `Particle` kernel from `particles.cl`, and sets up the kernel arguments: position buffer, velocity buffer, color buffer, and the time step `dt`.
	
	After that, the compute side is ready to run; you do not need to change any of this setup to experiment with the kernel.
	
	---
	
	**What Happens Each Frame**
	
	Every frame, `Animate()` runs the simulation step before anything gets drawn:
	
	1. **Pause Check**: If the app is paused, nothing happens.
	2. **Give Buffers to OpenCL**: `clEnqueueAcquireGLObjects` tells OpenGL to stand back while OpenCL updates the shared position and color buffers.
	3. **Set the Time Step**: A small `dt` is computed (`BASE_DT / STEPS_PER_FRAME`) and passed as kernel argument 3.
	4. **Run the Kernel**: The `Particle` kernel is launched `STEPS_PER_FRAME` times over all `NUM_PARTICLES`. Each work-item handles one particle and updates its position, velocity, and (optionally) color.
	5. **Give Buffers Back to OpenGL**: `clEnqueueReleaseGLObjects` hands the shared buffers back, and `clFinish` waits for everything to complete.
	
	By the time this function returns, the OpenGL VBOs contain the new particle positions and colors ready to be drawn.
	
	---
	
	**How the Scene Gets Drawn**
	
	The `Display()` function handles all of the drawing:
	
	- Sets up the camera and projection (perspective or orthographic).
	- Draws the wireframe spheres that act as bumpers for the particles (the same spheres are defined in the OpenCL kernel for collision).
	- Binds the particle VAO and calls `glDrawArrays(GL_POINTS, 0, NUM_PARTICLES)` to draw every particle as a point, using the shared position and color data.s
	- Renders the ImGui UI and performance overlay on top.
	
	So, by the time drawing happens, no CPU-side loops are needed to touch individual particles. Everything is done on the GPU.

??? abstract "Explaining the Kernel" 
	<div style="display:none;">
	### Explaining Kernel
	</div>
	
	**Advancing a Particle by DT**

	In the sample code, Joe Parallel wanted to clean up the code by treating x, y, z positions and velocities as single variables instead of handling each component separately. To do this, he created custom types called `point`, `vector`, and `color` using typedef, all backed by OpenCL's `float4` type. (OpenCL doesn't have a `float3`, so `float4` is the next best option, the fourth component goes unused.) He also stored sphere definitions as a `float4`, packing the center coordinates and radius as x, y, z, r.

	```cpp
	typedef float4 point;   // x, y, z, 1.
	typedef float4 vector;  // vx, vy, vz, 0.
	typedef float4 color;   // r, g, b, a
	typedef float4 sphere;  // x, y, z, r

	Spheres[0] = (sphere)(-100.0, -800.0, 0.0, 600.0);
	```

	Joe Parallel also stored the (x,y,z) acceleration of gravity in a `float4`:

	```cpp
	const vector G	= (vector)(0.0, -9.8, 0.0, 0.0);
	```

	Now, given a particle's position `point p` and a particle's velocity `vector v`, here is how you advance it one time step:

	```cpp
	kernel
	void
	Particle( global point *dPobj, global vector *dVel, global color *dCobj )
	{
	int gid = get_global_id( 0 ); // particle number
	
	point	p = dPobj[gid];
	vector	v = dVel[gid];
	color	c = dCobj[gid];
	
	point  pp = p + v*DT + G*(point)(0.5 * DT * DT); 	// p'
	vector vp = v + G*DT; 								// v'
	
	pp.w = 1.0;
	vp.w = 0.0;
	```

	Bouncing is handled by changing the velocity vector according to the outward-facing surface normal of the bumper at the point right before an impact:

	```cpp
	// Test against the first sphere here:
	if(IsInsideSphere(pp, Spheres[0]))
	{
	  vp = BounceSphere(p, v, Spheres[0]);
	  pp = p + vp * DT + G * (point)(0.5 * DT * DT);
	}
	```

	And then do this again for the second bumper object. Assigning the new positions and velocities back into the global buffers happens like this:

	```cpp
	dPobj[gid] = pp;
	dVel[gid]  = vp;
	dCobj[gid] = ????;	// Some change in color based on something 
						// happening in the simulation
	```

	---
	
	**Some utility functions you might find helpful:**

	```cpp
	bool IsInsideSphere(point p, sphere s)
	{
	  float 	r	= fast_length(p.xyz - s.xyz);
	  return	(r < s.w);
	}
	```

	```cpp
	vector BounceSphere(point p, vector v, sphere s)
	{
	  vector	n;
	  n.xyz		= fast_normalize(p.xyz - s.xyz);	
	  n.w		= 0.0;
	  return	Bounce(v, n);
	}
	```

	```cpp
	vector Bounce(vector in, vector n)
	{
	  n.w			= 0.0;
	  n 			= fast_normalize(n);
	  vector out 	= in - n*(vector)(2.0 * dot(in.xyz, n.xyz));	// angle of reflection equals angle of incidence
	  out.w 		= 0.0;
	  return 		out;
	}
	```
	
??? abstract "Getting an Error Message that says something about "UTF-8"?"
	<div style="display:none;">
	### "UTF-8" Error?
	</div>
	
	TODO: Find the actual source for this:
	This is the problem where Windows text editors put 2 marks at the end of a text line instead of the expected one mark.
	Refer to Slide #43 of the Project Notes noteset. 

??? abstract "Determining Platform and Device Information"
	<div style="display:none;">
	### Device Information
	</div>
	
	TODO: Add stuff about VSYNC
	
	The sample code includes code from the printinfo program. This will show what OpenCL capabilities are on your system. The code will also attempt to pick the best OpenCL environment. Feel free to change this if you think it has picked the wrong one. 

---

## Grading

| Requirement | Points |
| :-- | :-- |
| Convincing particle motion | 20 |
| Bouncing from at least two bumpers | 20 |
| Predictable dynamic color changes (random changes are not good enough) | 30 |
| Performance table and graph | 20 |
| Commentary in the PDF file | 30 |
| **Potential Total** | **120** |

!!! success
    The motion, bouncing, and colors of the particles need to be demonstrated via a video link. If it is a Kaltura video, be sure it has been set to **Unlisted**.
