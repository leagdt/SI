
//! \file tp.cpp code de depart minimaliste


#include "wavefront.h"
#include "texture.h"

#include "draw.h"        
#include "app.h"        // classe Application a deriver
#include "app_camera.h"
#include "app_time.h"
#include "program.h"
#include "uniforms.h"
#include "mat.h"
#include "image.h"
#include <cstdio>
#include <vector>
#include <ctime>

class Camera{
public: 
    Camera() {
        m_position = Identity(); 
    }
    Transform projection (const uint width, const uint height, const float fov, const float znear, const float zfar){
        m_width= width;
        m_height= height;
        m_fov= fov;
        m_znear = znear; 
        m_zfar = zfar; 
        return Perspective(fov, float(width) / float(height), znear, zfar);
    }
    Transform projection (){
        return Perspective(m_fov, float(m_width) / float(m_height), m_znear, m_zfar);
    }
    Transform view(){
        return Inverse( m_position * m_rotation);
    }
    Transform projectionOrtho(const Point pmin, const Point pmax){
        return Ortho(pmin.x, pmax.x, pmin.z, pmax.z, m_znear, m_zfar);
    }
    Transform lookat(const Point from, const Point to, const Vector up){
        return Lookat(from, to, up);
    }
    void rotationX (const float x){
        m_position = m_position * RotationX(x);
    }
    void rotationY (const float y){
        m_position = m_position * RotationY(y);
    }
    void rotationZ (const float z){
        m_position = m_position * RotationZ(z);
    }
    void translation (const float x, const float y, const float z){
        m_position = m_position * Translation(x,y,z); 
    }

    void rotationCamX (float x){
        m_rotation = m_rotation * RotationX(x);
    }
    float znear(){ return m_znear;}
    float zfar(){ return m_zfar;}

    Point getPosition() const {
        vec4 col = m_position.column(3); 
        return Point(col.x, col.y, col.z);
    }

protected:
    Transform m_position; 
    Transform m_rotation; 
    //vec2 m_position; 
    //vec2 m_rotation; 
    float m_znear = 0.0; 
    float m_zfar = 0.0; 
    float m_fov = 0.0; 
    uint m_width = 0.0;
    uint m_height = 0.0; 
};

class HeightField{
    public:
    HeightField(){}
    HeightField (const Point pmin, const Point pmax) : m_pmin(pmin), m_pmax(pmax){
        m_width = uint(pmax.x - pmin.x);
        m_height = uint(pmax.z- pmin.z);

        m_data = std::vector<float> (m_width * m_height);
    }

    uint width()const{ return m_width; }
    uint height()const{ return m_height; }
    Point pmin() const { return m_pmin; }
    Point pmax() const { return m_pmax; }

    float value(const uint x, const uint y){
        float index = y * m_width + x; 
        return m_data[index];
    }

    float valueXZ(const float x, const float z){
        // Normalisation dans [0,1]
        float u = (x - m_pmin.x) / (m_pmax.x - m_pmin.x);
        float v = (z - m_pmin.z) / (m_pmax.z - m_pmin.z);

        // Passage en indices pixel
        int i = int(u * (m_width - 1));
        int j = int((1.0 - v) * (m_height - 1));

        // Clamp pour rester dans les bornes
        i = std::max(0, std::min((int)m_width - 1, i));
        j = std::max(0, std::min((int)m_height - 1, j));
        return value(i, j);
    }

    void setValue(const uint index, const float v){
        m_data[index] = v; 
    }

    float* data() {
        return m_data.data();
    }

    const float* data() const {
        return m_data.data();
    }

    void writeImage(const char* nameFile){
        Image image(m_width, m_height);

        for(uint i=0;i<m_width * m_height;i++){
            float valNormalise = (m_data[i] - m_pmin.y) / (m_pmax.y - m_pmin.y) ;
            image(i) = Color(valNormalise, valNormalise, valNormalise, 1.0f);
        }

        write_image(image, nameFile);
    }

    protected:
    uint m_width; 
    uint m_height; 
    Point m_pmin; 
    Point m_pmax; 
    std::vector<float> m_data; 
}; 

class TP : public AppTime
{
public:
    // constructeur : donner les dimensions de l'image, et eventuellement la version d'openGL.
    TP( ) : AppTime(1024, 640) {}
    
