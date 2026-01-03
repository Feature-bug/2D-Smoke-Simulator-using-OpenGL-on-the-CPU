#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <shader.h>

//Resizing window function
void framebuffer_size_callback(GLFWwindow* window, int width, int height){
    glViewport(0, 0, width, height);
}

const unsigned int screen_width = 1280;
const unsigned int screen_height = 720;

#define GRID_RESOLUTION 150
#define SIZE ((GRID_RESOLUTION + 2) * (GRID_RESOLUTION + 2))
#define INDEX(i, j) ((i) + (GRID_RESOLUTION + 2) * (j))
#define SWAP(density_prev, density) {float* temp = density_prev; density_prev = density; density = temp;}
float *u, *v, *u_prev, *v_prev, *dens, *dens_prev;

//Add sources to grid
void add_source(float* final, float* initial, float dt);

//Diffusion function
void diffuse(int boundary, float* final, float* initial, float diffusion, float dt);

void density_step(float* density, float* density_prev, float* u, float* v, float diffusion, float dt);
//Advection function
void advect(int boundary, float* final, float* initial, float* u, float* v, float dt);

//Velocity Solver
void velocity_step(float* u, float* v, float* u0, float* v0, float visc, float dt);

//Poisson equation
void project(float* u, float* v, float* p, float* div);

//Boundary conditions
void set_bound(int boundary, float* x);

//Dissipation Factor
void dissipation(float* d, float rate){
    for(int i = 0; i < SIZE; i++){
        d[i] *= rate; //Multiply each cell
    }
}

float* source;
static double xPos, yPos;
static double xPosPrevious, yPosPrevious;
//Input function
void processInput(GLFWwindow* window, float dt){
    if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS){
        glfwSetWindowShouldClose(window, true);
    }

    //Adding sources and force by mouse
    glfwGetCursorPos(window, &xPos, &yPos);
    double dx = xPos - xPosPrevious;
    double dy = yPos - yPosPrevious;
    int i = (int)((xPos / screen_width) * GRID_RESOLUTION );
    int j = (int)(((screen_height - yPos) / screen_height) * GRID_RESOLUTION );

    if (i >= 1 && i <= GRID_RESOLUTION && j >= 1 && j <= GRID_RESOLUTION){
        u_prev[INDEX(i, j)] += dx * 10.0f;
        v_prev[INDEX(i, j)] += -dy * 10.0f;

        if(glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS){
        source[INDEX(i, j)] = 9000.0f; //Source strength
        v_prev[INDEX(i, j)] = 60.0f;   //Vertical velocity
        add_source(dens, source, dt);
    }
    }
    xPosPrevious = xPos;
    yPosPrevious = yPos;
}

