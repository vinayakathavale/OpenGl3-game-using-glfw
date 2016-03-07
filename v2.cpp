#include <iostream>
#include <cmath>
#include <fstream>
#include <vector>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <SOIL.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace std;

double projectile_x_coordinate=-3,projectile_y_coordinate=-2,projectile_velocity=0,projectile_angle=0;
double projectile_x_velocity=0,projectile_y_velocity=0;
double air_resistance=0.998,gravity=0.02,bounce=0.7;
int flag=0;

float ortho_x_max=4.0f,ortho_x_min=-4.0f;
float ortho_y_max = 4.0f,ortho_y_min=-4.0f;

typedef struct object
{

  int collided;
  int obj_num;
  double radius,x_coordinate,y_coordinate,z_coordinate;
  double x_velocity,y_velocity;

}object;

object array_collisions[100];   

struct VAO {
    GLuint VertexArrayID;
    GLuint VertexBuffer;
    GLuint ColorBuffer;
    GLuint TextureBuffer;
    GLuint TextureID;


    GLenum PrimitiveMode;
    GLenum FillMode;
    int NumVertices;
};
typedef struct VAO VAO;

struct GLMatrices {
	glm::mat4 projection;
	glm::mat4 model;
	glm::mat4 view;
	GLuint MatrixID;
} Matrices;

struct FTGLFont {
  FTFont* font;
  GLuint fontMatrixID;
  GLuint fontColorID;
} GL3Font;

GLuint programID, fontProgramID, textureProgramID;

/* Function to load Shaders - Use it as it is */
GLuint LoadShaders(const char * vertex_file_path,const char * fragment_file_path) {

	// Create the shaders
	GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
	GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

	// Read the Vertex Shader code from the file
	std::string VertexShaderCode;
	std::ifstream VertexShaderStream(vertex_file_path, std::ios::in);
	if(VertexShaderStream.is_open())
	{
		std::string Line = "";
		while(getline(VertexShaderStream, Line))
			VertexShaderCode += "\n" + Line;
		VertexShaderStream.close();
	}

	// Read the Fragment Shader code from the file
	std::string FragmentShaderCode;
	std::ifstream FragmentShaderStream(fragment_file_path, std::ios::in);
	if(FragmentShaderStream.is_open()){
		std::string Line = "";
		while(getline(FragmentShaderStream, Line))
			FragmentShaderCode += "\n" + Line;
		FragmentShaderStream.close();
	}

	GLint Result = GL_FALSE;
	int InfoLogLength;

	// Compile Vertex Shader
	printf("Compiling shader : %s\n", vertex_file_path);
	char const * VertexSourcePointer = VertexShaderCode.c_str();
	glShaderSource(VertexShaderID, 1, &VertexSourcePointer , NULL);
	glCompileShader(VertexShaderID);

	// Check Vertex Shader
	glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	std::vector<char> VertexShaderErrorMessage(InfoLogLength);
	glGetShaderInfoLog(VertexShaderID, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
	fprintf(stdout, "%s\n", &VertexShaderErrorMessage[0]);

	// Compile Fragment Shader
	printf("Compiling shader : %s\n", fragment_file_path);
	char const * FragmentSourcePointer = FragmentShaderCode.c_str();
	glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer , NULL);
	glCompileShader(FragmentShaderID);

	// Check Fragment Shader
	glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	std::vector<char> FragmentShaderErrorMessage(InfoLogLength);
	glGetShaderInfoLog(FragmentShaderID, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);
	fprintf(stdout, "%s\n", &FragmentShaderErrorMessage[0]);

	// Link the program
	fprintf(stdout, "Linking program\n");
	GLuint ProgramID = glCreateProgram();
	glAttachShader(ProgramID, VertexShaderID);
	glAttachShader(ProgramID, FragmentShaderID);
	glLinkProgram(ProgramID);

	// Check the program
	glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
	glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	std::vector<char> ProgramErrorMessage( max(InfoLogLength, int(1)) );
	glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
	fprintf(stdout, "%s\n", &ProgramErrorMessage[0]);

	glDeleteShader(VertexShaderID);
	glDeleteShader(FragmentShaderID);

	return ProgramID;
}

static void error_callback(int error, const char* description)
{
    fprintf(stderr, "Error: %s\n", description);
}

void quit(GLFWwindow *window)
{
    glfwDestroyWindow(window);
    glfwTerminate();
    exit(EXIT_SUCCESS);
}

glm::vec3 getRGBfromHue (int hue)
{
  float intp;
  float fracp = modff(hue/60.0, &intp);
  float x = 1.0 - abs((float)((int)intp%2)+fracp-1.0);

  if (hue < 60)
    return glm::vec3(1,x,0);
  else if (hue < 120)
    return glm::vec3(x,1,0);
  else if (hue < 180)
    return glm::vec3(0,1,x);
  else if (hue < 240)
    return glm::vec3(0,x,1);
  else if (hue < 300)
    return glm::vec3(x,0,1);
  else
    return glm::vec3(1,0,x);
}
/* Generate VAO, VBOs and return VAO handle */
struct VAO* create3DObject (GLenum primitive_mode, int numVertices, const GLfloat* vertex_buffer_data, const GLfloat* color_buffer_data, GLenum fill_mode=GL_FILL)
{
    struct VAO* vao = new struct VAO;
    vao->PrimitiveMode = primitive_mode;
    vao->NumVertices = numVertices;
    vao->FillMode = fill_mode;

