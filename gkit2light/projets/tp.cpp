
//! \file tp.cpp code de depart minimaliste


#include "wavefront.h"
#include "texture.h"

#include "draw.h"        
#include "app.h"        // classe Application a deriver
#include "app_camera.h"
#include "app_time.h"
#include "program.h"
#include "uniforms.h"


// utilitaire. creation d'une grille / repere. permet de visualiser l'espace autour des objets.
Mesh make_grid( const int n= 10)
{
    Mesh grid= Mesh(GL_LINES);
    
    // grille
    grid.color(White());
    for(int x= 0; x < n; x++)
    {
        float px= float(x) - float(n)/2 + .5f;
        grid.vertex(Point(px, 0, - float(n)/2 + .5f)); 
        grid.vertex(Point(px, 0, float(n)/2 - .5f));
    }

    for(int z= 0; z < n; z++)
    {
        float pz= float(z) - float(n)/2 + .5f;
        grid.vertex(Point(- float(n)/2 + .5f, 0, pz)); 
        grid.vertex(Point(float(n)/2 - .5f, 0, pz)); 
    }
    
    // axes XYZ
    grid.color(Red());
    grid.vertex(Point(0, .1, 0));
    grid.vertex(Point(1, .1, 0));
    
    grid.color(Green());
    grid.vertex(Point(0, .1, 0));
    grid.vertex(Point(0, 1, 0));
    
    grid.color(Blue());
    grid.vertex(Point(0, .1, 0));
    grid.vertex(Point(0, .1, 1));
    
    glLineWidth(2);
    
    return grid;
}


class TP : public AppTime
{
public:
    // constructeur : donner les dimensions de l'image, et eventuellement la version d'openGL.
    TP( ) : AppTime(1024, 640) {}
    
    // creation des objets de l'application
    int init( )
    {
        ////////////////// Chargement des objets 3d ////////////////////////
        m_objetCube= read_mesh("data/cube.obj");
        m_objetRobot= read_mesh("data/robot.obj");
        m_objetBiggy= read_mesh("data/result.obj");

        if(m_objetRobot.materials().count() == 0)
            return -1;     // pas de matieres, pas d'affichage

        // decrire un repere / grille 
        m_repere= make_grid(10);

        
        ////////////Création des buffers + configuration des vao //////////
    
        // trie les triangles par matiere et recupere les groupes de triangles utilisant la meme matiere.
        m_groupsRobot= m_objetRobot.groups();
        vaoRobot= m_objetRobot.create_buffers( /* texcoods */ false, /* normals */ true, /* color */ false, /* material index */ true );
        
        vaoBiggy= m_objetBiggy.create_buffers( /* texcoods */ false, /* normals */ true, /* color */ false, /* material index */ false );

        //////////////////// Chargement des textures //////////////////////
        
        texture = read_texture(0, "data/pacman.png");

        ///////////// Chargement et compilation des shaders ///////////////

        program= read_program("src/shader/shader.glsl");
        program_print_errors(program);

        programColor= read_program("src/shader/color.glsl");
        program_print_errors(programColor);

        programTexture= read_program("src/shader/texture.glsl");
        program_print_errors(programTexture);
        
        
        /////////////////// etat openGL par defaut ////////////////////////


        // projection par defaut, adaptee a la fenetre
        m_camera.projection(window_width(), window_height(), 45);
        m_camera.lookat(Point(-10, 0, -10), Point(10, 10, 10));

        glClearColor(0.2f, 0.2f, 0.2f, 1.f);        // couleur par defaut de la fenetre
        
        glClearDepth(1.f);                          // profondeur par defaut
        glDepthFunc(GL_LESS);                       // ztest, conserver l'intersection la plus proche de la camera
        glEnable(GL_DEPTH_TEST);                    // activer le ztest

        return 0;   // pas d'erreur, sinon renvoyer -1
    }
    
    // destruction des objets de l'application
    int quit( )
    {
        m_objetCube.release();
        m_objetRobot.release();
        m_objetBiggy.release();
        m_repere.release();
        return 0;   // pas d'erreur
    }

    void updateCamera(){
        // recupere les mouvements de la souris
        int mx, my;
        unsigned int mb= SDL_GetRelativeMouseState(&mx, &my);
        int mousex, mousey;
        SDL_GetMouseState(&mousex, &mousey);
        
        // deplace la camera
        if(mb & SDL_BUTTON(1))
            m_camera.rotation(mx, my);      // tourne autour de l'objet
        else if(mb & SDL_BUTTON(3))
            m_camera.translation((float) mx / (float) window_width(), (float) my / (float) window_height()); // deplace le point de rotation
        else if(mb & SDL_BUTTON(2))
            m_camera.move(mx);           // approche / eloigne l'objet
        
        SDL_MouseWheelEvent wheel= wheel_event();
        if(wheel.y != 0)
        {
            clear_wheel_event();
            m_camera.move(8.f * wheel.y);  // approche / eloigne l'objet
        }
    
        const char *orbiter_filename= "app_orbiter.txt";
        // copy / export / write orbiter
        if(key_state('c'))
        {
            clear_key_state('c');
            m_camera.write_orbiter(orbiter_filename);
            
        }
        // paste / read orbiter
        if(key_state('v'))
        {
            clear_key_state('v');
            
            Orbiter tmp;
            if(tmp.read_orbiter(orbiter_filename) < 0)
                // ne pas modifer la camera en cas d'erreur de lecture...
                tmp= m_camera;
            
            m_camera= tmp;
        }
        
        // screenshot
        if(key_state('s'))
        {
            static int calls= 1;
            clear_key_state('s');
            screenshot("app", calls++);
        }
    }
    