int main(){
    //Config settings
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    //Setting window
    GLFWwindow* window = glfwCreateWindow(screen_width, screen_height, "Smoke", NULL, NULL);
    if(window == NULL){
        printf("Failed to create GLFW window.");
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    //Initialising GLAD
    if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)){
        printf("Failed to initialise GLAD");
        return -1;
    }

    Shader shader = shader_create("Src/Shader/fluid.vs", "Src/Shader/fluid.fs");
    unsigned int densityTexture;
    glGenTextures(1, &densityTexture);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, densityTexture);
    //Setting texture wrapping option on currently bound object
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, GRID_RESOLUTION + 2, GRID_RESOLUTION + 2, 0, GL_RED, GL_FLOAT, NULL);
    //Initialising framebuffer
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    u = calloc(SIZE, sizeof(float));
    u_prev = calloc(SIZE, sizeof(float));
    v = calloc(SIZE, sizeof(float));
    v_prev = calloc(SIZE, sizeof(float));
    dens = calloc(SIZE, sizeof(float));
    dens_prev = calloc(SIZE, sizeof(float));
    source = calloc(SIZE, sizeof(float));
    float quadVertices[] = {
        // positions   // texCoords
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f
    };
    
    unsigned int indices[] = {0, 1, 2, 0, 2, 3};
    
    unsigned int VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    
    glBindVertexArray(VAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    glBindVertexArray(0);

    double previousTime = glfwGetTime();
    int frameCount = 0;
    float lastFrame = 0;
    //Render loop
    while(!glfwWindowShouldClose(window)){
        memset(source, 0, SIZE * sizeof(float));
        memset(v_prev, 0, SIZE * sizeof(float));
        memset(u_prev, 0, SIZE * sizeof(float));

        double currentTime = glfwGetTime();
        frameCount++;

        if(currentTime - previousTime >= 1){
            double fps = (double)(frameCount) / (currentTime - previousTime);

            char title[50];
            sprintf(title, "Smoke | FPS: %.1f", fps);
            glfwSetWindowTitle(window, title);

            frameCount = 0;
            previousTime = currentTime;
        }

        float currentFrame = (float)glfwGetTime();
        float dt = currentFrame - lastFrame;
        lastFrame = currentFrame;

        if(dt > 0.03f){
            dt = 0.03f;
        }

        processInput(window, dt); //Intialising input

        //Setting colour to render per frame
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        velocity_step(u, v, u_prev, v_prev, 0.0f, dt);
        density_step(dens, dens_prev, u, v, 0.00001f, dt);

        glBindTexture(GL_TEXTURE_2D, densityTexture);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, GRID_RESOLUTION + 2, GRID_RESOLUTION + 2, GL_RED, GL_FLOAT, dens);
        shader_use(&shader);
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
        //Frame changing and Event queueing
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    free(source);
    free(u);
    free(v);
    free(u_prev);
    free(v_prev);
    free(dens);
    free(dens_prev);
    //Termination of above operations(freeing space)
    glfwTerminate();
    return 0;
}

void add_source(float* final, float* initial, float dt){
    int i;
    for(i = 0; i < SIZE; i++){
        final[i] += dt * initial[i];
    }
}

void diffuse(int boundary, float* final, float* initial, float diffusion, float dt){
    int i, j, k;
    float a = dt * diffusion * GRID_RESOLUTION * GRID_RESOLUTION;

    for(k = 0; k < 20; k++){
        for(i = 1; i <= GRID_RESOLUTION; i++){
            for(j = 1; j <= GRID_RESOLUTION; j++){
                final[INDEX(i, j)] = (initial[INDEX(i, j)] + a * (final[INDEX(i - 1, j)] + final[INDEX(i + 1, j)] + final[INDEX(i, j - 1)] + final[INDEX(i, j + 1)])) / (1 + 4 * a);
            }
        }
        set_bound(boundary, final);
    }
}

void density_step(float* density, float* density_prev, float* u, float* v, float diffusion, float dt){
    add_source(density, density_prev, dt);
    SWAP(density_prev, density);
    diffuse(0, density, density_prev, diffusion, dt);
    SWAP(density_prev, density);
    advect(0, density, density_prev, u, v, dt);
    dissipation(density, 0.982);
}

void advect(int boundary, float* final, float* initial, float* u, float* v, float dt){
    int i, j, i0, j0, i1, j1;
    float x, y, s0, t0, s1, t1, dt0;
    
    dt0 = dt * GRID_RESOLUTION;
    for(i = 1; i <= GRID_RESOLUTION; i++){
        for(j = 1; j <= GRID_RESOLUTION; j++){
            x = i - dt0 * u[INDEX(i, j)];
            y = j - dt0 * v[INDEX(i, j)];
            if(x < 0.5){
                x = 0.5;
            }
            if(x > GRID_RESOLUTION + 0.5){
                x = GRID_RESOLUTION + 0.5;
            }
            i0 = (int)x;
            i1 = i0 + 1;

            if(y < 0.5){
                y = 0.5;
            }
            if(y > GRID_RESOLUTION + 0.5){
                y = GRID_RESOLUTION + 0.5;
            }
            j0 = (int)y;
            j1 = j0 + 1;
            
            s1 = x - i0;
            s0 = 1 - s1;
            t1 = y - j0;
            t0 = 1 - t1;

            final[INDEX(i, j)] = s0 * (t0 * initial[INDEX(i0, j0)] + t1 * initial[INDEX(i0, j1)]) + s1 * (t0 * initial[INDEX(i1, j0)] + t1 * initial[INDEX(i1, j1)]);
        }
    }
    set_bound(boundary, final);
}