    // Create Vertex Array Object
    // Should be done after CreateWindow and before any other GL calls
    glGenVertexArrays(1, &(vao->VertexArrayID)); // VAO
    glGenBuffers (1, &(vao->VertexBuffer)); // VBO - vertices
    glGenBuffers (1, &(vao->ColorBuffer));  // VBO - colors

    glBindVertexArray (vao->VertexArrayID); // Bind the VAO 
    glBindBuffer (GL_ARRAY_BUFFER, vao->VertexBuffer); // Bind the VBO vertices 
    glBufferData (GL_ARRAY_BUFFER, 3*numVertices*sizeof(GLfloat), vertex_buffer_data, GL_STATIC_DRAW); // Copy the vertices into VBO
    glVertexAttribPointer(
                          0,                  // attribute 0. Vertices
                          3,                  // size (x,y,z)
                          GL_FLOAT,           // type
                          GL_FALSE,           // normalized?
                          0,                  // stride
                          (void*)0            // array buffer offset
                          );

    glBindBuffer (GL_ARRAY_BUFFER, vao->ColorBuffer); // Bind the VBO colors 
    glBufferData (GL_ARRAY_BUFFER, 3*numVertices*sizeof(GLfloat), color_buffer_data, GL_STATIC_DRAW);  // Copy the vertex colors
    glVertexAttribPointer(
                          1,                  // attribute 1. Color
                          3,                  // size (r,g,b)
                          GL_FLOAT,           // type
                          GL_FALSE,           // normalized?
                          0,                  // stride
                          (void*)0            // array buffer offset
                          );

    return vao;
}

/* Generate VAO, VBOs and return VAO handle - Common Color for all vertices */
struct VAO* create3DObject (GLenum primitive_mode, int numVertices, const GLfloat* vertex_buffer_data, const GLfloat red, const GLfloat green, const GLfloat blue, GLenum fill_mode=GL_FILL)
{
    GLfloat* color_buffer_data = new GLfloat [3*numVertices];
    for (int i=0; i<numVertices; i++) {
        color_buffer_data [3*i] = red;
        color_buffer_data [3*i + 1] = green;
        color_buffer_data [3*i + 2] = blue;
    }

    return create3DObject(primitive_mode, numVertices, vertex_buffer_data, color_buffer_data, fill_mode);
}

struct VAO* create3DTexturedObject (GLenum primitive_mode, int numVertices, const GLfloat* vertex_buffer_data, const GLfloat* texture_buffer_data, GLuint textureID, GLenum fill_mode=GL_FILL)
{
  struct VAO* vao = new struct VAO;
  vao->PrimitiveMode = primitive_mode;
  vao->NumVertices = numVertices;
  vao->FillMode = fill_mode;
  vao->TextureID = textureID;

  // Create Vertex Array Object
  // Should be done after CreateWindow and before any other GL calls
  glGenVertexArrays(1, &(vao->VertexArrayID)); // VAO
  glGenBuffers (1, &(vao->VertexBuffer)); // VBO - vertices
  glGenBuffers (1, &(vao->TextureBuffer));  // VBO - textures

  glBindVertexArray (vao->VertexArrayID); // Bind the VAO
  glBindBuffer (GL_ARRAY_BUFFER, vao->VertexBuffer); // Bind the VBO vertices
  glBufferData (GL_ARRAY_BUFFER, 3*numVertices*sizeof(GLfloat), vertex_buffer_data, GL_STATIC_DRAW); // Copy the vertices into VBO
  glVertexAttribPointer(
              0,                  // attribute 0. Vertices
              3,                  // size (x,y,z)
              GL_FLOAT,           // type
              GL_FALSE,           // normalized?
              0,                  // stride
              (void*)0            // array buffer offset
              );

  glBindBuffer (GL_ARRAY_BUFFER, vao->TextureBuffer); // Bind the VBO textures
  glBufferData (GL_ARRAY_BUFFER, 2*numVertices*sizeof(GLfloat), texture_buffer_data, GL_STATIC_DRAW);  // Copy the vertex colors
  glVertexAttribPointer(
              2,                  // attribute 2. Textures
              2,                  // size (s,t)
              GL_FLOAT,           // type
              GL_FALSE,           // normalized?
              0,                  // stride
              (void*)0            // array buffer offset
              );

  return vao;
}


/* Render the VBOs handled by VAO */
void draw3DObject (struct VAO* vao)
{
    // Change the Fill Mode for this object
    glPolygonMode (GL_FRONT_AND_BACK, vao->FillMode);

    // Bind the VAO to use
    glBindVertexArray (vao->VertexArrayID);

    // Enable Vertex Attribute 0 - 3d Vertices
    glEnableVertexAttribArray(0);
    // Bind the VBO to use
    glBindBuffer(GL_ARRAY_BUFFER, vao->VertexBuffer);

    // Enable Vertex Attribute 1 - Color
    glEnableVertexAttribArray(1);
    // Bind the VBO to use
    glBindBuffer(GL_ARRAY_BUFFER, vao->ColorBuffer);

    // Draw the geometry !
    glDrawArrays(vao->PrimitiveMode, 0, vao->NumVertices); // Starting from vertex 0; 3 vertices total -> 1 triangle
}

