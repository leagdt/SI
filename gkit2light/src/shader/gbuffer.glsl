#version 330
 
#ifdef VERTEX_SHADER
// doit calculer la position d'un sommet dans le repere projectif
// indice du sommet : gl_VertexID
uniform mat4 mvpMatrix;
uniform mat4 modelMatrix;
layout(location= 0) in vec3 position;
layout(location= 1) in vec2 texcoord;
layout(location= 2) in vec3 normal;

out vec3 normalVertex;
out vec2 texcoordVertex; 

void main( )
{
    gl_Position= mvpMatrix * vec4(position, 1);
    mat3 normalMatrix = transpose(inverse(mat3(modelMatrix)));
    normalVertex = normalize(normalMatrix * normal);
    texcoordVertex = texcoord; 
}
#endif
 
#ifdef FRAGMENT_SHADER
// doit calculer la couleur du fragment

//layout(early_fragment_tests) in; //beug dans les arbres
layout(location= 0) out vec4 fragment_color;        // declare la sortie 0
layout(location= 1) out vec3 fragment_normal;       // declare la sortie 1
layout(location= 2) out float fragment_depth;       // declare la sortie 3
 
in vec3 normalVertex;
in vec2 texcoordVertex; 

uniform sampler2D diffuse_color;    // declare une texture 2d
uniform float znear;
uniform float zfar;


void main( )
{
    if(texture(diffuse_color, texcoordVertex).a < 0.5               )
        discard;
    
    fragment_color = texture(diffuse_color, texcoordVertex);
    fragment_normal = normalize(normalVertex); 

    float linearDepth = (gl_FragCoord.z - znear) / (zfar - znear); // ou calcul plus précis selon ta projection
    fragment_depth = gl_FragCoord.z;
}
#endif