    // creation des objets de l'application
    int init( )
    {
        ////////////////// Chargement des objets 3d ////////////////////////
        m_scene= read_mesh("data/rungholt/rungholt.obj");
       
        if(m_scene.materials().count() == 0)
            return -1;     // pas de matieres, pas d'affichage

        
        ////////////Création des buffers + configuration des vao //////////
    
        // trie les triangles par matiere et recupere les groupes de triangles utilisant la meme matiere.
        m_groupsScene= m_scene.groups();
        Point pmin, pmax;
        m_scene.bounds(pmin, pmax);
        gridCoords = Vector(6,6,6); 
        triangleGrid = createGrid(m_scene, pmin, pmax); 

        vaoScene= m_scene.create_buffers( /* texcoods */ true, /* normals */ true, /* color */ false, /* material index */ true );
        
        heightMap = HeightField(pmin, pmax); 

        glGenTextures(1, &textureHeightMap);
        glBindTexture(GL_TEXTURE_2D, textureHeightMap);
        glTexImage2D(GL_TEXTURE_2D, 
            /* level */ 0,
            /* texel format */  GL_R32F, 
            /* width, height, border */ heightMap.width(), heightMap.height(), 0,
            /* data format */ GL_RED, /* data type */ GL_FLOAT,
            /* data */ nullptr);
         
        // nombre de niveaux de la texture : 1 seul, cf level 0
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);


        glGenTextures(1, &zbufferHeighMap);
        glBindTexture(GL_TEXTURE_2D, zbufferHeighMap);
        
        glTexImage2D(GL_TEXTURE_2D, 0,
            GL_DEPTH_COMPONENT, heightMap.width(), heightMap.height(), 0,
            GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, nullptr);