void draw3DTexturedObject (struct VAO* vao)
{
  // Change the Fill Mode for this object
  glPolygonMode (GL_FRONT_AND_BACK, vao->FillMode);

  // Bind the VAO to use
  glBindVertexArray (vao->VertexArrayID);

  // Enable Vertex Attribute 0 - 3d Vertices
  glEnableVertexAttribArray(0);
  // Bind the VBO to use
  glBindBuffer(GL_ARRAY_BUFFER, vao->VertexBuffer);

  // Bind Textures using texture units
  glBindTexture(GL_TEXTURE_2D, vao->TextureID);

  // Enable Vertex Attribute 2 - Texture
  glEnableVertexAttribArray(2);
  // Bind the VBO to use
  glBindBuffer(GL_ARRAY_BUFFER, vao->TextureBuffer);

  // Draw the geometry !
  glDrawArrays(vao->PrimitiveMode, 0, vao->NumVertices); // Starting from vertex 0; 3 vertices total -> 1 triangle

  // Unbind Textures to be safe
  glBindTexture(GL_TEXTURE_2D, 0);
}

GLuint createTexture (const char* filename)
{
  GLuint TextureID;
  // Generate Texture Buffer
  glGenTextures(1, &TextureID);
  // All upcoming GL_TEXTURE_2D operations now have effect on our texture buffer
  glBindTexture(GL_TEXTURE_2D, TextureID);
  // Set our texture parameters
  // Set texture wrapping to GL_REPEAT
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  // Set texture filtering (interpolation)
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  // Load image and create OpenGL texture
  int twidth, theight;
  unsigned char* image = SOIL_load_image(filename, &twidth, &theight, 0, SOIL_LOAD_RGB);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, twidth, theight, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
  glGenerateMipmap(GL_TEXTURE_2D); // Generate MipMaps to use
  SOIL_free_image_data(image); // Free the data read from file after creating opengl texture
  glBindTexture(GL_TEXTURE_2D, 0); // Unbind texture when done, so we won't accidentily mess it up

  return TextureID;
}


/**************************
 * Customizable functions *
 **************************/

float triangle_rot_dir = 1;
float rectangle_rot_dir = 1;
bool triangle_rot_status = true;
bool rectangle_rot_status = true;

/* Executed when a regular key is pressed/released/held-down */
/* Prefered for Keyboard events */

void refreshValues()
{
   projectile_x_coordinate=-3,projectile_y_coordinate=-2,projectile_velocity=0,projectile_angle=0;
   projectile_x_velocity=0,projectile_y_velocity=0;
   flag=0;

}
VAO *speedbar;
void createSpeedbar()
{
    
    
  GLfloat vertex_buffer_data [] = {
        -3.5,0.5,0, // vertex 1
        -3.5,0,0, // vertex 2
        (-3.3 + projectile_velocity/3),-0,0, // vertex 3

       -3.5, 0.5,0, // vertex 1
        (projectile_velocity/2 - 3.3), 0.5,0, // vertex 4
        (-3.3 + projectile_velocity/3),0,0, // vertex 3
      
    };

    GLfloat color_buffer_data [] = {
        (1-(projectile_velocity/2.0)),1,1, // color 1
        (1-(projectile_velocity/2.0)),1,0, // color 2
        1,(projectile_velocity/2.0),0, // color 3
        (1-(projectile_velocity/2.0)),1,1, // color 1
        1,(projectile_velocity/2.0),0, // color 4
        1,(projectile_velocity/2.0),0 // color 3
    

  /*      1,1,1,
        1,1,1,
        1,1,1,
        1,1,1,
        1,1,1,
        1,1,1,
      */
    };
    speedbar = create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
}

void changeOrtho(int temp)  // if temp ==1 then zoom in else zoom out
{
  if (temp==1)
  {
    ortho_x_min += 0.5;
    ortho_y_min += 0.5;
    ortho_x_max -= 0.5;
    ortho_y_max -= 0.5;

  }
  else if (temp==2)
  {
    ortho_x_min -= 0.5;
    ortho_y_min -= 0.5;
    ortho_x_max += 0.5;
    ortho_y_max += 0.5;
  }
  else if (temp==3)
  {
    ortho_x_min -= 0.5;
    ortho_y_min -= 0.5;
    ortho_x_max -= 0.5;
    ortho_y_max -= 0.5;
  }
  else if (temp==4)
  {
    ortho_x_min += 0.5;
    ortho_y_min += 0.5;
    ortho_x_max += 0.5;
    ortho_y_max += 0.5;
  }



  if (ortho_y_max <= ortho_y_min)
  {
    ortho_y_max += 0.5;
    ortho_y_min -= 0.5;
    ortho_x_max += 0.5;
    ortho_x_min -= 0.5;
  }

}

