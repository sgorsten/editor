layout(binding = 0) uniform PerObject
{
    mat4 u_model;
    mat4 u_modelIT;
    vec3 u_emissive;
    vec3 u_diffuse;
};

#ifdef VERT_SHADER
layout(location = 0) in vec3 v_position;
layout(location = 1) in vec3 v_normal;
out vec3 position;
out vec3 normal;
void main()
{
	vec4 worldPos = u_model * vec4(v_position, 1);
    gl_Position = u_viewProj * worldPos;
    position = worldPos.xyz;
    normal = normalize((u_modelIT * vec4(v_normal,0)).xyz);
}
#endif

#ifdef FRAG_SHADER
in vec3 position;
in vec3 normal;
void main()
{
    vec3 eyeDir = normalize(u_eye - position);
    vec3 light = u_emissive;
    for(int i=0; i<8; ++i)
    {
        vec3 lightDir = normalize(u_lights[i].position - position);
        light += u_lights[i].color * u_diffuse * max(dot(normal, lightDir), 0);

        vec3 halfDir = normalize(lightDir + eyeDir);
        light += u_lights[i].color * u_diffuse * pow(max(dot(normal, halfDir), 0), 128);
    }
    gl_FragColor = vec4(light,1);
}
#endif
