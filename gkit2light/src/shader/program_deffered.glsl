#version 330 core

#ifdef VERTEX_SHADER
layout(location= 0) in vec3 position;
layout(location= 1) in vec2 texcoord;

out vec2 TexCoords;

void main()
{
    TexCoords = texcoord;
    gl_Position = vec4(position.xy, 0.0, 1.0); // positions NDC
}
#endif

#ifdef FRAGMENT_SHADER
in vec2 TexCoords;

// G-buffer textures
uniform sampler2D gNormal;
uniform sampler2D gAlbedo;
uniform sampler2D gDepth;

// paramètres
uniform mat4 invProjView;

uniform vec3 lights[500];
uniform uint nbLights; 

vec3 reconstructWorldPos(vec2 uv)
{
    float depth = texelFetch(gDepth, ivec2(uv), 0).r;
    float z = depth ;
    vec4 clip = vec4(uv, z, 1.0);

    vec4 world = invProjView * clip;
    world /= world.w;
    return world.xyz;
}

void main()
{

    if(texture(gAlbedo, TexCoords).a < 0.5               )
        discard;

    float depth = texture(gDepth, TexCoords).r;
    vec3 normal = texture(gNormal, TexCoords).rgb;
    vec4 albedo = texture(gAlbedo, TexCoords).rgba;

    vec3 pos = reconstructWorldPos(gl_FragCoord.xy);

    float result = 0.4; 
    for (uint i = 0u; i<nbLights; i++){
        vec3 l = lights[i] - pos; 
        float dist2 = length(l) * length(l) * 0.1;

        vec3 l_dir = normalize(l);

        float cos_theta = max(dot(normal, l_dir) / dist2, 0.0);
    
        result += cos_theta ; 
    }
    vec3 color = albedo.rgb * result;

    gl_FragColor = vec4(color, albedo.a);
}
#endif