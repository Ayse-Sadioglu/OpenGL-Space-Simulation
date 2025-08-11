#include "Angel.h"
#include "mat.h"
#include "vec.h"
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <iostream>
#include <vector>



const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec3 vNormal;

out vec3 interpColor;

uniform mat4 ModelView;
uniform mat4 Projection;
uniform vec3 LightPos;
uniform vec3 LightColor;
uniform vec3 ObjectColor;
uniform bool UseLighting;

void main() {
    if (UseLighting) {
       
        vec3 Normal = normalize(mat3(ModelView) * vNormal);

        
        vec3 LightDir = normalize(LightPos - vec3(ModelView * vec4(vPosition, 1.0)));

        // Compute diffuse lighting
        float diff = max(dot(Normal, LightDir), 0.0);
        vec3 diffuse = diff * LightColor;

        // Final color = Diffuse * Object color
        interpColor = ObjectColor * diffuse;
    } else {
        interpColor = ObjectColor; // No lighting for unlit objects (e.g., white square)
    }

    gl_Position = Projection * ModelView * vec4(vPosition, 1.0);
}
)";


const char* fragmentShaderSource = R"(
#version 330 core
in vec3 interpColor;
out vec4 FragColor;

void main() {
    FragColor = vec4(interpColor, 1.0);
}
)";

std::vector<vec3> sphereVertices;
std::vector<GLuint> sphereIndices;
std::vector<vec3> sphereNormals;
GLuint sphereVAO, sphereVBO, sphereEBO;
vec4 eye, at, up;  
vec3 shipPosition = vec3(1.0f, 10.0f, 5.0f); 
vec3 shipDirection = vec3(1.0f, 0.0f, 0.0f); 
float shipSpeed = 0.02f; 
bool isPaused = false;


enum CameraView {
    CONTROL_DESK,  
    FRONT_STATION, 
    BEHIND_SHIP,   
    TOP_VIEW       
};


CameraView currentView = CONTROL_DESK;

std::vector<vec3> tetrahedronVertices = {
    vec3(0.0f,  0.5f,  0.0f),
    vec3(-0.5f, -0.5f,  0.5f),
    vec3(0.5f, -0.5f,  0.5f),
    vec3(0.0f, -0.5f, -0.5f)
};

std::vector<GLuint> tetrahedronIndices = {
    0, 1, 2,
    0, 2, 3,
    0, 3, 1,
    1, 3, 2
};
std::vector<vec3> tetraNormals = {
    vec3(0.0f,  1.0f,  0.0f),
    vec3(-0.707f, -0.5f,  0.5f),
    vec3(0.707f, -0.5f,  0.5f),
    vec3(0.0f, -0.5f, -1.0f)
};


GLfloat planet_coords[][3] = {
    {30, 30, 30},
    {30, 170, 15},
    {80, 110, 25},
    {70, 60, 12},
    {90, 150, 13},
    {120, 80, 17},
    {150, 40, 15},
    {160, 170, 22}
};


std::vector<GLuint> tetrahedronEdges = {
    0, 1,  0, 2,  0, 3,  
    1, 2,  2, 3,  3, 1  
};


GLuint tetraEdgeVAO, tetraEdgeVBO, tetraEdgeEBO;

GLuint tetraVAO, tetraVBO, tetraEBO;




GLfloat planet_colors[][3] = {
    {0.30, 0.30, 0.30}, 
    {1.00, 0.00, 0.00}, 
    {0.00, 1.00, 0.00}, 
    {0.00, 0.00, 1.00}, 
    {1.00, 1.00, 0.00}, 
    {1.00, 0.00, 1.00}, 
    {0.00, 1.00, 1.00}, 
    {1.00, 1.00, 1.00} 
};


GLuint VAO, VBO, EBO, shaderProgram;
GLuint ModelViewLoc, ProjectionLoc;


