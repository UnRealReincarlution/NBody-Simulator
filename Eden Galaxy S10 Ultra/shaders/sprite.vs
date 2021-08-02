#version 330 core
layout (location = 0) in vec4 vertex; // <vec2 position, vec2 texCoords>

out vec2 TexCoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    TexCoords = vertex.zw;

    mat3 bm = transpose(mat3(view));

    mat4 blackmagic = mat4(bm);
    blackmagic[3] = model[3];

    mat4 transform = mat4(mat3(model));

    gl_Position = projection * view * blackmagic * transform * vec4(vertex.xy, 0.0, 1.0);
}