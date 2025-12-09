#include <cmath>
#include <vector>
#include <cfloat>
#include <chrono>
#include <random>

#include "vec.h"
#include "mat.h"
#include "color.h"
#include "image.h"
#include "image_io.h"
#include "image_hdr.h"
#include "orbiter.h"
#include "mesh.h"
#include "wavefront.h"

struct Ray
{
    Point o;                // origine
    Vector d;               // direction
    float tmax;             // position de l'extremite, si elle existe. le rayon est un intervalle [0 tmax]
    
    // le rayon est un segment, on connait origine et extremite, et tmax= 1
    Ray( const Point& origine, const Point& extremite ) : o(origine), d(Vector(origine, extremite)), tmax(1) {}
    
    // le rayon est une demi droite, on connait origine et direction, et tmax= \inf
    Ray( const Point& origine, const Vector& direction ) : o(origine), d(direction), tmax(FLT_MAX) {}
    
    // renvoie le point sur le rayon pour t
    Point point( const float t ) const { return o + t * d; }
};


struct Hit
{
    float t;            // p(t)= o + td, position du point d'intersection sur le rayon
    float u, v;         // p(u, v), position du point d'intersection sur le triangle
    int triangle_id;    // indice du triangle dans le mesh
    
    Hit( ) : t(FLT_MAX), u(), v(), triangle_id(-1) {}
    Hit( const float _t, const float _u, const float _v, const int _id ) : t(_t), u(_u), v(_v), triangle_id(_id) {}
    
    // renvoie vrai si intersection
    operator bool ( ) { return (triangle_id != -1); }
};

struct Triangle
{
    Point p;            // sommet a du triangle
    Vector e1, e2;      // aretes ab, ac du triangle
    int id;             // indice du triangle
    
    Triangle( const TriangleData& data, const int _id ) : p(data.a), e1(Vector(data.a, data.b)), e2(Vector(data.a, data.c)), id(_id) {}
    
    /* calcule l'intersection ray/triangle
        cf "fast, minimum storage ray-triangle intersection" 
        
        renvoie faux s'il n'y a pas d'intersection valide (une intersection peut exister mais peut ne pas se trouver dans l'intervalle [0 tmax] du rayon.)
        renvoie vrai + les coordonnees barycentriques (u, v) du point d'intersection + sa position le long du rayon (t).
        convention barycentrique : p(u, v)= (1 - u - v) * a + u * b + v * c
    */
    Hit intersect( const Ray &ray, const float tmax ) const
    {
        Vector pvec= cross(ray.d, e2);
        float det= dot(e1, pvec);
        
        float inv_det= 1 / det;
        Vector tvec(p, ray.o);
        
        float u= dot(tvec, pvec) * inv_det;
        if(u < 0 || u > 1) return Hit();        // pas d'intersection
        
        Vector qvec= cross(tvec, e1);
        float v= dot(ray.d, qvec) * inv_det;
        if(v < 0 || u + v > 1) return Hit();    // pas d'intersection
        
        float t= dot(e2, qvec) * inv_det;
        if(t > tmax || t < 0) return Hit();     // pas d'intersection
        
        return Hit(t, u, v, id);                // p(u, v)= (1 - u - v) * a + u * b + v * c
    }
    float area() const
    {
        return 0.5f * length(cross(e1, e2));
    }

    Point getA() const{
        return p; 
    }
    Point getB() const{
        return p + e1; 
    }
    Point getC() const{
        return p + e2; 
    }

};

// renvoie la normale au point d'intersection
Vector normal( const Mesh& mesh, const Hit& hit )
{
    // recuperer le triangle du mesh
    const TriangleData& data= mesh.triangle(hit.triangle_id);
    
    // interpoler la normale avec les coordonn�es barycentriques du point d'intersection
    float w= 1 - hit.u - hit.v;
    Vector n= w * Vector(data.na) + hit.u * Vector(data.nb) + hit.v * Vector(data.nc);
    return normalize(n);
}

Color computeColor (const Vector& n, const Point& p, const std::vector<Triangle>& triangles, const int& N)
{
    //return Color(std::abs(n.x), std::abs(n.y), std::abs(n.z));

    std::random_device hwseed;

    // init d'un générateur de nombre de aleatoire std
    std::default_random_engine rng( hwseed() );
    std::uniform_real_distribution<float> uniform(0, 1);

    Color color(0, 0, 0, 1.0);
    for(int i= 0; i < N; i++)
    {
        // genere u1 et u2
        float u1= uniform( rng );
        float u2= uniform( rng );
    
        // construit la direction l et évalue sa pdf
        float cos = u1; 
        float sin = sqrt(1.0 - cos*cos); 
        float phi = 2.0 * M_PI * u2; 
        Vector l= Vector(std::cos(phi) * sin, cos, sin * std::sin(phi));
        float pdf= 1.0/(2.0 * M_PI);

        // evalue la fonction

        Ray lr (p, normalize(l)); 
        float V= 1; // 0 ou 1, selon les intersections
        float tmax= lr.tmax;   // extremite du rayon

        Color emission= Color(1);

        for(int i= 0; i < int(triangles.size()); i++)
        {
            if(Hit h= triangles[i].intersect(lr, tmax))
            {
                assert(h.t > 0);
                tmax = h.t; 
            }
        }
        
        float cos_theta= dot(normalize(n), normalize(l));
    
        // moyenne
        color = color + emission * V * cos_theta / pdf;
    }
    return Color(color.r / float(N), color.g / float(N), color.b / float(N), 1.0);     
}