void keyboard (GLFWwindow* window, int key, int scancode, int action, int mods)
{
     // Function is called first on GLFW_PRESS.

    if (action == GLFW_RELEASE) {
        switch (key) {
            case GLFW_KEY_UP:
                changeOrtho(1);             // 1 for zoom_in
                break;
            case GLFW_KEY_DOWN:
                changeOrtho(2);             // 1 for zoom_in
                break;
            case GLFW_KEY_LEFT:
                changeOrtho(3);  
                projectile_angle += 5;
                break;
            case GLFW_KEY_RIGHT:
                changeOrtho(4);
                projectile_angle -= 5;
                break;  
            case GLFW_KEY_F:
                rectangle_rot_status = !rectangle_rot_status;
                projectile_velocity += 1.5;
                createSpeedbar();
                break;
            case GLFW_KEY_S:
                triangle_rot_status = !triangle_rot_status;
                projectile_velocity -= 1;
                createSpeedbar();
                break;
            case GLFW_KEY_SPACE:
                if (flag!=1)
                {
                  projectile_x_velocity = (projectile_velocity*cos(projectile_angle*3.14/180));
                  projectile_y_velocity = (projectile_velocity*sin(projectile_angle*3.14/180));
                  flag=1;
                }
                // do something ..
                 break;
            case GLFW_KEY_R:
                refreshValues();
                break;  
            default:
                break;

        }
    }
    else if (action == GLFW_PRESS) {
        switch (key) {
            case GLFW_KEY_ESCAPE:
                quit(window);
                break;
            default:
                break;
        }
    }
}

/* Executed for character input (like in text boxes) */
void keyboardChar (GLFWwindow* window, unsigned int key)
{
	switch (key) {
		case 'Q':
		case 'q':
            quit(window);
            break;
		default:
			break;
	}
}

/* Executed when a mouse button is pressed/released */
void mouseButton (GLFWwindow* window, int button, int action, int mods)
{
    switch (button) {
        case GLFW_MOUSE_BUTTON_LEFT:
            if (action == GLFW_RELEASE)
                if (flag!=1)
                {
                  projectile_x_velocity = (projectile_velocity*cos(projectile_angle*3.14/180));
                  projectile_y_velocity = (projectile_velocity*sin(projectile_angle*3.14/180));
                  flag=1;
                }
                // do something ..
                 break;
                triangle_rot_dir *= -1;
            break;
        case GLFW_MOUSE_BUTTON_RIGHT:
            if (action == GLFW_RELEASE) {
                rectangle_rot_dir *= -1;
            }
            break;
        default:
            break;
    }

}


/* Executed when window is resized to 'width' and 'height' */
/* Modify the bounds of the screen here in glm::ortho or Field of View in glm::Perspective */
void reshapeWindow (GLFWwindow* window, int width, int height)
{
    int fbwidth=width, fbheight=height;
    /* With Retina display on Mac OS X, GLFW's FramebufferSize
     is different from WindowSize */
    glfwGetFramebufferSize(window, &fbwidth, &fbheight);

	GLfloat fov = 90.0f;

	// sets the viewport of openGL renderer
	glViewport (0, 0, (GLsizei) fbwidth, (GLsizei) fbheight);

	// set the projection matrix as perspective
	/* glMatrixMode (GL_PROJECTION);
	   glLoadIdentity ();
	   gluPerspective (fov, (GLfloat) fbwidth / (GLfloat) fbheight, 0.1, 500.0); */
	// Store the projection matrix in a variable for future use
    // Perspective projection for 3D views
    // Matrices.projection = glm::perspective (fov, (GLfloat) fbwidth / (GLfloat) fbheight, 0.1f, 500.0f);

    // Ortho projection for 2D views
    Matrices.projection = glm::ortho(ortho_y_min, ortho_x_max, ortho_y_min, ortho_y_max, 0.1f, 500.0f);
}

VAO *triangle, *rectangle, *circle, *cannon, *cannonrect;
VAO *rectangle1,*rectangle2,*rectangle3,*rectangle4, *barrier1 , *barrier2;

// Creates the triangle object used in this sample code
void createTriangle ()
{
  /* ONLY vertices between the bounds specified in glm::ortho will be visible on screen */

  /* Define vertex array as used in glBegin (GL_TRIANGLES) */
  static const GLfloat vertex_buffer_data [] = {
    0, 1,0, // vertex 0
    -1,-1,0, // vertex 1
    1,-1,0, // vertex 2
  };

  static const GLfloat color_buffer_data [] = {
    1,0,0, // color 0
    0,1,0, // color 1
    0,0,1, // color 2
  };

  // create3DObject creates and returns a handle to a VAO that can be used later
  triangle = create3DObject(GL_TRIANGLES, 3, vertex_buffer_data, color_buffer_data, GL_LINE);
}

