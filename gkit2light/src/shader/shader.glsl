#version 330
 
#ifdef VERTEX_SHADER
// doit calculer la position d'un sommet dans le repere projectif
// indice du sommet : gl_VertexID
uniform mat4 mvpMatrix;
layout(location= 0) in vec3 position;
layout(location= 2) in vec3 normal;

out vec3 normalVertex; 

void main( )
{
    gl_Position= mvpMatrix * vec4(position, 1);                   // a completer
    normalVertex = normal;
}
#endif
 
#ifdef FRAGMENT_SHADER
// doit calculer la couleur du fragment

in vec3 normalVertex;

void main( )
{
    vec3 l = vec3(0,0,1); 
    float cos_theta= dot(normalize(normalVertex), normalize(l));

    int nbPalier = 20; 

    float color = floor(cos_theta * nbPalier) / (nbPalier-1); 
    gl_FragColor= vec4(vec3(color,0,0), 1);       // blanc opaque
}
#endif
