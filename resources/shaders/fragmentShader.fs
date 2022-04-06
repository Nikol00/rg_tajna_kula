#version 330 core

out vec4 FragColor;

in vec2 outTexCords;

uniform sampler2D t0;

void main(){
    FragColor = texture(t0, outTexCords);
}