void velocity_step(float* u, float* v, float* u0, float* v0, float visc, float dt){
    add_source(u, u0, dt);
    add_source(v, v0, dt);

    SWAP(u0, u);
    diffuse(1, u, u0, visc, dt);

    SWAP(v0, v);
    diffuse(2, v, v0, visc, dt);

    project(u, v, u0, v0);

    SWAP(u0, u);
    SWAP(v0, v);

    advect(1, u, u0, u0, v0, dt);
    advect(2, v, v0, u0, v0, dt);

    project(u, v, u0, v0);

    dissipation(v, 0.999);
    dissipation(u, 0.999);
}

void project(float* u, float* v, float* p, float* div){
    int i, j, k;
    float h;

    h = 1.0 / GRID_RESOLUTION;
    for(i = 1; i <= GRID_RESOLUTION; i++){
        for(j = 1; j <= GRID_RESOLUTION; j++){
            div[INDEX(i, j)] = -0.5 * h * (u[INDEX(i+1, j)] - u[INDEX(i - 1, j)] + v[INDEX(i, j + 1)] - v[INDEX(i, j-1)]);
            p[INDEX(i, j)] = 0;
        }
    }
    set_bound(0, div);
    set_bound(0, p);

    for(k = 0; k < 20; k++){
        for(i = 1; i <= GRID_RESOLUTION; i++){
            for(j = 1; j <= GRID_RESOLUTION; j++){
                p[INDEX(i, j)] = (div[INDEX(i, j)] + p[INDEX(i - 1, j)] + p[INDEX(i + 1, j)] + p[INDEX(i, j - 1)] + p[INDEX(i, j + 1)]) / 4;
            }
        }
        set_bound(0, p);
    }

    for(i = 1; i <= GRID_RESOLUTION; i++){
        for(j = 1; j <= GRID_RESOLUTION; j++){
            u[INDEX(i, j)] -= 0.5 * (p[INDEX(i + 1, j)] - p[INDEX(i - 1, j)]) / h;
            v[INDEX(i, j)] -= 0.5 * (p[INDEX(i , j + 1)] - p[INDEX(i, j - 1)]) / h;
        }
    }
    set_bound(1, u);
    set_bound(2, v);
}

void set_bound(int boundary, float* x){
    int i;
    for(i = 1; i <= GRID_RESOLUTION; i++){
        x[INDEX(0, i)] = boundary == 1 ? -x[INDEX(1, i)] : x[INDEX(1, i)];
        x[INDEX(GRID_RESOLUTION + 1, i)] = boundary == 1 ? -x[INDEX(GRID_RESOLUTION, i)] : x[INDEX(GRID_RESOLUTION, i)];
        x[INDEX(i, 0)] = boundary == 2 ? -x[INDEX(i, 1)] : x[INDEX(i, 1)];
        x[INDEX(i, GRID_RESOLUTION + 1)] = boundary == 2 ? -x[INDEX(i, GRID_RESOLUTION)] : x[INDEX(i, GRID_RESOLUTION)];
    }
    x[INDEX(0, 0)] = 0.5 * (x[INDEX(1, 0)] + x[INDEX(0, 1)]);
    x[INDEX(0, GRID_RESOLUTION + 1)] = 0.5 * (x[INDEX(1, GRID_RESOLUTION + 1)] + x[INDEX(0, GRID_RESOLUTION)]);
    x[INDEX(GRID_RESOLUTION + 1, 0)] = 0.5 * (x[INDEX(GRID_RESOLUTION, 0)] + x[INDEX(GRID_RESOLUTION + 1, 1)]);
    x[INDEX(GRID_RESOLUTION + 1, GRID_RESOLUTION + 1)] = 0.5 * (x[INDEX(GRID_RESOLUTION, GRID_RESOLUTION + 1)] + x[INDEX(GRID_RESOLUTION + 1, GRID_RESOLUTION)]);

}