// Creates the rectangle object used in this sample code
void createRectangle (int temp)
{
  // GL3 accepts only Triangles. Quads are not supported
  static const GLfloat vertex_buffer_data [] = {
    -0.2,-0.2,0, // vertex 1
    -0.2,0.2,0, // vertex 2
    0.2, 0.2,0, // vertex 3

    0.2, 0.2,0, // vertex 3
    0.2, -0.2,0, // vertex 4
    -0.2,-0.2,0  // vertex 1
  };

  static const GLfloat color_buffer_data [] = {
    0,0,0, // color 1
    0,0,0, // color 2
    0,0,0, // color 3

    0,0,0, // color 3
    0,0,0, // color 4
    0,0,0  // color 1
  };

  array_collisions[temp].radius=0.28;  // radius = 4*2^(1/2)
  // create3DObject creates and returns a handle to a VAO that can be used later
  if (temp==1)
    rectangle1 = create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
  if (temp==2)
    rectangle2 = create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
  if (temp==3)
    rectangle3 = create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
  if (temp==4)
    rectangle4 = create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);

}

void createCircle()
{
      int i,k=0;
      GLfloat vertex_buffer_data[1090]={};
      GLfloat color_buffer_data[1090]={};
      for (i = 0; i < 360; ++i)
      {
       vertex_buffer_data[k] = 0.1*cos(i*3.14/180);
       color_buffer_data[k] = ((double) rand() / (RAND_MAX)) +1;
       // cout << ((double) rand() / (RAND_MAX)) << endl;

       k++;
       vertex_buffer_data[k] = 0.1*sin(i*3.14/180);
       color_buffer_data[k] = ((double) rand() / (RAND_MAX)) +1;
       k++;
       vertex_buffer_data[k] = 0;
       color_buffer_data[k] = ((double) rand() / (RAND_MAX));
       k++;
     }

     circle = create3DObject(GL_TRIANGLE_FAN, 360, vertex_buffer_data, color_buffer_data, GL_FILL);
}

void createCannon()
{
      int i,k=0;
      GLfloat vertex_buffer_data[1090]={};
      GLfloat color_buffer_data[1090]={};
      for (i = 0; i < 360; ++i)
      {
       vertex_buffer_data[k] = 0.2*cos(i*3.14/180);
       color_buffer_data[k] = 1;//((double) rand() / (RAND_MAX)) +1;
       // cout << ((double) rand() / (RAND_MAX)) << endl;

       k++;
       vertex_buffer_data[k] = 0.2*sin(i*3.14/180);
       color_buffer_data[k] = 1;//((double) rand() / (RAND_MAX)) +1;
       k++;
       vertex_buffer_data[k] = 0;
       color_buffer_data[k] = 1;//((double) rand() / (RAND_MAX));
       k++;
     }

     cannon = create3DObject(GL_TRIANGLE_FAN, 360, vertex_buffer_data, color_buffer_data, GL_FILL);
}

void createCannonRectangle ()
{
  // GL3 accepts only Triangles. Quads are not supported
  static const GLfloat vertex_buffer_data [] = {
    0.2,0,0, // vertex 1  
    0.3,0,0, // vertex 2
    0.3, 0.04,0, // vertex 3

    0.3, 0.04,0, // vertex 3
    0.2, 0.04,0, // vertex 4
    0.2,0,0  // vertex 1
  };

  static const GLfloat color_buffer_data [] = {
    0,0,0, // color 1
    0,0,0, // color 2
    0,0,0, // color 3

    0,0,0, // color 3
    0,0,0, // color 4
    0,0,0  // color 1
  };
  cannonrect = create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
}

void createBarrier (int temp)
{
  // GL3 accepts only Triangles. Quads are not supported
   static const GLfloat vertex_buffer_data [] = {
    -0.2,-1.7,0, // vertex 1
    0.2,-1.7,0, // vertex 2
    0.2, 1.5,0, // vertex 3

    0.2, 1.5,0, // vertex 3
    -0.2, 1.5,0, // vertex 4
    -0.2,-1.7,0  // vertex 1
  };

  static const GLfloat color_buffer_data [] = {
    0.5,0.5,0.5, // color 1
    0.3,0.3,0.3, // color 2
    0.2,0.2,0.2, // color 3

    0.2,0.2,0.2, // color 3
    0.1,0.1,0.1, // color 4
    0.5,0.5,0.5, // color 1
  };


  //array_collisions[temp].radius=0.28;  // radius = 4*2^(1/2)
  // create3DObject creates and returns a handle to a VAO that can be used later
    barrier1 = create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
  /*
  if (temp==3)
    rectangle3 = create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
  if (temp==4)
    rectangle4 = create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
  */
}

void createBarrier2 (int temp)
{
  // GL3 accepts only Triangles. Quads are not supported
static const    GLfloat vertex_buffer_data [] = {
      -0.2,-1.2,0, // vertex 1
      0.2,-1.2,0, // vertex 2
      0.2, 1.0,0, // vertex 3

      0.2, 1,0, // vertex 3
      -0.2, 1,0, // vertex 4
      -0.2,-1.2,0  // vertex 1
    };

  static const GLfloat color_buffer_data [] = {
    0.5,0.5,0.5, // color 1
    0.3,0.3,0.3, // color 2
    0.2,0.2,0.2, // color 3

    0.2,0.2,0.2, // color 3
    0.1,0.1,0.1, // color 4
    0.5,0.5,0.5, // color 1
  };

  //array_collisions[temp].radius=0.28;  // radius = 4*2^(1/2)
  // create3DObject creates and returns a handle to a VAO that can be used later
  if (temp==2)
    barrier2 = create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
  /*
  if (temp==3)
    rectangle3 = create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
  if (temp==4)
    rectangle4 = create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
  */
}