Color computeColor (const Vector& n, const Point& p, const std::vector<Triangle>& triangles, const Mesh& mesh, const int& N, std::default_random_engine& rng)
{
    std::uniform_real_distribution<float> uniform(0, 1);

    Color color(0, 0, 0, 1.0);
    for(int i= 0; i < N; i++)
    {
        // genere u1 et u2
        float u1= uniform( rng );
        float u2= uniform( rng );
    
        // construit la direction l et évalue sa pdf
        float cos = u1; 
        float sin = sqrt(1.0 - cos*cos); 
        float phi = 2.0 * M_PI * u2; 
        Vector l= Vector(std::cos(phi) * sin, cos, sin * std::sin(phi));
        float pdf= 1.0/(2.0 * M_PI);

        // evalue la fonction

        Ray lr (p, normalize(l)); 
        float V= 0; // 0 ou 1, selon les intersections
        float tmax= lr.tmax;   // extremite du rayon

        Color emission= Color(1);

        for(int i= 0; i < int(triangles.size()); i++)
        {
            if(Hit h= triangles[i].intersect(lr, tmax))
            {
                assert(h.t > 0);
                tmax = h.t; 
                const Material& material= mesh.triangle_material(h.triangle_id);    // cf la doc de Mesh
                emission= material.emission;
                V = 1; 
            }
        }
        
        float cos_theta= dot(normalize(n), normalize(l));
    
        // moyenne
        color = color + emission * V * cos_theta / pdf;
    }
    return color;
}

int selectSource(const std::vector<float>& cdf, std::default_random_engine& rng) 
{ 
    std::uniform_real_distribution<float> U(0, 1); 
    float u = U(rng); // recherche linéaire (peut être binaire si tu veux) 
    for(int i = 0; i < (int)cdf.size(); i++) 
        if(u < cdf[i]) 
            return i; 
    
    return cdf.size() - 1; // fallback 
}


Point pdfTriangle(const Triangle& t, float& pdf){ 
    std::random_device hwseed;
    std::default_random_engine rng( hwseed() );
    std::uniform_real_distribution<float> uniform(0, 1);

    float r1= std::sqrt(uniform( rng ));
    float u2= uniform( rng );
    
    // generer les coordonnées barycentriques
    float alpha= 1 - r1;
    float beta= (1 - u2) * r1;
    float gamma= u2 * r1;
    
    // construire le point
    Point q= alpha*t.getA() + beta*t.getB() + gamma*t.getC();
    
    // evaluer sa densite de proba
    pdf= 1 / t.area();
    
    return q; 
}


Color computeColorMonteCarlo (const Vector& n, const Point& p, const std::vector<Triangle>& triangles, const Mesh& mesh, const int& N, 
                            const std::vector<Triangle>& sourceLumineuse, std::default_random_engine& rng, const std::vector<float>& cdf)
{

    Color color(0, 0, 0, 1.0);
    for(int i= 0; i < N; i++)
    {
        int idSource = selectSource(cdf, rng); 
        Triangle s = sourceLumineuse[idSource]; 

        float pdf = 0; 
        Point q = pdfTriangle(s, pdf); 

        Vector l = normalize(q - p);
        Ray lr(p, l);
        float V= 0; // 0 ou 1, selon les intersections
        float tmax= lr.tmax;   // extremite du rayon

        Color emission= Color(1);

        for(int i= 0; i < int(triangles.size()); i++)
        {
            if(Hit h= triangles[i].intersect(lr, tmax))
            {
                assert(h.t > 0);
                tmax = h.t; 
                const Material& material= mesh.triangle_material(h.triangle_id);    // cf la doc de Mesh
                emission= material.emission;
                V = 1; 
            }
        }
        Vector n_s = normalize(cross(s.e1, s.e2));
        float cosThetaP = std::max(0.0f, dot(n, l));
        float cosThetaQ = std::max(0.0f, dot(n_s, -l));

        color = color + emission * V * cosThetaP * cosThetaQ / (length2(q-p) * pdf);

    }


    return color; 
}