vec3 vertices[] = {
    vec3(-0.8, -0.8, 0.0),  
    vec3(0.8, -0.8, 0.0),  
    vec3(0.8, 0.8, 0.0),  
    vec3(-0.8,  0.8, 0.0)   
};



GLuint indices[] = {
    0, 1, 2,  
    2, 3, 0   
};

std::vector<vec3> torusVertices;
std::vector<vec3> torusNormals;
std::vector<GLuint> torusIndices;
GLuint torusVAO, torusVBO, torusEBO;
GLuint squareVAO, squareVBO, squareEBO;

float stationRotationAngle = 0.0f;
float stationRotationSpeed = 0.0f;
float savedShipSpeed = 0.0f;


// Updates spaceship position if simulation is not paused
void update(int value) {
    if (!isPaused) {
        shipPosition += shipDirection * shipSpeed;
    }
    glutPostRedisplay();
    glutTimerFunc(16, update, 0); 
}

// Updates station rotation angle if simulation is not paused
void timer(int value) {
    if (!isPaused) { 
        stationRotationAngle += stationRotationSpeed;
    }
    glutPostRedisplay();
    glutTimerFunc(16, timer, 0);  
}


void updateCamera() {
    if (currentView == CONTROL_DESK) {
        //Position the camera slightly behind and above the spaceship looking forward
        vec3 offset = -normalize(shipDirection) * 3.0f + vec3(0.0f, 0.0f, 1.5f); 
        eye = vec4(shipPosition + offset, 1.0f);
        at = vec4(shipPosition + normalize(shipDirection) * 10.0f, 1.0f); 
        up = vec4(0.0, 0.0, 1.0, 0.0);
    }
    else if (currentView == FRONT_STATION) {
        // Camera is placed in front of the station looking at it
        vec3 stationPos = vec3(100.0f, 10.0f, 10.0f);
        vec3 stationFront = vec3(1.0f, 0.0f, 0.0f);
        eye = vec4(stationPos - stationFront * 30.0f, 1.0f);
        at = vec4(stationPos, 1.0f);
        up = vec4(0.0, 0.0, 1.0, 0.0);
    }
    else if (currentView == BEHIND_SHIP) {
        //Camera is behind and above the spaceship looking in its movement direction
        vec3 offset = -normalize(shipDirection) * 15.0f + vec3(0.0f, 0.0f, 10.0f); 
        eye = vec4(shipPosition + offset, 1.0f);
        at = vec4(shipPosition + normalize(shipDirection) * 5.0f, 1.0f); 
        up = vec4(0.0, 0.0, 1.0, 0.0);
    }
    else if (currentView == TOP_VIEW) {
        //High above looking down to see the whole scene
        eye = vec4(100.0, 100.0, 400.0, 1.0);  
        at = vec4(100.0, 10.0, 10.0, 1.0);     
        up = vec4(0.0, 1.0, 0.0, 0.0);
    }
}


// Setup functions to render tetrahedrons used in spaceship and station
void setupTetrahedronEdges() {
    glGenVertexArrays(1, &tetraEdgeVAO);
    glGenBuffers(1, &tetraEdgeVBO);
    glGenBuffers(1, &tetraEdgeEBO);

    glBindVertexArray(tetraEdgeVAO);

    glBindBuffer(GL_ARRAY_BUFFER, tetraEdgeVBO);
    glBufferData(GL_ARRAY_BUFFER, tetrahedronVertices.size() * sizeof(vec3), &tetrahedronVertices[0], GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, tetraEdgeEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, tetrahedronEdges.size() * sizeof(GLuint), &tetrahedronEdges[0], GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
}
void setupTetrahedronBuffers() {
    glGenVertexArrays(1, &tetraVAO);
    glGenBuffers(1, &tetraVBO);
    glGenBuffers(1, &tetraEBO);

    glBindVertexArray(tetraVAO);

    
    std::vector<vec3> tetraNormals = {
        normalize(vec3(0.0f, 1.0f, 0.0f)),
        normalize(vec3(-0.707f, -0.5f, 0.5f)),
        normalize(vec3(0.707f, -0.5f, 0.5f)),
        normalize(vec3(0.0f, -0.5f, -1.0f))
    };

  
    std::vector<float> tetraData;
    for (size_t i = 0; i < tetrahedronVertices.size(); i++) {
        tetraData.push_back(tetrahedronVertices[i].x);
        tetraData.push_back(tetrahedronVertices[i].y);
        tetraData.push_back(tetrahedronVertices[i].z);
        tetraData.push_back(tetraNormals[i].x);
        tetraData.push_back(tetraNormals[i].y);
        tetraData.push_back(tetraNormals[i].z);
    }

   
    glBindBuffer(GL_ARRAY_BUFFER, tetraVBO);
    glBufferData(GL_ARRAY_BUFFER, tetraData.size() * sizeof(float), &tetraData[0], GL_STATIC_DRAW);

  
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, tetraEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, tetrahedronIndices.size() * sizeof(GLuint), &tetrahedronIndices[0], GL_STATIC_DRAW);

    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

  
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}


