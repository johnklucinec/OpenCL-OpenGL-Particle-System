#version 410 core

// Input from vertex shader
in vec3 vColor;

// Output fragment color
out vec4 fFragColor;

void main()
{
    fFragColor = vec4(vColor, 1.0);
}