float camera_rotation_angle = 90;
float rectangle_rotation = 0;
float triangle_rotation = 0;

  

/* Render the scene with openGL */
/* Edit this function according to your assignment */

void checkCollisionBarrier()
{
  //-1,-0.5
  if (projectile_x_coordinate > -1.3 && projectile_x_coordinate < 0.9)
    if (projectile_y_coordinate > -2 && projectile_y_coordinate < 1.1)
    {
     // projectile_y_velocity = -projectile_y_velocity*bounce;
      projectile_x_velocity = -projectile_x_velocity * bounce;
    }


//1,-1
  if (projectile_x_coordinate > -0.7 && projectile_x_coordinate < 1.3)
    if (projectile_y_coordinate > -1.1 && projectile_y_coordinate < 0.1)
    {
     // projectile_y_velocity = -projectile_y_velocity*bounce;
      projectile_x_velocity = -projectile_x_velocity * bounce;
    }

}


int checkCollision(int temp)
{
  double x_distance = pow( (projectile_x_coordinate - array_collisions[temp].x_coordinate), 2);
  double y_distance = pow( (projectile_y_coordinate - array_collisions[temp].y_coordinate), 2);

  if ( sqrt(x_distance + y_distance) < (0.28 + 0.1) )
    return 1;
  else
    return 0;

}
int f1=0,f2=0,f3=0,f4=0;

