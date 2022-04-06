#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCords;

out vec2 outTexCords;

void main(){
    gl_Position =  vec4(aPos.x, aPos.y, aPos.y, 1.0);
    outTexCords = aTexCords;
}