    // dessiner une nouvelle image
    int render( )
    {
        updateCamera(); 


        // transformations standards
        Transform view= m_camera.view();
        Transform projection= m_camera.projection();
        Transform model= Identity();
         
        // composition des 3 changements de repères
        Transform mvp= projection * view * model;   // P(V(M*p))
        
        glViewport(0, 0, window_width(), window_height());
        
        // couleur et profondeur par defaut
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        
    // dessine le repere, place au centre du monde
        draw(m_repere, /* model */ Identity(), m_camera);

    
    reload_shader(); 
        
        
    // Bigguy

        glBindVertexArray( vaoBiggy );

        Point pmin, pmax;
        m_objetRobot.bounds(pmin, pmax);
        float target_size = pmax.y-pmin.y; 

        Point pminBiggy, pmaxBiggy;
        m_objetBiggy.bounds(pminBiggy, pmaxBiggy);
        float sizeBiggy = pmaxBiggy.y - pminBiggy.y; 

        model = Scale(0.1) * Translation(0, sizeBiggy * 0.7, 0); 

        mvp= projection * view * model;

        glUseProgram(programColor);
        
        program_uniform(programColor, "mvpMatrix", mvp);
        

        // GLint location= glGetUniformLocation(program, "diffuse_color");
        // glUniform1i(location, 0);   // une seule texture utilisee, 0 dans ce cas
        
        // selectionner la texture :
        // glBindTexture(GL_TEXTURE_2D, texture);

        // dessiner les triangles du groupe
        glDrawArrays(GL_TRIANGLES, 0, m_objetBiggy.triangle_count() * 3);



    // Cube
        // mvp= projection * view * Identity();

        // glUseProgram(program);
        // program_uniform(program, "mvpMatrix", mvp);
        // m_objetCube.draw(program, /* use position */ true, /* use texcoord */ false, /* use normal */ true, /* use color */ false, 
        //     /* use material index */ false);            

        // int t = 40;
        // for(int i = -t;  i< t; i+=4){
        //     for(int j = -t; j<t; j+=4){
        //         model = Translation(i, 0, j); 
        //         mvp= projection * view * model;
        //         program_uniform(program, "mvpMatrix", mvp);
        //         m_objetCube.draw(program, /* use position */ true, /* use texcoord */ false, /* use normal */ true, /* use color */ false, 
        //             /* use material index */ false);  
        //     }
        // }

    // Robot 

        // glBindVertexArray( vaoRobot );
        // const Materials& materials= m_objetRobot.materials();
        // std::vector<Color> colors(16, Color(1, 0, 1));
       
        // // dessine chaque groupe de triangles, avec sa matiere
        // for(unsigned i= 0; i < m_groupsRobot.size(); i++)
        // {
        //     const TriangleGroup& group= m_groupsRobot[i];

        //     colors[i] =  materials.material(group.index).diffuse;
        // }

        // model = Identity();//Translation(3, 0, 0); 

        // mvp= projection * view * model;

        // parametrer le shader pour dessiner avec la couleur
        // glUseProgram(programColor);
        
        // program_uniform(programColor, "mvpMatrix", mvp);
        // program_uniform(programColor, "colors", colors);

        // dessiner les triangles du groupe
        //glDrawArrays(GL_TRIANGLES, 0, m_objetRobot.triangle_count() * 3);

        // for(int i = -t;  i< t; i+=4){
        //     for(int j = -t; j<t; j+=4){
        //         model = Translation(i, 0, j); 
        //         mvp= projection * view * model;
        //         program_uniform(programColor, "mvpMatrix", mvp);
        //         glDrawArrays(GL_TRIANGLES, 0, m_objetRobot.triangle_count() * 3);  
        //     }
        // }

        // glDrawArraysInstanced(GL_TRIANGLES, 0, m_objetRobot.triangle_count() * 3, 400);  
        
        
        // continuer, afficher une nouvelle image
        // tant que la fenetre est ouverte...
        return 1;
    }

protected:
    Mesh m_objetCube;
    Mesh m_objetRobot;
    Mesh m_objetBiggy;
    Mesh m_repere;
    std::vector<TriangleGroup> m_groupsRobot;
    GLuint program= 0;
    GLuint programColor= 0;
    GLuint vaoRobot= 0;
    GLuint programTexture= 0;
    GLuint vaoBiggy= 0;
    GLuint texture = 0; 

    Orbiter m_camera;

private: 
    void reload_shader(){
        if(key_state('r'))
        {
            clear_key_state('r');
            reload_program(program, "src/shader/shader.glsl");
            program_print_errors(program);
        }
    }
};


int main( int argc, char **argv )
{
    // il ne reste plus qu'a creer un objet application et la lancer 
    TP tp;
    tp.run();
    
    return 0;
}


// texture tuto 9 utilisation de buffer 
// numéroté les textures 