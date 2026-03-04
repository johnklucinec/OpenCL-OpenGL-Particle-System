#version 410 core

// Per-vertex attributes from the application
in vec3 aVertex;
in vec3 aColor;

// Uniforms from the application
uniform mat4 uModelMatrix;
uniform mat4 uViewMatrix;
uniform mat4 uProjMatrix;

// Output to fragment shader
out vec3 vColor;

void main()
{
    vColor = aColor;
    gl_Position = uProjMatrix * uViewMatrix * uModelMatrix * vec4(aVertex, 1.0);
}
