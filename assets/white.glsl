layout(binding = 0) uniform PerObject
{
    mat4 u_model;
};

#ifdef VERT_SHADER
layout(location = 0) in vec3 v_position;
void main()
{
    gl_Position = u_viewProj * u_model * vec4(v_position, 1);
}
#endif

#ifdef FRAG_SHADER
void main()
{
    gl_FragColor = vec4(1,1,1,1);
}
#endif
