#version 330
 
#ifdef VERTEX_SHADER
// doit calculer la position d'un sommet dans le repere projectif
// indice du sommet : gl_VertexID
uniform mat4 mvpMatrix;
layout(location= 0) in vec3 position;
layout(location= 2) in vec3 normal;
layout(location= 4) in uint material;

out vec3 normalVertex;
flat out uint materialVertex; // On interpole pas 
void main( )
{

    // int N = 20;                // taille de la grille (10x10)
    // float spacing = 4.0;       // distance entre objets

    // int row = gl_InstanceID / N;
    // int col = gl_InstanceID % N;

    // vec3 offset = vec3(
    //     -40.0 + float(col) * spacing, 
    //     0.0, 
    //     -40.0 + float(row) * spacing
    // );

    vec3 offset = vec3(0,0,0); 

    vec3 p= position + offset; 
    gl_Position= mvpMatrix * vec4(p, 1);
    normalVertex = normal;
    materialVertex = material; 
}
#endif
 
#ifdef FRAGMENT_SHADER
// doit calculer la couleur du fragment

in vec3 normalVertex;
flat in uint materialVertex; // Pas interpolé
uniform vec4 colors[16];

void main( )
{
    vec3 l = vec3(0,0,1); 
    float cos_theta= dot(normalize(normalVertex), normalize(l));
    vec3 color = colors[materialVertex].xyz; 

    gl_FragColor= vec4(vec3(color * cos_theta), 1);
}
#endif