        glGenFramebuffers(1, &fboHeightMap);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fboHeightMap);
        
        // associer la texture à une sortie du framebuffer
        glFramebufferTexture(GL_DRAW_FRAMEBUFFER, /* attachment */ GL_COLOR_ATTACHMENT0, /* texture */ textureHeightMap, /* mipmap level */ 0);
        glFramebufferTexture(GL_DRAW_FRAMEBUFFER,  /* attachment */ GL_DEPTH_ATTACHMENT, /* texture */ zbufferHeighMap, /* mipmap level */ 0);
        if(glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
                printf("FBO ERROR\n");


        GLenum buffersHeightMap[]= { GL_COLOR_ATTACHMENT0 };
        glDrawBuffers(1, buffersHeightMap);


        /////////////// Préparation du GBuffer /////////////////

        // Normal Buffer
        glGenTextures(1, &normalTexture);
        glBindTexture(GL_TEXTURE_2D, normalTexture);
        glTexImage2D(GL_TEXTURE_2D, 
            /* level */ 0,
            /* texel format */  GL_RGB8_SNORM, 
            /* width, height, border */ window_width(), window_height(), 0,
            /* data format */ GL_RGB, /* data type */ GL_FLOAT,
            /* data */ nullptr); 
     
        // prefiltre la texture / alloue les mipmaps
        glGenerateMipmap(GL_TEXTURE_2D);


        // Albedo Buffer
        glGenTextures(1, &albedoTexture);
        glBindTexture(GL_TEXTURE_2D, albedoTexture);
        glTexImage2D(GL_TEXTURE_2D, 
            /* level */ 0,
            /* texel format */  GL_RGB10_A2, 
            /* width, height, border */ window_width(), window_height(), 0,
            /* data format */ GL_RGBA, /* data type */ GL_FLOAT,
            /* data */ nullptr); 
     
        // prefiltre la texture / alloue les mipmaps
        glGenerateMipmap(GL_TEXTURE_2D);

        // Depth Buffer
            // Depth Buffer en couleur flottante (linéaire)
        glGenTextures(1, &zBuffer);
        glBindTexture(GL_TEXTURE_2D, zBuffer);
        
        glTexImage2D(GL_TEXTURE_2D, 0,
            GL_DEPTH_COMPONENT, window_width(), window_height(), 0,
            GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, nullptr);
        
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glGenFramebuffers(1, &framebuffer);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer);
        
        // associer la texture à une sortie du framebuffer
        glFramebufferTexture(GL_DRAW_FRAMEBUFFER, /* attachment */ GL_COLOR_ATTACHMENT0, /* texture */ albedoTexture, /* mipmap level */ 0);
        glFramebufferTexture(GL_DRAW_FRAMEBUFFER, /* attachment */ GL_COLOR_ATTACHMENT1, /* texture */ normalTexture, /* mipmap level */ 0);
        glFramebufferTexture(GL_DRAW_FRAMEBUFFER,  /* attachment */ GL_DEPTH_ATTACHMENT, /* texture */ zBuffer, /* mipmap level */ 0);
        if(glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
                printf("FBO ERROR\n");


        GLenum buffers[]= { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
        glDrawBuffers(2, buffers);

        fbo_print_errors(framebuffer); 

        ///////////////// Préparation du quad //////////////////////

        // vertices du quad (positions NDC + texCoords)
        float quadVertices[] = {
            // positions   // texCoords
            -1.0f,  1.0f,  0.0f, 1.0f,
            -1.0f, -1.0f,  0.0f, 0.0f,
            1.0f, -1.0f,  1.0f, 0.0f,

            -1.0f,  1.0f,  0.0f, 1.0f,
            1.0f, -1.0f,  1.0f, 0.0f,
            1.0f,  1.0f,  1.0f, 1.0f
        };

        GLuint quadVBO;
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

        // positions
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        // texCoords
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
        glEnableVertexAttribArray(1);

        glBindVertexArray(0);


        //////////////////// Chargement des textures //////////////////////
        
        texture = read_texture(0, "data/rungholt/rungholt-RGBA.png");

        ///////////// Chargement et compilation des shaders ///////////////

        program= read_program("src/shader/textureMaison.glsl");
        program_print_errors(program);

        programHeightMap= read_program("src/shader/textureHeightMap.glsl");
        program_print_errors(programHeightMap);

        programGBuffer= read_program("src/shader/gbuffer.glsl");
        program_print_errors(programGBuffer);
        
        program_deffered= read_program("src/shader/program_deffered.glsl");
        program_print_errors(program_deffered);
        
        /////////////////// etat openGL par defaut ////////////////////////


        // projection par defaut, adaptee a la fenetre
        m_camera.projection(window_width(), window_height(), 45, 0.1, 1000);
        m_camera.translation(0, 5.5, 0);

        m_cameraHeightMap.projection(heightMap.width(), heightMap.height(), 45, pmin.y, pmax.y);

        glClearColor(0.2f, 0.2f, 0.2f, 1.f);        // couleur par defaut de la fenetre
        
        glClearDepth(1.f);                          // profondeur par defaut
        glEnable(GL_DEPTH_TEST);                    // activer le ztest
        glDepthFunc(GL_LESS);                       // ztest, conserver l'intersection la plus proche de la camera

        constructHeightMap(); 

        printf("Height Map OK !!!!!!!!!!\n");

        glEnable(GL_DEPTH_TEST);                    // activer le ztest

        m_orbiter.projection(window_width(), window_height(), 45);
        m_orbiter.lookat(pmin, pmax);

        initLights(500, pmin, pmax ); 

        
        std::vector<vec3> pos = m_scene.positions(); 


        return 0;   // pas d'erreur, sinon renvoyer -1
    }
    
    // destruction des objets de l'application
    int quit( )
    {
        m_scene.release();
        glDeleteProgram(program);
        glDeleteProgram(programHeightMap);
        glDeleteProgram(programGBuffer);
        glDeleteProgram(program_deffered);
        glDeleteTextures(1, &textureHeightMap);
        glDeleteTextures(1, &zbufferHeighMap);
        glDeleteTextures(1, &albedoTexture);
        glDeleteTextures(1, &normalTexture);
        glDeleteTextures(1, &zBuffer);
        glDeleteTextures(1, &texture);
        glDeleteFramebuffers(1, &fboHeightMap);
        glDeleteFramebuffers(1, &framebuffer);
        glDeleteVertexArrays(1, &quadVAO); 
        return 0;   // pas d'erreur
    }

    void updateOrbiter(){
        // recupere les mouvements de la souris
        int mx, my;
        unsigned int mb= SDL_GetRelativeMouseState(&mx, &my);
        int mousex, mousey;
        SDL_GetMouseState(&mousex, &mousey);
        
        // deplace la camera
        if(mb & SDL_BUTTON(1))
            m_orbiter.rotation(mx, my);      // tourne autour de l'objet
        else if(mb & SDL_BUTTON(3))
            m_orbiter.translation((float) mx / (float) window_width(), (float) my / (float) window_height()); // deplace le point de rotation
        else if(mb & SDL_BUTTON(2))
            m_orbiter.move(mx);           // approche / eloigne l'objet
        
        SDL_MouseWheelEvent wheel= wheel_event();
        if(wheel.y != 0)
        {
            clear_wheel_event();
            m_orbiter.move(8.f * wheel.y);  // approche / eloigne l'objet
        }
    
        const char *orbiter_filename= "app_orbiter.txt";
        // copy / export / write orbiter
        if(key_state('c'))
        {
            clear_key_state('c');
            m_orbiter.write_orbiter(orbiter_filename);
            
        }
        // paste / read orbiter
        if(key_state('v'))
        {
            clear_key_state('v');
            
            Orbiter tmp;
            if(tmp.read_orbiter(orbiter_filename) < 0)
                // ne pas modifer la camera en cas d'erreur de lecture...
                tmp= m_orbiter;
            
            m_orbiter= tmp;
        }
        
        // screenshot
        if(key_state('s'))
        {
            static int calls= 1;
            clear_key_state('s');
            screenshot("app", calls++);
            writeColorBuffer(); 
        }
    }

    void updateCamera(){
        // recupere les mouvements de la souris
        int mx, my;
        unsigned int mb= SDL_GetRelativeMouseState(&mx, &my);
        int mousex, mousey;
        SDL_GetMouseState(&mousex, &mousey);
        
        // deplace la camera
        if(mb & SDL_BUTTON(1)){
            m_camera.rotationCamX(-my);      // tourne autour de l'objet
            m_camera.rotationY(-mx); 
        }
            

        float e = 0.000001; 

        // modifie la position de l'objet en fonction des fleches de direction
        if(key_state(SDLK_UP)){
            m_camera.translation(0, 0, -0.25);     // en avant
            Point nextPosition = m_camera.getPosition(); 
            float hauteurScene = heightMap.valueXZ(nextPosition.x, nextPosition.z);

            float diff = abs(hauteurScene - (nextPosition.y - 1.5));
            if( diff >1)
                m_camera.translation(0, 0, 0.25);
            else if (diff > e && (hauteurScene>(nextPosition.y - 1.5)))
                m_camera.translation(0, 1.0, 0);
            else if (diff > e && (hauteurScene<(nextPosition.y - 1.5)))
                m_camera.translation(0, -1.0, 0);
        }

        if(key_state(SDLK_DOWN)){
            m_camera.translation(0, 0, 0.25);     // en arrière
            Point nextPosition = m_camera.getPosition(); 
            float hauteurScene = heightMap.valueXZ(nextPosition.x, nextPosition.z);

            float diff = abs(hauteurScene - (nextPosition.y - 1.5));
            if( diff >1)
                m_camera.translation(0, 0, -0.25);
            else if (diff > e && (hauteurScene>(nextPosition.y - 1.5)))
                m_camera.translation(0, 1.0, 0);
            else if (diff > e && (hauteurScene<(nextPosition.y - 1.5)))
                m_camera.translation(0, -1.0, 0);
        }  

        if(key_state(SDLK_LEFT))
            m_camera.rotationY(4);     // tourne vers la droite
        if(key_state(SDLK_RIGHT))
            m_camera.rotationY(-4);     // tourne vers la gauche
        
       
        
        // screenshot
        if(key_state('s'))
        {
            static int calls= 1;
            clear_key_state('s');
            screenshot("app", calls++);
        }
    }

    void constructHeightMap(){
        
        Point pmin = heightMap.pmin();
        Point pmax = heightMap.pmax(); 

        Point from = Point(0.5f * (pmin.x + pmax.x), m_cameraHeightMap.zfar(), 0.5 * (pmin.z + pmax.z));
        Point to = Point(0.5f * (pmin.x + pmax.x), m_cameraHeightMap.znear(), 0.5 * (pmin.z + pmax.z));
        Vector up = Vector(0,0,-1); 

        Transform view= m_cameraHeightMap.lookat(from, to, up);
        Transform projection= m_cameraHeightMap.projectionOrtho(pmin, pmax);
         
        // composition des 3 changements de repères
        Transform mvp= projection * view;

        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fboHeightMap);
        glViewport(0, 0, heightMap.width(), heightMap.height());
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glBindVertexArray( vaoScene );
        
        // parametrer le shader pour dessiner avec la couleur
        glUseProgram(programHeightMap);
        
        program_uniform(programHeightMap, "mvpMatrix", mvp);

        // dessiner les triangles du groupe
        glDrawArrays(GL_TRIANGLES, 0, m_scene.vertex_count());

        glBindTexture(GL_TEXTURE_2D, textureHeightMap);
        glGetTexImage(GL_TEXTURE_2D, 0,  GL_RED, GL_FLOAT, heightMap.data());

        heightMap.writeImage("heightmap.png");

    }
    
    // dessiner une nouvelle image
    int render( )
    {
        use_camera(); 

        updateLights(heightMap.pmin(), heightMap.pmax());

        Transform view, projection, model; 
        float znear, zfar; 

        if(!use_Camera){
            updateCamera(); 
            view= m_camera.view();
            projection= m_camera.projection();
            model= Identity();
            znear = m_camera.znear(); 
            zfar = m_camera.zfar();
        }
        else{
            updateOrbiter();
            view= m_orbiter.view();
            projection= m_orbiter.projection();
            model= Identity();
            znear = m_orbiter.znear(); 
            zfar = m_orbiter.zfar();
        }

        
        // composition des 3 changements de repères
        Transform mvp= projection * view * model;   // P(V(M*p))

        ////////////Construction du GBuffer ///////////////

        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer);
        glViewport(0, 0, window_width(), window_height());
        // couleur et profondeur par defaut
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        glBindVertexArray( vaoScene );
        glUseProgram( programGBuffer );

        program_uniform(programGBuffer, "mvpMatrix", mvp);
        program_uniform(programGBuffer, "modelMatrix", model);
        glUniform1f(glGetUniformLocation(programGBuffer, "znear"), znear);
        glUniform1f(glGetUniformLocation(programGBuffer, "zfar"), zfar);

        GLint location= glGetUniformLocation(programGBuffer, "diffuse_color");
        glUniform1i(location, 0);   // une seule texture utilisee, 0 dans ce cas

        // selectionner la texture :
        glBindTexture(GL_TEXTURE_2D, texture);
        
        // dessiner les triangles du groupe
        // glDrawArrays(GL_TRIANGLES, 0, m_scene.vertex_count());


        vec3 diff = heightMap.pmax() - heightMap.pmin();
        Point cellSize(
            diff.x / float(gridCoords(0)),
            diff.y / float(gridCoords(1)),
            diff.z / float(gridCoords(2))
        );
        int nbSommetDessiné = 0; 
        
        for (const TriangleGroup& group : triangleGrid) {

            // 1. Calculer la bounding box de la cellule correspondante
            // Ici group.index = numéro de cellule
            int Nx = (int)gridCoords(0);
            int Ny = (int)gridCoords(1);
            int Nz = (int)gridCoords(2);

            int ix = group.index % Nx;
            int iy = (group.index / Nx) % Ny;
            int iz = group.index / (Nx * Ny);

            Point pminCell;
            pminCell.x = heightMap.pmin().x + ix * cellSize.x;
            pminCell.y = heightMap.pmin().y + iy * cellSize.y;
            pminCell.z = heightMap.pmin().z + iz * cellSize.z;
            
            Point pmaxCell = pminCell + cellSize;

            if(!isVisibleObj(pminCell, pmaxCell) ){
                    // 2. Tester si la cellule est dans le frustum
                //std::cerr << "cellule : " << group.index<<" hors frustum" << std::endl;
                continue; // ignorer ce groupe / cellule
            }else if (!isVisibleFrustum(pminCell, pmaxCell) ){
                //std::cerr << "cellule : " << group.index<<" hors frustum" << std::endl;
                continue; // ignorer ce groupe / cellule
            }

            

            // 3. Dessiner les triangles de ce groupe
            // group.first = premier sommet dans le VBO
            // group.n     = nombre de sommets à dessiner
            glDrawArrays(GL_TRIANGLES, group.first, group.n);
            nbSommetDessiné+=group.n; 
        }


        //std::cerr << "Nombre de sommet dessiné: " << nbSommetDessiné << std::endl;
        
        glBindVertexArray(0);

        /////////// Affiche la scène /////////////////
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        
        glViewport(0, 0, window_width(), window_height());

        // couleur et profondeur par defaut
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        
        reload_shader(); 

        glBindVertexArray(quadVAO);
        glUseProgram(program_deffered); 
        Transform vp = projection * view; 
        Transform viewport= Viewport(window_width(), window_height());
        Transform vpi= viewport * vp;
        program_uniform(program_deffered, "invProjView", vpi.inverse());

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, albedoTexture);
        glUniform1i(glGetUniformLocation(program_deffered, "gAlbedo"), 0);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, normalTexture);
        glUniform1i(glGetUniformLocation(program_deffered, "gNormal"), 1);

        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, zBuffer);
        glUniform1i(glGetUniformLocation(program_deffered, "gDepth"), 2);

        glUniform3fv( glGetUniformLocation(program_deffered, "lights"), lights.size(), &lights[0].x );
        glUniform1ui(glGetUniformLocation(program_deffered, "nbLights"), lights.size());

        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);
        
        return 1;
    }

