/* Overly commented OpenGL learning code
   Hopefully this will help me remember it
   better.
*/

#include <GL/glew.h> // pulls all the opengl function from our graphics card
#include <GLFW/glfw3.h> // allows us to create a window and fill it with opengl stuff
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>

/* debug macro which trigger a breakpoint on an OpenGL error
   this is not really good practice because it only works with Visual Studio,
   but it gets the job done for the time being
*/
#define ASSERT(x) if(!(x)) __debugbreak();

/* macro that allows us to use GLCall() instead of wrapping our
   functions in GLClearError() and GLLogCall()
*/
#define GLCall(x) GLClearError();\
    x;\
    ASSERT(GLLogCall(#x, __FILE__, __LINE__))

// debug function to clear error coming from previous functions
static void GLClearError() {
    while (glGetError() != GL_NO_ERROR);
}

// debug function to print all errors from previous functions
static bool GLLogCall(const char* function, const char* file, int line) {

    while (GLenum error = glGetError()) {
        std::cout << "[OPENGL ERROR] " << error << " " << file  << ": " << function
            << ": line "<< line << std::endl;

        return false;
    }

    return true;
}

// pair struct with vertex and fragment shader source code
struct ShaderProgramSource {
    std::string VertexSource;
    std::string FragmentSource;
};

// code for loading shaders from a separate source file
static ShaderProgramSource ParseShader(const std::string& filepath) {

    std::ifstream stream(filepath);

    enum class ShaderType {
        NONE = -1,
        VERTEX = 0,
        FRAGMENT = 1,
    };

    std::string line;
    std::stringstream ss[2];
    ShaderType type = ShaderType::NONE;

    while (getline(stream, line)) {
        if (line.find("#shader") != std::string::npos) {

            if (line.find("vertex") != std::string::npos) {
                type = ShaderType::VERTEX;
            }
            else if (line.find("fragment") != std::string::npos) {
                type = ShaderType::FRAGMENT;
            }
                
        }
        else {

            // this line gives us a false positive warning
#pragma warning(push)
#pragma warning(disable:6385)
            ss[(int)type] << line << '\n';
#pragma warning(pop)

        }
    }

    return { ss[0].str(), ss[1].str() };
}

// code for compiling shaders with additional debug info
static unsigned int CompileShader(unsigned int type, const std::string& source) {
    unsigned int id = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(id, 1, &src, nullptr);
    glCompileShader(id);


    /* Error handling code */
    int result;
    glGetShaderiv(id, GL_COMPILE_STATUS, &result);

    if (result == GL_FALSE) {
        int length;
        glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length);
        char* message = (char*)_malloca(sizeof(char) * length);
        glGetShaderInfoLog(id, length, &length, message);

        std::cout << "failed to compile " << (type == GL_VERTEX_SHADER ? "vertex" : "fragment") << " shader!" << std::endl;
        std::cout << message << std::endl;

        GLCall(glDeleteShader(id));
        return 0;
    }

    return id;
}


// code to generate a program from the shaders loaded in beforehand
static unsigned int CreateShader(const std::string& vertexShader, const std::string& fragmentShader) {
    unsigned int program = glCreateProgram();
    unsigned int vs = CompileShader(GL_VERTEX_SHADER, vertexShader);
    unsigned int fs = CompileShader(GL_FRAGMENT_SHADER, fragmentShader);

    GLCall(glAttachShader(program, fs));
    GLCall(glAttachShader(program, vs));

    GLCall(glLinkProgram(program));
    GLCall(glValidateProgram(program));

    GLCall(glDeleteShader(vs));
    GLCall(glDeleteShader(fs));

    return program;
}

int main(void){
    GLFWwindow* window;

    /* Initialize the library */
    if (!glfwInit())
        return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    /* Create a windowed mode window and its OpenGL context */
    window = glfwCreateWindow(640, 480, "Hello World", NULL, NULL);
    if (!window){
        glfwTerminate();
        return -1;
    }

    /* Make the window's context current */
    glfwMakeContextCurrent(window);
    
    /* synchronize with monitor refresh rate */
    glfwSwapInterval(1);

    if (glewInit() != GLEW_OK)
        std::cout << "GLEW Initialization failed!" << std::endl;


    std::cout << glGetString(GL_VERSION) << std::endl;

    // vertex positions (3 2D Vertex)
    float positions[] = {
        -0.5f, -0.5f,
         0.5f, -0.5f,
         0.5f,  0.5f,
        -0.5f,  0.5f,

    };


    // indices of our vertex to draw a rectangle out of two triangles
    unsigned int indices[] = {
        0, 1, 2,
        2, 3, 0
    };

    // creating a vertex array object to specify a layout for our vertex buffer
    unsigned int vao;
    GLCall(glGenVertexArrays(1, &vao));
    GLCall(glBindVertexArray(vao));

    // Creating an array buffer for our vertex and filling it with our positions
    unsigned int buffer;
    GLCall(glGenBuffers(1, &buffer));
    GLCall(glBindBuffer(GL_ARRAY_BUFFER, buffer));
    GLCall(glBufferData(GL_ARRAY_BUFFER, 4 * 2 * sizeof(float), positions, GL_STATIC_DRAW));

    GLCall(glEnableVertexAttribArray(0));
    GLCall(glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, 0));

    // creating an index buffer object
    unsigned int ibo;
    GLCall(glGenBuffers(1, &ibo));
    GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo));
    GLCall(glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6 * sizeof(unsigned int), indices, GL_STATIC_DRAW));



    ShaderProgramSource source = ParseShader("res/shaders/Basic.shader");

    /* output to the terminal the source code for the vertex and fragment buffers.
       (helps learning it, in some way :shrug:) 
    */
    std::cout << "VERTEX" << std::endl;
    std::cout << source.VertexSource << std::endl;
    std::cout << "FRAGMENT" << std::endl;
    std::cout << source.FragmentSource << std::endl;
    
    // creating a vertex and fragment shader program from source code and binding it
    unsigned int shader = CreateShader(source.VertexSource, source.FragmentSource);
    GLCall(glUseProgram(shader));

    // initializing uniform value u_Color from fragment shader
    int location = glGetUniformLocation(shader, "u_Color");

    float r = 0.0f; // automating the red channel of our uniform baby!
    float increment = 0.01f;
    // if u_Color is not used in the shader than location = -1 so it isn't always a problem
    ASSERT(location != -1);

    GLCall(glBindVertexArray(0));
    GLCall(glUseProgram(0));
    GLCall(glBindBuffer(GL_ARRAY_BUFFER, 0));
    GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));

    /* Loop until the user closes the window */
    while (!glfwWindowShouldClose(window)){
        /* Render here */
        glClear(GL_COLOR_BUFFER_BIT);

        GLCall(glUseProgram(shader));
        GLCall(glUniform4f(location, r, 0.4f, 0.6f, 1.0f));

        GLCall(glBindVertexArray(vao));

        // we bound our index buffer object while the vertex array object was bound resulting in linking the IBO with it
        // this means that binding our VAO again also binds the index buffer object at the same time
        //GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo)); <- became useless

        if (r > 1.0f || r < 0.0f) {
            increment = -increment;
        }

        r += increment;


        /* we do not need to pass indices as last parameter of this function
           because it has been bound since its creation 
        */

        GLCall(glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr));
        

        /* Swap front and back buffers */
        glfwSwapBuffers(window);

        /* Poll for and process events */
        glfwPollEvents();
    }

    // deleting the program constituted from our vertex and fragment shaders.
    GLCall(glDeleteProgram(shader));
    glfwTerminate();
    return 0;
}