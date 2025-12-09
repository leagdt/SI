#version 330
 
#ifdef VERTEX_SHADER
// doit calculer la position d'un sommet dans le repere projectif
// indice du sommet : gl_VertexID
uniform mat4 mvpMatrix;
uniform mat4 modelMatrix;
layout(location= 0) in vec3 position;
layout(location= 1) in vec2 texcoord;
layout(location= 2) in vec3 normal;

out vec3 positionVertex; 
out vec3 normalVertex;
out vec2 texcoordVertex; 

void main( )
{
    gl_Position= mvpMatrix * vec4(position, 1);
    positionVertex = vec3(modelMatrix * vec4(position, 1)); 
    normalVertex = vec3(modelMatrix * vec4(normal, 0));
    texcoordVertex = texcoord; 
}
#endif
 
#ifdef FRAGMENT_SHADER
// doit calculer la couleur du fragment

//layout(early_fragment_tests) in; //beug dans les arbres

in vec3 positionVertex; 
in vec3 normalVertex;
in vec2 texcoordVertex; 

uniform sampler2D diffuse_color;    // declare une texture 2d
uniform vec3 lights[256];
uniform uint nbLights; 

void main( )
{
    if(texture(diffuse_color, texcoordVertex).a < 0.5               )
        discard;
    
    vec4 color = texture(diffuse_color, texcoordVertex);
    vec3 finalColor = vec3(0); 
    float result = 0.0; 

    for (uint i = 0u; i<nbLights; i++){
        vec3 l = lights[i] - positionVertex; 
        float dist2 = length(l); // * length(l);
        vec3 n = normalize(normalVertex);
        vec3 l_dir = normalize(l);

        float cos_theta = max(dot(n, l_dir) / dist2, 0.0);

        result += cos_theta; 
    }

    finalColor = color.rgb *result ; 
    gl_FragColor= vec4(finalColor, color.a);
}
#endif