int main( const int argc, const char **argv )
{

    std::random_device hwseed;

    // init d'un générateur de nombre de aleatoire std
    std::default_random_engine rng( hwseed() );

    int NbRayons = 16; 
    if(argc > 1){
        NbRayons = std::stoi(argv[1]);
    }
    std::cerr<<"Nombre de rayons choisi : "<<NbRayons<<std::endl;   

    const char *mesh_filename= "data/cornell.obj";
    const char *orbiter_filename= "data/cornell_orbiter.txt";
    
    Orbiter camera;
    if(camera.read_orbiter(orbiter_filename) < 0)
        return 1;

    Mesh mesh= read_mesh(mesh_filename);
    
    int n= mesh.triangle_count();

    // recupere les triangles dans le mesh
    std::vector<Triangle> triangles;
    {
        for(int i= 0; i < n; i++)
            triangles.emplace_back(mesh.triangle(i), i);
    }

    std::vector<Triangle> sourceLumineuse;
    {
        for(int i =0; i<n; i++){
            const Material& material=mesh.triangle_material(i);
            if(material.emission.r != 0 || material.emission.g != 0 || material.emission.b != 0){
                sourceLumineuse.push_back(triangles[i]); 
            }
        }
    }

    // fonction de répartition cumulative
    std::vector<float> cdf;  
    {
        cdf.reserve(sourceLumineuse.size());

        float totalArea = 0.0f;
        for(const Triangle& s : sourceLumineuse)
            totalArea += s.area();

        float cumul = 0.0f;
        for(const Triangle& s : sourceLumineuse)
        {
            cumul += s.area() / totalArea;
            cdf.push_back(cumul);
        }
    }


    //
    Image image(1024, 768);

    // recupere les transformations pour generer les rayons
    camera.projection(image.width(), image.height(), 45);
    Transform model= Identity();
    Transform view= camera.view();
    Transform projection= camera.projection();
    Transform viewport= camera.viewport();
    Transform inv= Inverse(viewport * projection * view * model);
    
auto start= std::chrono::high_resolution_clock::now();
    
    // parcours tous les pixels de l'image
    for(int y= 0; y < image.height(); y++)
    for(int x= 0; x < image.width(); x++)
    {
        // generer le rayon au centre du pixel
        Point origine= inv(Point(x + float(0.5), y + float(0.5), 0));
        Point extremite= inv(Point(x + float(0.5), y + float(0.5), 1));
        Ray ray(origine, extremite);
        
        // calculer les intersections avec tous les triangles
        Hit hit;                // proprietes de l'intersection
        float tmax= ray.tmax;   // extremite du rayon
        for(int i= 0; i < int(triangles.size()); i++)
        {
            if(Hit h= triangles[i].intersect(ray, tmax))
            {
                // ne conserve que l'intersection *valide* la plus proche de l'origine du rayon
                assert(h.t > 0);
                hit= h;
                tmax= h.t;
            }
        }
        
        if(hit)
        {
            Vector n= normal(mesh, hit);
            Point p= ray.o + hit.t * ray.d + 0.0001 * n; 
            int N = 256; 
            
            Color color; 
            if(argc > 2)
            {
                std::string argument1 = argv[2]; // récupération de l'argument
                if(argument1 == "1") {
                    color = computeColor (n, p, triangles, mesh, NbRayons, rng);
                } else{
                    color = computeColorMonteCarlo(n, p, triangles, mesh, NbRayons, sourceLumineuse, rng, cdf); 
                }
            } else {
                color = computeColorMonteCarlo(n, p, triangles, mesh, NbRayons, sourceLumineuse, rng, cdf); 
            }
            
            const Material& material= mesh.triangle_material(hit.triangle_id);
            Color diffuse = material.diffuse; 
            color = diffuse * color/float(NbRayons) + material.emission; 
            color.a = 1.0; 
            // image(x, y)= Color(color.r / float(N), color.g / float(N), color.b / float(N), 1.0); 
            image(x, y)= color;
        }
    }

auto stop= std::chrono::high_resolution_clock::now();
    int cpu= std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count();
    printf("%dms\n", cpu);
    

    if(argc > 2)
    {
        std::string argument1 = argv[2]; // récupération de l'argument
        if(argument1 == "1") {
            std::cerr<<"Monte Carlo simple choisi "<<std::endl;   

            write_image(image, "rendu_monteCarlo_simple.png");
            write_image_hdr(image, "rendu_monteCarlo_simple.hdr");   
        } else{
            std::cerr<<"Monte Carlo orienté vers les sources de lumières choisi "<<std::endl;  
            write_image(image, "rendu_monteCarlo_efficace.png");
            write_image_hdr(image, "rendu_monteCarlo_efficace.hdr");
        }
    } else {
        std::cerr<<"Monte Carlo orienté vers les sources de lumières choisi "<<std::endl;
        write_image(image, "rendu_monteCarlo_efficace.png");
        write_image_hdr(image, "rendu_monteCarlo_efficace.hdr");
    }
    
    return 0;
}
