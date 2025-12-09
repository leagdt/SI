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

layout(early_fragment_tests) in; //beug dans les arbres

in vec3 positionVertex; 
in vec3 normalVertex;
in vec2 texcoordVertex; 

uniform sampler2D diffuse_color;    // declare une texture 2d


void main( )
{

    vec4 color = texture(diffuse_color, texcoordVertex);
    gl_FragColor= color;
}
#endif