protected:
    Mesh m_scene;
    std::vector<TriangleGroup> m_groupsScene;    
    GLuint program= 0;
    GLuint vaoScene= 0;
    GLuint texture = 0; 
    GLuint programHeightMap= 0;
    GLuint fboHeightMap = 0;
    GLuint textureHeightMap = 0; 
    GLuint albedoTexture=0; 
    GLuint normalTexture=0; 
    GLuint zBuffer=0;
    GLuint framebuffer=0; 
    GLuint programGBuffer=0; 
    GLuint zbufferHeighMap = 0; 
    Image heightMapImage; 
    GLuint quadVAO; 
    GLuint program_deffered= 0;

    Vector gridCoords; 
    std::vector< TriangleGroup > triangleGrid; 

    HeightField heightMap;
    Camera m_camera;
    Camera m_cameraHeightMap;
    Orbiter m_orbiter; 
    std::vector<vec3> lights; 

    bool use_Camera = true; 

private: 
    void reload_shader(){
        if(key_state('r'))
        {
            clear_key_state('r');
            reload_program(program, "src/shader/program_deffered.glsl");
            program_print_errors(program);
        }
    }
    void use_camera(){
        if(key_state('o')){
            use_Camera = !use_Camera; 
        }
    }
    template<typename T>
    const T& clamp(const T& v, const T& lo, const T& hi)
    {
        return (v < lo) ? lo : (hi < v) ? hi : v;
    }

    void initLights(const int& nbLights, const Point& pmin, const Point& pmax){
        std::srand(std::time(nullptr)); 
        for (size_t i =0; i<nbLights; ++i){
            
            vec3 light; 
            light.x = pmin.x + std::rand() % int(pmax.x -pmin.x +1); 
            light.y = pmin.y + std::rand() % int(pmax.y -pmin.y +1); 
            light.z = pmin.z + std::rand() % int(pmax.z -pmin.z +1);

            lights.push_back(light); 
        }
    }

    void updateLights(Point pmin, Point pmax){
        float t = global_time() * 0.0001; // temps en secondes

        for (size_t i = 0; i < lights.size(); ++i) {
            // amplitude = demi-taille de la boîte englobante
            vec3 amplitude = (pmax - pmin) * 0.5f;
        
            // centre = centre de la boîte
            vec3 center = (pmax + pmin) * 0.5f;
        
            // mouvement sinusoïdal indépendant pour chaque lumière
            lights[i].x = center.x + amplitude.x * std::sin(t + i); 
            lights[i].y = center.y + amplitude.y * std::cos(t + i*1.3f); 
            lights[i].z = center.z + amplitude.z * std::sin(t + i*2.1f);
            
        }
    } 

    void fbo_print_errors(GLuint fbo){
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer);
        GLenum code = glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER);
        switch(code)
        {
            case GL_FRAMEBUFFER_COMPLETE:
                // OK : framebuffer prêt à être utilisé
                break;
            case GL_FRAMEBUFFER_UNDEFINED:
                std::cerr << "FBO erreur: cible indéfinie." << std::endl;
                break;
            case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
                std::cerr << "FBO erreur: attachement incomplet ou invalide." << std::endl;
                break;
            case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
                std::cerr << "FBO erreur: aucun attachement." << std::endl;
                break;
            case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
                std::cerr << "FBO erreur: un draw buffer n'a pas de texture associee." << std::endl;
                break;
            case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
                std::cerr << "FBO erreur: un read buffer invalide." << std::endl;
                break;
            case GL_FRAMEBUFFER_UNSUPPORTED:
                std::cerr << "FBO erreur: combinaison de formats non supportee." << std::endl;
                break;
            default:
                std::cerr << "FBO erreur inconnue (code=" << code << ")." << std::endl;
                break;
        }

    }

    void writeColorBuffer(){
        // Image image(window_width(), window_height());

        // for(uint i=0;i<window_width()* window_height();i++){
        //     float valNormalise = (m_data[i] - m_pmin.y) / (m_pmax.y - m_pmin.y) ;
        //     image(i) = Color(valNormalise, valNormalise, valNormalise, 1.0f);
        // }

        // write_image(image, nameFile);

        printf("je commence à écrire la texture\n");

        std::vector<float> pixels(window_width()* window_height() * 4);
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        glReadBuffer(GL_COLOR_ATTACHMENT0);
        glReadPixels(0, 0, window_width(), window_height(), GL_RGBA, GL_FLOAT, pixels.data());

        
        printf("j'ai récup la texture \n");

        Image image(window_width(), window_height());
        for(size_t i=0; i < pixels.size(); i+=4) {
            size_t pixelIndex = i / 4; 
            image(pixelIndex) = Color(pixels[i], pixels[i+1], pixels[i+2], pixels[i+3]);
        }
        
        printf("j'écris l'image'\n");
        write_image(image, "out.png");

        printf("image réussi\n");
    }

    bool PointInFrustum(vec3 p){
        vec4 pNdc = worldToNdc(p);

        if (-1 <= pNdc.x && pNdc.x <= 1 &&
            -1 <= pNdc.y && pNdc.y <= 1 &&
            -1 <= pNdc.z && pNdc.z <= 1)
        {
            return true;
        }
        return false;
    }

    bool allPointsOutsidePlane(const float& A,const float& B, const float& C, const float& D, std::vector<Point> pts){
        // bool hasPos = false; 
        // bool hasNeg = false;
        for (size_t i = 0; i<pts.size(); ++i){
            Point p = pts[i]; 
            float s= A*p(0) + B*p(1) + C*p(2) + D; //TODO 0.5 a supp
            if (s>=0) // A l'interieur ou à la surface du plan
                return false;//hasPos = true;
            // else   
            //     hasNeg = true; 
            
            // if( hasPos && hasNeg)
            //     return false; 
        }
        return true;
    }

    bool isVisibleFrustum (const Point& pmin, const Point& pmax){
        std::vector<Point> corners;
        int i = 0;
        for (int a=0; a<=1; ++a){
            for (int b=0; b<=1; ++b){
                for (int c=0; c<=1; ++c){
                    Point cornerWorld = getCorner(pmin, pmax, a, b, c); 
                    Point cornerNdc = worldToNdc(cornerWorld); 
                    corners.push_back(cornerNdc);
                }
            }
        }
        float p = 1.0; 
        
        if (allPointsOutsidePlane(1, 0, 0, p, corners))
            return false;
        if (allPointsOutsidePlane(-1, 0, 0, p, corners))
            return false;
        if (allPointsOutsidePlane(0, 1, 0, p, corners))
            return false;
        if (allPointsOutsidePlane(0, -1, 0, p, corners))
            return false;
        if (allPointsOutsidePlane(0, 0, 1, p, corners))
            return false;
        if (allPointsOutsidePlane(0, 0, -1, p, corners))
            return false;

        return true; 
    }

    bool isVisibleObj (const Point& pmin, const Point& pmax){
        std::vector<Point> ptsFrustumWorld = getPointFrustumWorld();
        
        if (allPointsOutsidePlane(1, 0, 0, -pmin(0), ptsFrustumWorld))
            return false;
        if (allPointsOutsidePlane(-1, 0, 0, pmax(0), ptsFrustumWorld))
            return false;
        if (allPointsOutsidePlane(0, 1, 0, -pmin(1), ptsFrustumWorld))
            return false;
        if (allPointsOutsidePlane(0, -1, 0, pmax(1), ptsFrustumWorld))
            return false;
        if (allPointsOutsidePlane(0, 0, 1, -pmin(2), ptsFrustumWorld))
            return false;
        if (allPointsOutsidePlane(0, 0, -1, pmax(2), ptsFrustumWorld))
            return false;

        return true; 
    }

    Point getCorner(const Point& pmin, const Point& pmax, int a, int b, int c)
    {
        return pmin + Vector(a, b, c) * (pmax - pmin);
    }



    Point worldToNdc(const Point p){
        Transform view, projection;
        if(!use_Camera){ 
            view= m_camera.view();
            projection= m_camera.projection();
        }
        else{
            view= m_orbiter.view();
            projection= m_orbiter.projection();
        }

        // 1. Combinaison des deux
        Transform VP = projection * view;

        // 2. Point en coordonnées monde
        vec4 worldPos = vec4(p);

        // 3. Application de la matrice VP
        vec4 clipPos = VP(worldPos);

        // 4. Division perspective
        return Point(clipPos(0)/ clipPos(3), clipPos(1)/ clipPos(3), clipPos(2)/ clipPos(3)) ;
    }

    std::vector< TriangleGroup > createGrid(Mesh& scene, const Point& pmin, const Point& pmax){
        std::vector<unsigned int> indexGrid;
        for(int i = 0; i<scene.triangle_count(); i++){
            TriangleData triangle = scene.triangle(i); 
            // Calcule du centre du triangle
            Point c((triangle.a.x + triangle.b.x + triangle.c.x) / 3.0f,
                    (triangle.a.y + triangle.b.y + triangle.c.y) / 3.0f,
                    (triangle.a.z + triangle.b.z + triangle.c.z) / 3.0f);

            Vector diff(pmax - pmin); 
            // coordonnée recentrer entre 0 et 1 pour connaitre dans quel cube ce situe le triangle
            vec3 rel;
            rel.x = (c.x - pmin.x) / diff.x;
            rel.y = (c.y - pmin.y) / diff.y;
            rel.z = (c.z - pmin.z) / diff.z;

            // 3. Coordonnées entières dans la grille
            int x = std::min(int(rel.x * gridCoords(0)), int(gridCoords.x)-1);
            int y = std::min(int(rel.y * gridCoords(1)), int(gridCoords.y)-1);
            int z = std::min(int(rel.z * gridCoords(2)), int(gridCoords.z)-1);

            // 5. Conversion 3D → 1D
            unsigned int index = x + gridCoords(0) * (y + gridCoords(1) * z);

            indexGrid.push_back(index);             
        }

        return scene.groups(indexGrid);
    }

    std::vector< Point > getPointFrustumWorld(){
        std::vector<Point> ptsFrustum; 

        Transform view, projection;
        if(!use_Camera){ 
            view= m_camera.view();
            projection= m_camera.projection();
        }
        else{
            view= m_orbiter.view();
            projection= m_orbiter.projection();
        }

        // 1. Combinaison des deux
        Transform VP = projection * view;
        Transform invVP = Inverse(VP); 

        float p = 1.0;
        ptsFrustum.push_back(invVP(Point(-p, -p, -p))); 
        ptsFrustum.push_back(invVP(Point(-p, -p, p))); 
        ptsFrustum.push_back(invVP(Point(-p, p, -p))); 
        ptsFrustum.push_back(invVP(Point(p, -p, -p))); 
        ptsFrustum.push_back(invVP(Point(-p, p, p))); 
        ptsFrustum.push_back(invVP(Point(p, -p, p))); 
        ptsFrustum.push_back(invVP(Point(p, p, p))); 
        ptsFrustum.push_back(invVP(Point(p, p, -p))); 
        return ptsFrustum;         
    }
};


