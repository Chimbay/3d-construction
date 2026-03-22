#version 410

uniform mat4 mv_matrix;
uniform mat4 p_matrix;

const vec3 Pos[4] = vec3[4](
        vec3(-10.0, 0.0, -10.0),
        vec3(10.0, 0.0, -10.0),
        vec3(10.0, 0.0, 10.0),
        vec3(-10.0, 0.0, 10.0)
    );

const int Indices[6] = int[6](0, 2, 1, 2, 0, 3);

void main() {
    int Index = Indices[gl_VertexID];
    vec4 vPos = vec4(Pos[Index], 1.0);

    gl_Position = p_matrix * mv_matrix * vPos;
}
