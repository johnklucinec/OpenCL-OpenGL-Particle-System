typedef float4 point;		// x, y, z, 1.0
typedef float4 vector;	// vx, vy, vz, 0.0
typedef float4 color;		// r, g, b, a
typedef float4 sphere;	// x, y, z, r


// ==================================================
// Helper Functions:
// ==================================================

vector Bounce(vector in, vector n)
{
	n.w					= 0.0;
	n 					= fast_normalize(n);
	vector out 	= in - n*(vector)(2.0 * dot(in.xyz, n.xyz));	// θ of reflection == θ of incidence
	out.w 			= 0.0;
	return 			out;
}

vector BounceSphere(point p, vector v, sphere s)
{
  vector	n;
  n.xyz		= fast_normalize(p.xyz - s.xyz);
  n.w			= 0.0;
  return	Bounce(v, n);
}

bool IsInsideSphere(point p, sphere s)
{
  float 	r	= fast_length(p.xyz - s.xyz);
  return	(r < s.w);
}


// ==================================================
// Kernel Code:
// ==================================================
kernel
void
Particle(global point* dPobj, global vector* dVel, global color* dCobj, float dt)
{
 // Local memory for spheres.
  __local sphere Spheres[2];

  const vector G	= (vector)(0.0, -9.8, 0.0, 0.0);
  const float  DT	= dt;

  // Have one work-item initialize shared data.
  if (get_local_id(0) == 0)
  {
    Spheres[0] = (sphere)(-100.0, -800.0, 0.0, 600.0);
    /* second sphere */
  }

  // Ensure all work-items see the loaded data.
  // Now workgroup size matters, so make sure you set it correctly.
  barrier(CLK_LOCAL_MEM_FENCE);

  int	gid	= get_global_id(0);	// particle number

  // extract the position and velocity for this particle:
  point		p = dPobj[gid];
  vector	v = dVel[gid];

  // Remember that you also need to extract this particle's color
  // and change it in some way that is obviously correct.


  // Advance the particle:
  point  pp = p + v * DT + G * (point)(0.5 * DT * DT);
  vector vp = v + G * DT;

  pp.w = 1.0;
  vp.w = 0.0;

  // Test against the first sphere here:
  if(IsInsideSphere(pp, Spheres[0]))
  {
    vp = BounceSphere(p, v, Spheres[0]);
    pp = p + vp * DT + G * (point)(0.5 * DT * DT);
  }

  // Test against the second sphere here:


  dPobj[gid] = pp;
  dVel[gid]  = vp;
}