void draw ()
{
  // clear the color and depth in the frame buffer
  glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // use the loaded shader program
  // Don't change unless you know what you are doing
  glUseProgram (programID);

  // Eye - Location of camera. Don't change unless you are sure!!
  glm::vec3 eye ( 5*cos(camera_rotation_angle*M_PI/180.0f), 0, 5*sin(camera_rotation_angle*M_PI/180.0f) );
  // Target - Where is the camera looking at.  Don't change unless you are sure!!
  glm::vec3 target (0, 0, 0);
  // Up - Up vector defines tilt of camera.  Don't change unless you are sure!!
  glm::vec3 up (0, 1, 0);

  // Compute Camera matrix (view)
  // Matrices.view = glm::lookAt( eye, target, up ); // Rotating Camera for 3D
  //  Don't change unless you are sure!!
  Matrices.view = glm::lookAt(glm::vec3(0,0,3), glm::vec3(0,0,0), glm::vec3(0,1,0)); // Fixed camera for 2D (ortho) in XY plane

  // Compute ViewProject matrix as view/camera might not be changed for this frame (basic scenario)
  //  Don't change unless you are sure!!
  glm::mat4 VP = Matrices.projection * Matrices.view;

  // Send our transformation to the currently bound shader, in the "MVP" uniform
  // For each model you render, since the MVP will be different (at least the M part)
  //  Don't change unless you are sure!!
  glm::mat4 MVP;	// MVP = Projection * View * Model

  // Load identity to model matrix
//  Matrices.model = glm::mat4(1.0f);

  /* Render your scene */

//  glm::mat4 translateTriangle = glm::translate (glm::vec3(-2.0f, 0.0f, 0.0f)); // glTranslatef
//  glm::mat4 rotateTriangle = glm::rotate((float)(triangle_rotation*M_PI/180.0f), glm::vec3(0,0,1));  // rotate about vector (1,0,0)
//  glm::mat4 triangleTransform = translateTriangle * rotateTriangle;
//  Matrices.model *= triangleTransform; 
//  MVP = VP * Matrices.model; // MVP = p * V * M

  //  Don't change unless you are sure!!
//  glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

  // draw3DObject draws the VAO given to it using current MVP matrix
//  draw3DObject(triangle);

  // Pop matrix to undo transformations till last push matrix instead of recomputing model matrix
  // glPopMatrix ();
  

  Matrices.model = glm::mat4(1.0f);

  glm::mat4 translateRectangle1 = glm::translate (glm::vec3(0, 0, 0));        // glTranslatef
  array_collisions[1].x_coordinate=0; array_collisions[1].y_coordinate=0;
  if (checkCollision(1) || f1==1)
    { 
      f1=1;
      // cout<<"aaa"<<endl;
      translateRectangle1 = glm::translate (glm::vec3(100, 100, 0));
    }
  glm::mat4 rotateRectangle1 = glm::rotate((float)(rectangle_rotation*M_PI/180.0f), glm::vec3(0,0,1)); // rotate about vector (-1,1,1)
  Matrices.model *= (translateRectangle1 * rotateRectangle1);
  MVP = VP * Matrices.model;
  glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

  // draw3DObject draws the VAO given to it using current MVP matrix
  draw3DObject(rectangle1);

  Matrices.model = glm::mat4(1.0f);
  glm::mat4 translateRectangle2 = glm::translate (glm::vec3(0, 2, 0));        // glTranslatef
  array_collisions[2].x_coordinate=0; array_collisions[2].y_coordinate=2;
  if (checkCollision(2) || f2==1)
  {
    f2=1;
    translateRectangle2 = glm::translate (glm::vec3(100, 100, 0));
  }
  glm::mat4 rotateRectangle2 = glm::rotate((float)(rectangle_rotation*M_PI/180.0f), glm::vec3(0,0,1)); // rotate about vector (-1,1,1)
  Matrices.model *= (translateRectangle2 * rotateRectangle2);
  MVP = VP * Matrices.model;
  glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
  draw3DObject(rectangle2);

  Matrices.model = glm::mat4(1.0f);
  glm::mat4 translateRectangle3 = glm::translate (glm::vec3(-1, 3, 0));        // glTranslatef
  array_collisions[3].x_coordinate=-1; array_collisions[3].y_coordinate=3;
  if (checkCollision(3) || f3==1)
  {
    f3=1;
    translateRectangle3 = glm::translate (glm::vec3(100, 100, 0));
  }
  glm::mat4 rotateRectangle3 = glm::rotate((float)(rectangle_rotation*M_PI/180.0f), glm::vec3(0,0,1)); // rotate about vector (-1,1,1)
  Matrices.model *= (translateRectangle3 * rotateRectangle3);
  MVP = VP * Matrices.model;
  glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
  draw3DObject(rectangle3);

  Matrices.model = glm::mat4(1.0f);
  glm::mat4 translateRectangle4 = glm::translate (glm::vec3(2.5, -1, 0));        // glTranslatef
  array_collisions[4].x_coordinate=2.5; array_collisions[4].y_coordinate=-1;
  if (checkCollision(4) || f4==1)
  {
    f4=1;
    translateRectangle4 = glm::translate (glm::vec3(100, 100, 0));
  }
  glm::mat4 rotateRectangle4 = glm::rotate((float)(rectangle_rotation*M_PI/180.0f), glm::vec3(0,0,1)); // rotate about vector (-1,1,1)
  Matrices.model *= (translateRectangle4 * rotateRectangle4);
  MVP = VP * Matrices.model;
  glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
  draw3DObject(rectangle4);


  Matrices.model = glm::mat4(1.0f);
  glm::mat4 translateSpeedbar = glm::translate (glm::vec3(0, -4, 0));        // glTranslatef
 // glm::mat4 rotateSpeedbar = glm::rotate((float)(rectangle_rotation*M_PI/180.0f), glm::vec3(0,0,1)); // rotate about vector (-1,1,1)
  Matrices.model *= translateSpeedbar; //* rotateSpeedbar);
  MVP = VP * Matrices.model;
  glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
  draw3DObject(speedbar);



/*
  // Increment angles
  float increments = 1;

  //camera_rotation_angle++; // Simulating camera rotation
  triangle_rotation = triangle_rotation + increments*triangle_rot_dir*triangle_rot_status;
  rectangle_rotation = rectangle_rotation + increments*rectangle_rot_dir*rectangle_rot_status;
*/


  Matrices.model = glm::mat4(1.0f);
  glm::mat4 translateProjectile = glm::translate (glm::vec3(projectile_x_coordinate, projectile_y_coordinate, 0));
  Matrices.model *= translateProjectile;
  MVP = VP * Matrices.model;
  glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
  draw3DObject(circle);

  Matrices.model = glm::mat4(1.0f);
  glm::mat4 translatecannon = glm::translate (glm::vec3(-3,-2,0));
  Matrices.model *= translatecannon;
  MVP = VP * Matrices.model;
  glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
  draw3DObject(cannon);

  Matrices.model = glm::mat4(1.0f);
  glm::mat4 translaterectcannon = glm::translate (glm::vec3(-3, -2, 0));
  glm::mat4 rotateRectanglePro = glm::rotate((float)(projectile_angle*M_PI/180.0f), glm::vec3(0,0,1)); 
  Matrices.model *= (translaterectcannon*rotateRectanglePro);
  MVP = VP * Matrices.model;
  glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
  draw3DObject(cannonrect);

  Matrices.model = glm::mat4(1.0f);
  glm::mat4 translateBarrier1 = glm::translate (glm::vec3(-1, -0.5, 0));
  glm::mat4 rotateBarrier1 = glm::rotate((float)(0*projectile_angle*M_PI/180.0f), glm::vec3(0,0,1)); 
  Matrices.model *= (translateBarrier1*rotateBarrier1);
  MVP = VP * Matrices.model;
  glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
  draw3DObject(barrier1);

  Matrices.model = glm::mat4(1.0f);
  glm::mat4 translateBarrier2 = glm::translate (glm::vec3(1, -1, 0));
  glm::mat4 rotateBarrier2 = glm::rotate((float)(0*projectile_angle*M_PI/180.0f), glm::vec3(0,0,1)); 
  Matrices.model *= (translateBarrier2*rotateBarrier2);
  MVP = VP * Matrices.model;
  glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
  draw3DObject(barrier2);


}