void generateTorus(float R, float r, int numMajor, int numMinor) {
// It calculates positions and normals for each vertex by using two angles:
// theta for the major circle and phi for the minor circle.
// The positions are stored in torusVertices, and the normals in torusNormals.
// Triangle indices are generated to connect the vertices in a mesh using torusIndices.
    torusVertices.clear();
    torusNormals.clear();
    torusIndices.clear();

    for (int i = 0; i < numMajor; i++) {
        float theta = (float)i / numMajor * 2.0f * M_PI;
        float cosTheta = cos(theta);
        float sinTheta = sin(theta);

        for (int j = 0; j < numMinor; j++) {
            float phi = (float)j / numMinor * 2.0f * M_PI;
            float cosPhi = cos(phi);
            float sinPhi = sin(phi);

            float x = (R + r * cosPhi) * cosTheta;
            float y = (R + r * cosPhi) * sinTheta;
            float z = r * sinPhi;

            vec3 normal = normalize(vec3(cosPhi * cosTheta, cosPhi * sinTheta, sinPhi));
            torusVertices.push_back(vec3(x, y, z));
            torusNormals.push_back(normal);
        }
    }

    for (int i = 0; i < numMajor; i++) {
        for (int j = 0; j < numMinor; j++) {
            int first = (i * numMinor) + j;
            int second = ((i + 1) % numMajor) * numMinor + j;
            int next = (j + 1) % numMinor;

            torusIndices.push_back(first);
            torusIndices.push_back(second);
            torusIndices.push_back(first + next);

            torusIndices.push_back(second);
            torusIndices.push_back(second + next);
            torusIndices.push_back(first + next);
        }
    }
}

