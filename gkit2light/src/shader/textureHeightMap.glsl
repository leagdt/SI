#version 330
 
#ifdef VERTEX_SHADER
// doit calculer la position d'un sommet dans le repere projectif
// indice du sommet : gl_VertexID
uniform mat4 mvpMatrix;
layout(location= 0) in vec3 position;

out float height;

void main( )
{
    height = position.y;
    gl_Position= mvpMatrix * vec4(position, 1);
}
#endif
 
#ifdef FRAGMENT_SHADER
// doit calculer la couleur du fragment

in float height;
out vec4 fragColor; 

void main() {
    fragColor = vec4(height, 0, 0, 1);
}
#endif