void cursorPosCallback(GLFWwindow *window, double x_position,double y_position)
{
//   cout<<x_position<<" "<<y_position<<endl;

  double x_pos = x_position*(8.0f/600.0f);
  double y_pos = y_position*(8.0f/600.0f);

//cout<<"s "<<x_pos<<" "<<y_pos<<endl;
  y_pos *= -1;
  x_pos -= 1;
  y_pos += 6;

  //cout<<x_pos<<" "<<y_pos<<endl;

  // cout<< atan(y_pos/x_pos) * 180/3.14 << endl;
  if (flag == 0)
  {
    projectile_angle = atan(y_pos/x_pos) * 180/3.14;
    projectile_velocity = 0.8 * sqrt(pow( (x_pos - projectile_x_coordinate),2) + pow( (y_pos - projectile_y_coordinate), 2));  
  }
  createSpeedbar();

}

/* Initialise glfw window, I/O callbacks and the renderer to use */
/* Nothing to Edit here */
GLFWwindow* initGLFW (int width, int height)
{
    GLFWwindow* window; // window desciptor/handle

    glfwSetErrorCallback(error_callback);
    if (!glfwInit()) {
        exit(EXIT_FAILURE);
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(width, height, "Sample OpenGL 3.3 Application", NULL, NULL);

    if (!window) {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    glfwMakeContextCurrent(window);
    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
    glfwSwapInterval( 1 );

    /* --- register callbacks with GLFW --- */

    /* Register function to handle window resizes */
    /* With Retina display on Mac OS X GLFW's FramebufferSize
     is different from WindowSize */
    glfwSetFramebufferSizeCallback(window, reshapeWindow);
    glfwSetWindowSizeCallback(window, reshapeWindow);

    /* Register function to handle window close */
    glfwSetWindowCloseCallback(window, quit);

    /* Register function to handle keyboard input */
    glfwSetKeyCallback(window, keyboard);      // general keyboard input
    glfwSetCharCallback(window, keyboardChar);  // simpler specific character handling

    /* Register function to handle mouse click */
    glfwSetMouseButtonCallback(window, mouseButton);  // mouse button clicks

    // stuff i added
    glfwSetCursorPosCallback(window, cursorPosCallback);

    return window;
}

/* Initialize the OpenGL rendering properties */
/* Add all the models to be created here */
void initGL (GLFWwindow* window, int width, int height)
{
    /* Objects should be created before any other gl function and shaders */
	// Create the models
//	createTriangle (); // Generate the VAO, VBOs, vertices data & copy into the array buffer
  createRectangle (1);
  createRectangle (2);
  createRectangle (3);
	createRectangle (4);
  createSpeedbar();
	
  createCircle();
  createCannon();
  createCannonRectangle ();
  createBarrier(1);
  createBarrier2(2);
	// Create and compile our GLSL program from the shaders
	programID = LoadShaders( "Sample_GL.vert", "Sample_GL.frag" );
	// Get a handle for our "MVP" uniform
	Matrices.MatrixID = glGetUniformLocation(programID, "MVP");

	
	reshapeWindow (window, width, height);

    // Background color of the scene
	glClearColor (0.3f, 0.3f, 0.3f, 0.0f); // R, G, B, A
	glClearDepth (1.0f);

	glEnable (GL_DEPTH_TEST);
	glDepthFunc (GL_LEQUAL);

    cout << "VENDOR: " << glGetString(GL_VENDOR) << endl;
    cout << "RENDERER: " << glGetString(GL_RENDERER) << endl;
    cout << "VERSION: " << glGetString(GL_VERSION) << endl;
    cout << "GLSL: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;
}

int f[5]={};
void changeXVelocity()
{

  int i;
  for (i=1;i<5;i++)
  {
   if ( checkCollision(i)==1 )
    {
      if (f[i] != 1)
      {
        array_collisions[i].x_velocity=projectile_x_velocity*(1/3); // conservation of momentum area of circle =0.35 area of rect = 0.16
                                                                   // also accounts for energy lost during collision


        projectile_x_velocity=-projectile_x_velocity*0.8;
        f[i]=1;
        break;
      }
   }  
  }
}

int main (int argc, char** argv)
{
	int width = 600;
	int height = 600;

    GLFWwindow* window = initGLFW(width, height);

	initGL (window, width, height);

    double last_update_time = glfwGetTime(), current_time;

    /* Draw in loop */
    while (!glfwWindowShouldClose(window)) {
    reshapeWindow (window, width, height);

        // OpenGL Draw commands
        draw();

        // Swap Frame Buffer in double buffering
        glfwSwapBuffers(window);

        // Poll for Keyboard and mouse events
        glfwPollEvents();

        // Control based on time (Time based transformation like 5 degrees rotation every 0.5s)
        current_time = glfwGetTime(); // Time in seconds
        double delay=0.01;
        if ((current_time - last_update_time) >= delay) { // atleast 0.5s elapsed since last frame
            // do something every 0.5 seconds ..
            if (flag==1){
              projectile_x_velocity *= air_resistance;
              projectile_y_velocity -= gravity;
            }

            if (projectile_y_coordinate < -2)
              projectile_y_velocity =- projectile_y_velocity*bounce; 

            checkCollisionBarrier();
            changeXVelocity();

            projectile_x_coordinate += (projectile_x_velocity * delay);
            projectile_y_coordinate += (projectile_y_velocity * delay);  

          
            last_update_time = current_time;
        }
    }

    glfwTerminate();
    exit(EXIT_SUCCESS);
}