void setupTorusBuffers() {
// It creates and binds VAO, VBO, and EBO for the torus.
// Then it interleaves position and normal data in a single array and uploads it to the GPU.
// Finally it sets up the vertex attribute pointers so that the shader can access the data correctly.
    glGenVertexArrays(1, &torusVAO);
    glGenBuffers(1, &torusVBO);
    glGenBuffers(1, &torusEBO);
    glBindVertexArray(torusVAO);

    std::vector<float> torusData;
    for (size_t i = 0; i < torusVertices.size(); i++) {
        torusData.push_back(torusVertices[i].x);
        torusData.push_back(torusVertices[i].y);
        torusData.push_back(torusVertices[i].z);
        torusData.push_back(torusNormals[i].x);
        torusData.push_back(torusNormals[i].y);
        torusData.push_back(torusNormals[i].z);
    }

    glBindBuffer(GL_ARRAY_BUFFER, torusVBO);
    glBufferData(GL_ARRAY_BUFFER, torusData.size() * sizeof(float), &torusData[0], GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, torusEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, torusIndices.size() * sizeof(GLuint), &torusIndices[0], GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}


void setupSquareBuffers() {
    // Set up VAO and buffers for drawing a square (ground)
    glGenVertexArrays(1, &squareVAO);
    glGenBuffers(1, &squareVBO);
    glGenBuffers(1, &squareEBO);

    glBindVertexArray(squareVAO);

    glBindBuffer(GL_ARRAY_BUFFER, squareVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, squareEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
}


// Generates a sphere by computing vertex positions and normals over latitude/longitude bands
// and builds an index buffer to form triangles for rendering the sphere used for station and planets
void generateSphere(float radius, int latitudeBands, int longitudeBands) {
    sphereVertices.clear();
    sphereIndices.clear();
    sphereNormals.clear();

    for (int lat = 0; lat <= latitudeBands; lat++) {
        float theta = lat * M_PI / latitudeBands;
        float sinTheta = sin(theta);
        float cosTheta = cos(theta);

        for (int lon = 0; lon <= longitudeBands; lon++) {
            float phi = lon * 2.0f * M_PI / longitudeBands;
            float sinPhi = sin(phi);
            float cosPhi = cos(phi);

            float x = cosPhi * sinTheta;
            float y = cosTheta;
            float z = sinPhi * sinTheta;

            vec3 vertex = vec3(radius * x, radius * y, radius * z);
            vec3 normal = normalize(vec3(x, y, z)); // Normalize for lighting

            sphereVertices.push_back(vertex);
            sphereNormals.push_back(normal);
        }
    }

    // Generate indices for rendering
    for (int lat = 0; lat < latitudeBands; lat++) {
        for (int lon = 0; lon < longitudeBands; lon++) {
            int first = (lat * (longitudeBands + 1)) + lon;
            int second = first + longitudeBands + 1;

            sphereIndices.push_back(first);
            sphereIndices.push_back(second);
            sphereIndices.push_back(first + 1);

            sphereIndices.push_back(second);
            sphereIndices.push_back(second + 1);
            sphereIndices.push_back(first + 1);
        }
    }
}
void setupSphereBuffers() {
    glGenVertexArrays(1, &sphereVAO);
    glGenBuffers(1, &sphereVBO);
    glGenBuffers(1, &sphereEBO);

    glBindVertexArray(sphereVAO);

    std::vector<float> sphereData;
    for (size_t i = 0; i < sphereVertices.size(); i++) {
        sphereData.push_back(sphereVertices[i].x);
        sphereData.push_back(sphereVertices[i].y);
        sphereData.push_back(sphereVertices[i].z);
        sphereData.push_back(sphereNormals[i].x);
        sphereData.push_back(sphereNormals[i].y);
        sphereData.push_back(sphereNormals[i].z);
    }

    glBindBuffer(GL_ARRAY_BUFFER, sphereVBO);
    glBufferData(GL_ARRAY_BUFFER, sphereData.size() * sizeof(float), &sphereData[0], GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphereEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sphereIndices.size() * sizeof(GLuint), &sphereIndices[0], GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}


void init() {
    glewExperimental = GL_TRUE;
    glewInit();

    // Compile shaders
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // Get uniform locations
    glUseProgram(shaderProgram);
    ModelViewLoc = glGetUniformLocation(shaderProgram, "ModelView");
    ProjectionLoc = glGetUniformLocation(shaderProgram, "Projection");
    // Set light and object colors
    glUniform3f(glGetUniformLocation(shaderProgram, "LightPos"), 1.0f, 1.0f, 2.0f); 
    glUniform3f(glGetUniformLocation(shaderProgram, "LightColor"), 1.0f, 1.0f, 1.0f); 
    glUniform3f(glGetUniformLocation(shaderProgram, "ObjectColor"), 1.0f, 0.0f, 1.0f); 
    setupSquareBuffers();
    generateSphere(0.5f, 30, 30);
    setupSphereBuffers();
    generateTorus(2.5f, 0.7f, 40, 40);
    setupTetrahedronBuffers();
    setupTetrahedronEdges();
    setupTorusBuffers();
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
}

void keyboard(unsigned char key, int x, int y) {
    switch (key) {
    case 'a': //speed it up
        if (!isPaused) shipSpeed += 0.02f;
        break;
    case 'd'://slow it down
        if (!isPaused) shipSpeed = std::max(0.0f, shipSpeed - 0.02f);
        break;
    case 'j':  //station rotates faster
        if (!isPaused) stationRotationSpeed += 2.0f;
        break;
    case 'k':  // station rotates slower
        if (!isPaused) stationRotationSpeed -= 2.0f;
        break;
    case 'p':
        isPaused = !isPaused; 
        if (isPaused) {
            savedShipSpeed = shipSpeed; // save speed
            shipSpeed = 0.0f;          
        }
        else {
            shipSpeed = savedShipSpeed; 
        }
        break;
        //camera views
        case 'c':
            currentView = CONTROL_DESK;
            break;
        case 's':
            currentView = FRONT_STATION;
            break;
        case 't':
            currentView = BEHIND_SHIP;
            break;
        case 'w':
            currentView = TOP_VIEW;
            break;
        }
        glutPostRedisplay();
}


void specialKeyboard(int key, int x, int y) {//to rotate spaceship left right
    vec3 zAxis = vec3(0.0f, 0.0f, 1.0f); //rotation axis (Z-axis)
    float turnAmount = 0.2f; 

    if (key == GLUT_KEY_LEFT) {
        // Rotate the ship direction to the left
        shipDirection = normalize(shipDirection + cross(shipDirection, zAxis) * turnAmount);
    }
    else if (key == GLUT_KEY_RIGHT) {
        // Rotate the ship direction to the right
        shipDirection = normalize(shipDirection - cross(shipDirection, zAxis) * turnAmount);
    }

    glutPostRedisplay();
}


void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(shaderProgram);
    updateCamera();
    // Set up view and projection matrices
    mat4 view = LookAt(eye, at, up);
    mat4 projection = Perspective(45.0, 800.0 / 600.0, 0.1, 5000.0);

    glUniformMatrix4fv(ModelViewLoc, 1, GL_TRUE, view);
    glUniformMatrix4fv(ProjectionLoc, 1, GL_TRUE, projection);

    // Update ship position
    shipPosition += shipDirection * shipSpeed; // Move the ship forward

    float rotationAngle = atan2(shipDirection.y, shipDirection.x) * 180.0 / M_PI;
    mat4 shipTransform = Translate(shipPosition.x, shipPosition.y, shipPosition.z) * RotateZ(rotationAngle);



    //spaceship
    // First Torus (Orange - XZ plane) 
    glUniform3f(glGetUniformLocation(shaderProgram, "ObjectColor"), 1.0f, 0.5f, 0.0f);
    glUniformMatrix4fv(ModelViewLoc, 1, GL_TRUE, view * shipTransform * RotateX(90));
    glBindVertexArray(torusVAO);
    glDrawElements(GL_TRIANGLES, torusIndices.size(), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    //Second Torus (Green - YZ plane)
    glUniform3f(glGetUniformLocation(shaderProgram, "ObjectColor"), 0.5f, 1.0f, 0.0f);
    glUniformMatrix4fv(ModelViewLoc, 1, GL_TRUE, view * shipTransform * RotateY(90));
    glBindVertexArray(torusVAO);
    glDrawElements(GL_TRIANGLES, torusIndices.size(), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    //Tetrahedron (Front of the ship)
    glUniform1i(glGetUniformLocation(shaderProgram, "UseLighting"), true);

    // Draw solid tetrahedron
    glUniform3f(glGetUniformLocation(shaderProgram, "ObjectColor"), 1.0f, 0.0f, 0.0f); 
    glUniformMatrix4fv(ModelViewLoc, 1, GL_TRUE, view * shipTransform * Translate(3.0f, 0.0f, 0.0f) * Scale(2.5f, 2.5f, 2.5f));
    glBindVertexArray(tetraVAO);
    glDrawElements(GL_TRIANGLES, tetrahedronIndices.size(), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    // Draw edges with a thick black outline (it was hard to see thats why i used this)
    glUniform3f(glGetUniformLocation(shaderProgram, "ObjectColor"), 0.0f, 0.0f, 0.0f); 
    glLineWidth(4.0f);  
    glBindVertexArray(tetraEdgeVAO);
    glDrawElements(GL_LINES, tetrahedronEdges.size(), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    //Ground 
    GLint wasLightingOn;
    glGetUniformiv(shaderProgram, glGetUniformLocation(shaderProgram, "UseLighting"), &wasLightingOn); 

    glUniform1i(glGetUniformLocation(shaderProgram, "UseLighting"), false); 

    glUniform3f(glGetUniformLocation(shaderProgram, "ObjectColor"), 1.0f, 1.0f, 1.0f);
    mat4 squareModel = Translate(0.0f, 0.0f, -5.0f) * RotateX(-90) * Scale(200.0f, 200.0f, 1.0f);
    glUniformMatrix4fv(ModelViewLoc, 1, GL_TRUE, view * squareModel);

    glBindVertexArray(squareVAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);


    glUniform1i(glGetUniformLocation(shaderProgram, "UseLighting"), wasLightingOn);

    //Space Station (Large Gray Sphere)
    glUniform3f(glGetUniformLocation(shaderProgram, "ObjectColor"), 0.6f, 0.6f, 0.6f); 

    //Apply station rotation
    mat4 stationTransform = Translate(100.0f, 10.0f, 10.0f) * RotateZ(stationRotationAngle);
    glUniformMatrix4fv(ModelViewLoc, 1, GL_TRUE, view * stationTransform * Scale(20.0f, 20.0f, 20.0f));

    glBindVertexArray(sphereVAO);
    glDrawElements(GL_TRIANGLES, sphereIndices.size(), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    //Attach a red tetrahedron to the front of the space station
    glUniform3f(glGetUniformLocation(shaderProgram, "ObjectColor"), 1.0f, 0.0f, 0.0f);
    mat4 stationFrontModel = stationTransform * Translate(0.0f, 20.0f, 0.0f) * Scale(4.0f, 4.0f, 4.0f);
    glUniformMatrix4fv(ModelViewLoc, 1, GL_TRUE, view * stationFrontModel);
    glBindVertexArray(tetraVAO);
    glDrawElements(GL_TRIANGLES, tetrahedronIndices.size(), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);


    //Render planets
    glUniform1i(glGetUniformLocation(shaderProgram, "UseLighting"), true);
    glBindVertexArray(sphereVAO);
    for (int i = 0; i < 8; i++) {
        GLfloat x = planet_coords[i][0];
        GLfloat y = planet_coords[i][1];
        GLfloat z = planet_coords[i][2];

        glUniform3f(glGetUniformLocation(shaderProgram, "ObjectColor"),
            planet_colors[i][0],
            planet_colors[i][1],
            planet_colors[i][2]);

        mat4 sphereModel = Translate(x / 8.0f, y / 8.0f, z / 8.0f) * Scale(5.0f, 5.0f, 5.0f);
        glUniformMatrix4fv(ModelViewLoc, 1, GL_TRUE, view * sphereModel);
        glDrawElements(GL_TRIANGLES, sphereIndices.size(), GL_UNSIGNED_INT, 0);
    }
    glBindVertexArray(0);
    glutSwapBuffers();
}



int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
    glutInitWindowSize(800, 600);
    glutCreateWindow("MajorTom");
    glewInit();
    init();
    generateSphere(0.5f, 30, 30);
    setupSphereBuffers();
    glutTimerFunc(16, update, 0);
    glutTimerFunc(16, timer, 0);
    glutDisplayFunc(display);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKeyboard);
    glutMainLoop();

    return 0;
}

