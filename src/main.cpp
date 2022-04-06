#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/filesystem.h>
#include <learnopengl/shader_m.h>
#include <learnopengl/camera.h>
#include <learnopengl/model.h>

#include <iostream>



unsigned int loadTexture(const char * path);


void framebuffer_size_callback(GLFWwindow *window, int width, int height);

void mouse_callback(GLFWwindow *window, double xpos, double ypos);

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);

void processInput(GLFWwindow *window);

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// camera

float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

int blinn=0;
// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

struct PointLight {
    glm::vec3 position;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;

    float constant;
    float linear;
    float quadratic;
};

struct ProgramState {
    glm::vec3 clearColor = glm::vec3(0);
    bool ImGuiEnabled = false;
    Camera camera;
    bool CameraMouseMovementUpdateEnabled = true;
    glm::vec3 turtlePosition = glm::vec3(50.0f,-100.0f,50.0f);
    glm::vec3 towerPosition = glm::vec3(0.0f,-100.0f,0.0f);

    float towerScale = 7.0f;
    float turtleScale = 0.5f;

    PointLight pointLight;
    ProgramState()
           : camera(glm::vec3(0.0f, 0.0f, 3.0f)) {}

    void SaveToFile(std::string filename);

    void LoadFromFile(std::string filename);
};

void ProgramState::SaveToFile(std::string filename) {
    std::ofstream out(filename);
    out << clearColor.r << '\n'
        << clearColor.g << '\n'
        << clearColor.b << '\n'
        << ImGuiEnabled << '\n'
        << camera.Position.x << '\n'
        << camera.Position.y << '\n'
        << camera.Position.z << '\n'
        << camera.Front.x << '\n'
        << camera.Front.y << '\n'
        << camera.Front.z << '\n';
}

void ProgramState::LoadFromFile(std::string filename) {
    std::ifstream in(filename);
    if (in) {
        in >> clearColor.r
           >> clearColor.g
           >> clearColor.b
           >> ImGuiEnabled
           >> camera.Position.x
           >> camera.Position.y
           >> camera.Position.z
           >> camera.Front.x
           >> camera.Front.y
           >> camera.Front.z;
    }
}

unsigned int loadCubemap(vector<std::string> faces);

ProgramState *programState;

void DrawImGui(ProgramState *programState);