int main( int argc, char **argv )
{
    // il ne reste plus qu'a creer un objet application et la lancer 
    TP tp;
    tp.run();
    
    return 0;
}



    // maison 

        // glBindVertexArray( vaoScene );
        
        // // parametrer le shader pour dessiner avec la couleur
        // glUseProgram(program);
        
        // program_uniform(program, "mvpMatrix", mvp);
        // program_uniform(program, "modelMatrix", model);

        // location= glGetUniformLocation(program, "diffuse_color");
        // glUniform1i(location, 0);   // une seule texture utilisee, 0 dans ce cas

        // location = glGetUniformLocation(program, "lights");
        // glUniform3fv( location, lights.size(), &lights[0].x );


        // //l = light[i] - vertexpos; 
        // // dist2 = len(l)*len(l)
        // // result += max(0.0, dot(Normalvertex, normalize(l)) / dist2)
        // glUniform1ui(glGetUniformLocation(program, "nbLights"), lights.size());
        
        // // selectionner la texture :
        // glBindTexture(GL_TEXTURE_2D, texture);

        // //glBindTexture(GL_TEXTURE_2D, albedoTexture);
        // //glGenerateMipmap(GL_TEXTURE_2D);

        // // dessiner les triangles du groupe
        // glDrawArrays(GL_TRIANGLES, 0, m_scene.vertex_count());


        // glBindTexture(GL_TEXTURE_2D, 0);
        
        // continuer, afficher une nouvelle image
        // tant que la fenetre est ouverte...