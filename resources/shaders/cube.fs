#version 330 core
out vec4 FragColor;

in vec3 TexCoords;

uniform samplerCube cube;

void main()
{
    FragColor = texture(cube, TexCoords);
}