int main() {
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Tajna kula", nullptr, nullptr);//otvaranje prozora
    if (window == nullptr) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);
    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {     //pronalazi OpenGl funkcije
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // tell stb_image.h to flip loaded texture's on the y-axis (before loading model).
    stbi_set_flip_vertically_on_load(false);

    programState = new ProgramState;
    programState->LoadFromFile("resources/program_state.txt");

    glEnable(GL_DEPTH_TEST);
    //glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    // build and compile shaders
    // -------------------------

    Shader ourShader("resources/shaders/lighting.vs", "resources/shaders/lighting.fs");
    Shader cubeShader("resources/shaders/magic.vs", "resources/shaders/magic.fs");
    Shader skyboxShader("resources/shaders/skybox.vs", "resources/shaders/skybox.fs");
    Shader lightCubeShader("light_cube.vs", "light_cube.fs");
    Shader sparkleShader("resources/shaders/blending.vs","resources/shaders/blending.fs");

    Model ourModel("resources/objects/10042_Sea_Turtle_V1_L3.123c708c1821-ff1e-470c-a557-b57441fbf771/10042_Sea_Turtle_V2_iterations-2.obj");
    ourModel.SetShaderTextureNamePrefix("material.");
    Model towerModel("resources/objects/Tower_v1_L1.123caaba91a9-bf80-44b9-80e6-d3483ab9b0a1/20950_Tower_v1_NEW.obj");
    towerModel.SetShaderTextureNamePrefix("material.");


    glm::vec3 lightPos(0.0f, 0.0f, 0.0f);

    float skyboxVertices[] = {
            -1.0f,  1.0f, -1.0f,
            -1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

            -1.0f,  1.0f, -1.0f,
            1.0f,  1.0f, -1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
            1.0f, -1.0f,  1.0f
    };



    glm::vec3 pointLightPositions[] = {
            glm::vec3( 4.7f,  1.2f,  2.0f),
            glm::vec3( 2.3f, -3.3f, -4.0f),
            glm::vec3( 0.0f,  0.0f, -3.0f)
    };
    float vertices[] = {
            // positions          // normals           // texture coords
            0.5f, -0.5f, -0.5f,  0.0f, 0.0f, -1.0f,  0.0f,  0.0f, // bottom-left
            -0.5f, -0.5f, -0.5f,  1.0f, 0.0f,-1.0f,  1.0f,  0.0f, // bottom-right
            -0.5f,  0.5f, -0.5f,  1.0f, 1.0f,  -1.0f,  1.0f,  1.0f,// top-right
            -0.5f,  0.5f, -0.5f,  1.0f, 1.0f,-1.0f,  1.0f,  1.0f, // top-right
            0.5f,  0.5f, -0.5f,  0.0f, 1.0f,-1.0f,  0.0f,  1.0f, // top-left
            0.5f, -0.5f, -0.5f,  0.0f, 0.0f, -1.0f,  0.0f,  0.0f, // bottom-left


            // front face (CCW winding)
            -0.5f, -0.5f,  0.5f,  0.0f, 0.0f, 1.0f,  0.0f,  0.0f,// bottom-left
            0.5f, -0.5f,  0.5f,  1.0f, 0.0f,1.0f,  1.0f,  1.0f, // bottom-right
            0.5f,  0.5f,  0.5f,  1.0f, 1.0f, 1.0f,  1.0f,  0.0f, // top-right
            0.5f,  0.5f,  0.5f,  1.0f, 1.0f,1.0f,  1.0f,  1.0f, // top-right
            -0.5f,  0.5f,  0.5f,  0.0f, 1.0f, 1.0f,  0.0f,  0.0f, // top-left
            -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,1.0f,  0.0f,  1.0f, // bottom-left


            // left face (CCW)
            -0.5f, -0.5f, -0.5f,  0.0f, 0.0f, 0.0f,  1.0f,  0.0f, // bottom-left
            -0.5f, -0.5f,  0.5f,  1.0f, 0.0f,0.0f,  0.0f,  1.0f, // bottom-right
            -0.5f,  0.5f,  0.5f,  1.0f, 1.0f, 0.0f,  1.0f,  1.0f, // top-right
            -0.5f,  0.5f,  0.5f,  1.0f, 1.0f, 0.0f,  0.0f,  1.0f,// top-right
            -0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 0.0f,  1.0f,  0.0f,// top-left
            -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,0.0f,  0.0f,  0.0f, // bottom-left


            // right face (CCW)
            0.5f, -0.5f,  0.5f,  0.0f, 0.0f, 0.0f,  1.0f,  0.0f, // bottom-left
            0.5f, -0.5f, -0.5f,  1.0f, 0.0f,0.0f,  1.0f,  1.0f, // bottom-right
            0.5f,  0.5f, -0.5f,  1.0f, 1.0f,0.0f,  0.0f,  1.0f, // top-right
            0.5f,  0.5f, -0.5f,  1.0f, 1.0f,0.0f,  0.0f,  1.0f, // top-right
            0.5f,  0.5f,  0.5f,  0.0f, 1.0f, 0.0f,  0.0f,  0.0f, // top-left
            0.5f, -0.5f,  0.5f,  0.0f, 0.0f,0.0f,  1.0f,  0.0f,
            // bottom-left

            // bottom face (CCW)
            -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,0.0f,  0.0f,  1.0f, // bottom-left
            0.5f, -0.5f, -0.5f,  1.0f, 0.0f,0.0f,  1.0f,  0.0f, // bottom-right
            0.5f, -0.5f,  0.5f,  1.0f, 1.0f, 0.0f,  1.0f,  1.0f,// top-right
            0.5f, -0.5f,  0.5f,  1.0f, 1.0f, 0.0f,  1.0f,  0.0f, // top-right
            -0.5f, -0.5f,  0.5f,  0.0f, 1.0f, 0.0f,  0.0f,  1.0f, // top-left
            -0.5f, -0.5f, -0.5f,  0.0f, 0.0f, 0.0f,  0.0f,  0.0f, // bottom-left

            // top face (CCW)
            -0.5f,  0.5f,  0.5f,  0.0f, 0.0f,0.0f,  0.0f,  1.0f, // bottom-left
            0.5f,  0.5f,  0.5f,  1.0f, 0.0f,0.0f,  1.0f,  1.0f, // bottom-right
            0.5f,  0.5f, -0.5f,  1.0f, 1.0f,0.0f,  1.0f,  0.0f, // top-right
            0.5f,  0.5f, -0.5f,  1.0f, 1.0f,0.0f,  1.0f,  0.0f, // top-right
            -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,0.0f,  0.0f,  0.0f, // top-left
            -0.5f,  0.5f,  0.5f,  0.0f, 0.0f, 0.0f,  0.0f,  1.0f // bottom-left
    };



    unsigned int cubeVBO, cubeVAO;
    glGenVertexArrays(1, &cubeVAO);
    glGenBuffers(1, &cubeVBO);

    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindVertexArray(cubeVAO);
    //pozicije
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    //normale
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    //tex koordinate
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    unsigned int lightCubeVAO;
    glGenVertexArrays(1, &lightCubeVAO);
    glBindVertexArray(lightCubeVAO);

    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);



    int sparkles=10000;
    float * rotateAngle=new float[sparkles];
    float x,y,z,div,tempY;
    y=-7.0f;
    glm::vec3* sparklePosition= new glm::vec3[sparkles];
    for(int i=0;i<sparkles;++i){

        rotateAngle[i]=(rand()%1000)/100+(rand()%1000)/1000 ;


        if(i%10==0){
            y+=0.5;

        }
        if(i%20){
            y+=1.0;
        }

        div=std::rand()%2000;

        x=std::rand()%200/div;
        z=std::rand()%200/div;
        if(i%4==0){
            x*=-1;
            z*=-1;
        }
        if(i%4==1) {
            x *= -1;
            z *= 1;
        }
        if(i%4==2) {
            x *= 1;
            z *= -1;
        }

        sparklePosition[i]=glm::vec3(x,y,z);
        y=-7.0f;
    }



    float transparentVertices[] = {

            0.0f,  0.5f,  0.0f,  0.0f,  0.0f,
            0.0f, -0.5f,  0.0f,  0.0f,  1.0f,
            1.0f, -0.5f,  0.0f,  1.0f,  1.0f,

            0.0f,  0.5f,  0.0f,  0.0f,  0.0f,
            1.0f, -0.5f,  0.0f,  1.0f,  1.0f,
            1.0f,  0.5f,  0.0f,  1.0f,  0.0f
    };



    unsigned int transparentVAO, transparentVBO;
    glGenVertexArrays(1, &transparentVAO);
    glGenBuffers(1, &transparentVBO);
    glBindVertexArray(transparentVAO);
    glBindBuffer(GL_ARRAY_BUFFER, transparentVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(transparentVertices), transparentVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glBindVertexArray(0);
    //ebo
    float ebo_vertices[] = {
            -0.5f, -0.5f, -0.5f, // levo dole nazad
            0.5f, -0.5f, -0.5f, //desno dole nazad
            -0.5f, 0.5f, -0.5f, // levo gore nazad
            0.5f, 0.5f, -0.5f, // desno gore nazad
            -0.5f, -0.5f, 0.5f, // levo dole napred
            0.5f, -0.5f, 0.5f, // desno dole napred
            -0.5f, 0.5f, 0.5f, // levo gore napred
            0.5f, 0.5f, 0.5f //desno gore napred
    };

    unsigned int indices[] = {
            0, 1, 2, //iza
            1, 2, 3,

            4, 5, 6,//ispred
            5, 6, 7,

            0, 4, 2,//levo
            4, 2, 6,

            1, 5, 3,//desno
            5, 3, 7,

            2, 3, 6, //gore
            3, 6, 7,

            0, 1, 4,//dole
            1, 4, 5
    };

    unsigned int VBO, VAO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    // bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(ebo_vertices), ebo_vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);



    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // remember: do NOT unbind the EBO while a VAO is active as the bound element buffer object IS stored in the VAO; keep the EBO bound.


    glBindVertexArray(0);
    //kraj ebo


    //skybox
    unsigned int skyboxVAO, skyboxVBO;
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    vector<std::string> faces
            {
                    FileSystem::getPath("resources/textures/lagoon/lagoon_ft.tga"),
                    FileSystem::getPath("resources/textures/lagoon/lagoon_bk.tga"),
                    FileSystem::getPath("resources/textures/lagoon/lagoon_up.tga"),
                    FileSystem::getPath("resources/textures/lagoon/lagoon_dn.tga"),
                    FileSystem::getPath("resources/textures/lagoon/lagoon_rt.tga"),
                    FileSystem::getPath("resources/textures/lagoon/lagoon_lf.tga")
            };

    skyboxShader.use();
    skyboxShader.setInt("skybox", 0);
    unsigned int cubemapTexture = loadCubemap(faces);



    unsigned  int diffuseMap=loadTexture(FileSystem::getPath("resources/textures/—Pngtree—blue creative texture starry sky_5370563.png").c_str());


    cubeShader.use();
    cubeShader.setInt("material.diffuse", 0);

    cubeShader.use();
    // light properties
    cubeShader.setVec3("light.ambient", 0.3f, 0.3f, 0.3f);
    cubeShader.setVec3("light.diffuse", 0.5f, 0.5f, 0.5f);
    cubeShader.setVec3("light.specular", 0.1f, 0.1f, 0.1f);

    // material properties
    cubeShader.setVec3("material.specular", 0.5f, 0.5f, 0.5f);
    cubeShader.setFloat("material.shininess", 64.0f);



    unsigned int transparentTexture=loadTexture(FileSystem::getPath("resources/textures/PngItem_3292435.png").c_str());
    unsigned int transparentTexture2=loadTexture(FileSystem::getPath("resources/textures/PngItem_3292435.png").c_str());



    sparkleShader.use();
    sparkleShader.setInt("texture1",0);


    PointLight& pointLight = programState->pointLight;
    pointLight.position = lightPos;
    pointLight.ambient = glm::vec3(0.2f, 0.2f, 0.2f);
    pointLight.diffuse = glm::vec3(0.5, 0.5, 0.5);
    pointLight.specular = glm::vec3(1.0, 1.0, 1.0);

    pointLight.constant = 0.1f;
    pointLight.linear = 0.01f;
    pointLight.quadratic = 0.0001f;




    // render loop
    // -----------
    while (!glfwWindowShouldClose(window)) {
        // per-frame time logic
        // --------------------
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        // -----
        processInput(window);


        // render
        // ------
        glClearColor(programState->clearColor.r, programState->clearColor.g, programState->clearColor.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


        glm::mat4 projection = glm::perspective(glm::radians(programState->camera.Zoom),
                                                (float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f, 1000.0f);
        glm::mat4 view = programState->camera.GetViewMatrix();



        cubeShader.use();

        cubeShader.setVec3("light.direction", programState->camera.Front);//glm::vec3(glm::sin(glfwGetTime()),-1.0f,glm::cos(glfwGetTime()))
        cubeShader.setVec3("viewPos", programState->camera.Position);


        cubeShader.setMat4("projection", projection);
        cubeShader.setMat4("view", view);


        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D,diffuseMap);

        for (unsigned int i = 0; i < 3; i++)
        {

            glEnable(GL_CULL_FACE);
            glCullFace(GL_BACK);

            glm::mat4 modelLight = glm::mat4(1.0f);
            modelLight = glm::translate(modelLight, pointLightPositions[i]*3.0f*glm::vec3((float)glm::cos(glfwGetTime()), glm::cos(glfwGetTime()), glm::sin(glfwGetTime())));
            modelLight = glm::scale(modelLight, glm::vec3(3.0f));
            cubeShader.setMat4("model", modelLight);


            glBindVertexArray(cubeVAO);
            glDrawArrays(GL_TRIANGLES, 0, 36);

        }

        glDisable(GL_CULL_FACE);


        lightCubeShader.use();
        projection = glm::perspective(glm::radians(programState->camera.Zoom),(float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f, 1000.0f);
        lightCubeShader.setMat4("projection", projection);
        view = programState->camera.GetViewMatrix();
        lightCubeShader.setMat4("view", view);



        glm::mat4 modelLight = glm::mat4(1.0f);
        modelLight = glm::translate(modelLight, lightPos);
        modelLight = glm::scale(modelLight, glm::vec3(0.8f));
        lightCubeShader.setMat4("model", modelLight);



        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);


        sparkleShader.use();
        sparkleShader.setMat4("projection",projection);
        sparkleShader.setMat4("view",view);

        glBindVertexArray(transparentVAO);
        glBindTexture(GL_TEXTURE_2D,transparentTexture);


        for(int i=0;i<sparkles/2;++i) {
            glm::mat4 modelSparkle = glm::mat4(1.0f);
            modelSparkle = glm::scale(modelSparkle, glm::vec3(3.0f));
            modelSparkle = glm::translate(modelSparkle,sparklePosition[i]);

            modelSparkle=glm::rotate(modelSparkle,(float)glfwGetTime()+rotateAngle[i],glm::vec3(0.0f,1.0f,0.0f));
            sparkleShader.setMat4("model", modelSparkle);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }
        glBindTexture(GL_TEXTURE_2D,transparentTexture2);

        for(int i=sparkles/2;i<sparkles;++i) {
            glm::mat4 modelSparkle = glm::mat4(1.0f);
            modelSparkle = glm::scale(modelSparkle, glm::vec3(3.0f));
            modelSparkle = glm::translate(modelSparkle,sparklePosition[i]);
            modelSparkle=glm::rotate(modelSparkle,2*(float)glfwGetTime()+rotateAngle[i],glm::vec3(0.0f,-1.0f,0.0f));

            sparkleShader.setMat4("model", modelSparkle);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }

        ourShader.use();

        pointLight.position = glm::vec3(4.0, 4.0f, 4.0); //* cos(currentFrame),  * sin(currentFrame)
       // pointLight.position = glm::vec3(4.0 * cos(currentFrame), 4.0f, 4.0 * sin(currentFrame));
        ourShader.setVec3("pointLight.position", lightPos);
        ourShader.setVec3("pointLight.ambient", pointLight.ambient);
        ourShader.setVec3("pointLight.diffuse", pointLight.diffuse);
        ourShader.setVec3("pointLight.specular", pointLight.specular);
        ourShader.setFloat("pointLight.constant", pointLight.constant);
        ourShader.setFloat("pointLight.linear", pointLight.linear);
        ourShader.setFloat("pointLight.quadratic", pointLight.quadratic);
        ourShader.setVec3("viewPosition", programState->camera.Position);
        ourShader.setFloat("material.shininess", 32.0f);
        // view/projection transformations
        // render the loaded model
        projection = glm::perspective(glm::radians(programState->camera.Zoom),(float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f, 1000.0f);
        view = programState->camera.GetViewMatrix();
        glm::mat4 model = glm::mat4(1.0f);
        ourShader.setMat4("model", model);
        ourShader.setMat4("projection", projection);
        ourShader.setMat4("view", view);

        model = glm::translate(model,programState->turtlePosition);

        model = glm::rotate(model, -1.5706f, glm::vec3(1.0f, 0.0f, 0.0f)); //za 90 stepeni
        model = glm::rotate(model, 1.5706f, glm::vec3(0.0f, 0.0f, 1.0f));

        model = glm::scale(model, glm::vec3(programState->turtleScale));


        ourShader.setMat4("model", model);
        ourModel.Draw(ourShader);


        //kulaa
        model = glm::mat4(1.0f);
        ourShader.setMat4("model", model);
        ourShader.setMat4("projection", projection);
        ourShader.setMat4("view", view);
        model = glm::translate(model,glm::vec3(programState->towerPosition));

        model = glm::rotate(model, -1.5706f, glm::vec3(1.0f, 0.0f, 0.0f));
        //model = glm::rotate(model, -(float)glfwGetTime(), glm::vec3(0.0f, 0.0f, 1.0f));

        model = glm::scale(model, glm::vec3(programState->towerScale));

        ourShader.setMat4("model", model);

        towerModel.Draw(ourShader);

        ourShader.setInt("blinn",blinn);
        if(blinn==1){
            std::cout<<"blinn"<<endl;
        }
        else{
            std::cout<<"phong"<<endl;
        }

        //ourModel.Draw(ourShader);




        projection = glm::perspective(glm::radians(programState->camera.Zoom),(float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f, 1000.0f);
        view = programState->camera.GetViewMatrix();

        //glDisable(GL_CULL_FACE);
        glDepthMask(GL_FALSE);
        glDepthFunc(GL_LEQUAL);
        skyboxShader.use();
        skyboxShader.setMat4("view", glm::mat4(glm::mat3(view)));
        skyboxShader.setMat4("projection", projection);
        glm::vec3 myVector = glm::vec3((sin(glfwGetTime())+1)/2, (sin(glfwGetTime())+1)/2, (sin(glfwGetTime())+1)/2);
        skyboxShader.setVec3("skyboxColor", myVector);
        glBindVertexArray(skyboxVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glDepthMask(GL_TRUE);
        glDepthFunc(GL_LESS);


        //glBindVertexArray(0);




        glfwSwapBuffers(window);
        glfwPollEvents();
    }


    programState->SaveToFile("resources/program_state.txt");
    delete programState;
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(FORWARD, 5*deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(BACKWARD, 5*deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(LEFT, 5*deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(RIGHT, 5*deltaTime);

    if(glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
        programState->turtlePosition.z -= 3;
    if(glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
        programState->turtlePosition.z += 3;
    if(glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
        programState->turtlePosition.x -= 3;
    if(glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
        programState->turtlePosition.x += 3;
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow *window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    if (programState->CameraMouseMovementUpdateEnabled)
        programState->camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
    programState->camera.ProcessMouseScroll(yoffset);
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_F1 && action == GLFW_PRESS) {
        programState->ImGuiEnabled = !programState->ImGuiEnabled;
        if (programState->ImGuiEnabled) {
            programState->CameraMouseMovementUpdateEnabled = false;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        } else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
    }
    else if(key==GLFW_KEY_B && action==GLFW_PRESS){
        blinn=!blinn;
    }
}

unsigned int loadCubemap(vector<std::string> faces)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++)
    {
        unsigned char *data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }
        else
        {
            std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
            return -1;
            stbi_image_free(data);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}
unsigned int loadTexture(const char * path)             //ucitavanje teksture
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;

    stbi_set_flip_vertically_on_load(true);

    unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
    stbi_set_flip_vertically_on_load(false);
    if (data)
    {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, format == GL_RGBA ? GL_CLAMP_TO_EDGE : GL_REPEAT);    //kada koordinate izadju van opsega
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, format == GL_RGBA ? GL_CLAMP_TO_EDGE : GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); //ako se tekstura mapira na manji objekat => linearno
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);   //ako se mapira na mnogo vece =>linearno

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;

}
