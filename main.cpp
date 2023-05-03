const int FOO = 1;
//include <exception>
#include <fstream>
#include <glm/fwd.hpp>
#include <glm/matrix.hpp>
#include <iostream>
#include "GL/glew.h"
#include "GLFW/glfw3.h"
#include "glm/glm.hpp"
#include "glm/gtx/transform.hpp"
#include <chrono>
#include <sstream>
using namespace std;

GLFWwindow *window = NULL;

const int IDIFFDENS = 1;
const int IDIFFVEL = 5;
const int IPROJ1 = 140;
const int IPROJ2 = 140;
//const float DENS_LOSS = 0.99999;//multiplicateur
const float DENS_LOSS = 1.001;//multiplicateur
//const float VEL_LOSS = 0.999999;//multiplicateur
const float VEL_LOSS = 1.0;//multiplicateur
const float DENS_VISC = 0.000001;
const float VEL_VISC = 0.9;
const float NON_LINEAR_RENDER = 1.1;
//static float COLOR[4] = {1.0, 1.0, 1.0, 1.0};//RGBA - couleur qu'on ajout au click
//static float COLOR[4] = {0.9, 0.9, 0.9, 1.0};//RGBA - couleur qu'on ajout au click
static float COLOR[4] = {0.8, 0.6, 0.8, 1.0};//RGBA - couleur qu'on ajout au click
static float COLOR_CHANGE_VIT = 8.5;
static bool BOOL_CHANGE_COLOR = 1;

const int WINDOW_WIDTH = 800;
//const int WINDOW_WIDTH = 704;
const int WINDOW_HEIGHT = WINDOW_WIDTH;
static unsigned int _dim_dispatch[2] = {WINDOW_WIDTH / 32, WINDOW_HEIGHT / 32};

static GLuint _ntex_dens = 0;
static GLuint _ntex_vel = 0;
static GLuint _ntex_p = 0;
static GLuint _rend_vert_shader, _rend_frag_shader;
static GLuint _advect_cs, _diff_cs, _dens_add_cs, _vel_add_cs;
static GLuint _project_1_cs, _project_2_cs, _project_3_cs;
static GLuint _dens_add_program, _vel_add_program;
static GLuint _project_1_program, _project_2_program, _project_3_program;
static GLuint _diff_program, _advect_program;
static GLuint _render_program;
//static GLuint _vao = 0;
static GLuint _fbo = 0;
static GLuint _fboTexDens[2] = {0, 0};
static GLuint _fboTexVel[2] = {0, 0};
static GLuint _fboTexProjectP[2] = {0, 0};
static GLuint _fboTexProjectDiv[1] = {0};
//static GLuint _quad = 0;
static GLuint _quad_vbo = 0;
static GLuint _quad_vao = 0;
//static GLuint _buffer[3] = {0};

static float _dt, _last_time;
static float _seconde = 0.0;
static int _fps = 0;

static double _xy_mouse_double[2] = {90.0f, 90.0f};
static GLfloat _xy_mouse_float[2] = {90.0f, 90.0f};
static int _xy_mouse_int[2] = {0, 0};
static int _clickstate = 0;
static int _addmotion1[2] = {0, 0};
static int _addmotion2[2] = {0, 0};
static GLfloat _motion[2] = {0.0, 0.0};

std::string read(const char *filename) {
  std::stringbuf source;
  std::ifstream in(filename);
  // verifie que le fichier existe
  if (in.good() == false)
    // affiche une erreur, si le fichier n'existe pas ou n'est pas accessible
    cout << "[error] loading program : " << filename << endl;
  else
    printf("loading program '%s'...\n", filename);
  // lire le fichier, jusqu'au premier separateur,
  // le caractere '\0' ne peut pas se trouver dans un fichier texte, donc lit
  // tout le fichier d'un seul coup
  in.get(source, 0);
  // renvoyer la chaine de caracteres
  return source.str();
}

void mouse_button_callback(GLFWwindow *window, int button, int action,
                           int mods) {
  glfwGetCursorPos(window, &_xy_mouse_double[0], &_xy_mouse_double[1]);
  // if(button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS){
  if (button == GLFW_MOUSE_BUTTON_LEFT) {
    _clickstate = (_clickstate + 1) % 2;
    glfwGetCursorPos(window, &_xy_mouse_double[0], &_xy_mouse_double[1]);
    if (_clickstate == 1) {
      _addmotion1[0] = (int)_xy_mouse_double[0];
      _addmotion1[1] = (int)_xy_mouse_double[1];
      _addmotion2[0] = (int)_xy_mouse_double[0];
      _addmotion2[1] = (int)_xy_mouse_double[1];
    } else {
      _motion[0] = 0.0;
      _motion[1] = 0.0;
    }
  }
}

void cursor_position_callback(GLFWwindow* window, double xpos, double ypos){
  //cout << "clickstate = " << _clickstate << endl;
  _addmotion1[0] = _addmotion2[0];
  _addmotion1[1] = _addmotion2[1];
  _addmotion2[0] = (int) xpos;
  _addmotion2[1] = (int) ypos;
  _motion[0] = (GLfloat)xpos - (GLfloat) _addmotion1[0];
  _motion[1] = (GLfloat)ypos - (GLfloat) _addmotion1[1];
  _motion[1] *= -1.0;
  //cout << "x motion = " << _motion[0] << "  - y motion = " << _motion[1] << endl;
}

void printWorkGroupsCapabilities() {
  GLint workgroup_count[3] = {0};
  GLint workgroup_size[3];
  GLint workgroup_invocations;
  glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &workgroup_count[0]);
  glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &workgroup_count[1]);
  glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &workgroup_count[2]);
  printf("Taille maximale des workgroups:\n\tx:%u\n\ty:%u\n\tz:%u\n",
         workgroup_size[0], workgroup_size[1], workgroup_size[2]);
  glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &workgroup_size[0]);
  glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &workgroup_size[1]);
  glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &workgroup_size[2]);
  printf("Nombre maximal d'invocation locale:\n\tx:%u\n\ty:%u\n\tz:%u\n",
         workgroup_size[0], workgroup_size[1], workgroup_size[2]);
  glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &workgroup_invocations);
  printf("Nombre maximum d'invocation de workgroups:\n\t%u\n",
         workgroup_invocations);
}

void compil_shader(const char *filename, GLuint *id_shader,
                   GLuint shader_type) {
  GLint status;
  std::string source = read(filename);
  // cout << source << endl;
  const char *strings[] = {source.c_str()};
  switch (shader_type) {
  case 0:
    (*id_shader) = glCreateShader(GL_VERTEX_SHADER);
    break;
  case 1:
    (*id_shader) = glCreateShader(GL_GEOMETRY_SHADER);
    break;
  case 2:
    (*id_shader) = glCreateShader(GL_FRAGMENT_SHADER);
    break;
  case 3:
    (*id_shader) = glCreateShader(GL_COMPUTE_SHADER);
    break;
  default:
    break;
  }
  glShaderSource(*id_shader, 1, strings, NULL);
  glCompileShader(*id_shader);
  glGetShaderiv(*id_shader, GL_COMPILE_STATUS, &status);
  if (status == GL_TRUE) {
    cout << "compilation du program : " << filename << " -> ok" << endl;
  } else {
    cout << "*********************************** ERROR : erreur de compilation de : " << filename << endl;
    GLsizei length;
    glGetShaderiv(*id_shader, GL_INFO_LOG_LENGTH, &length);
    char *message = new char[1024];
    glGetShaderInfoLog(*id_shader, 1024, &length, message);
    cout << "erreurs de compilation :" << endl << message << endl;
    delete message;
  }
}

void check_attach_and_link(GLuint program) {
  GLint status;
  glGetProgramiv(program, GL_LINK_STATUS, &status);
  if (status == GL_TRUE) {
    cout << "Attach and Link program : ok " << endl;
  } else {
    // GLsizei length;
    GLchar errorLog[512];
    glGetProgramInfoLog(program, 512, NULL, errorLog);
    cout << "*************************************** ERROR : Attach and link program" << errorLog << endl;
  }
}

void init() {
  GLuint * tex = (GLuint*) malloc(WINDOW_WIDTH * WINDOW_HEIGHT * sizeof *tex);
  assert(tex);
  /* remplir tex de noir*/
  for(int i = 0; i < WINDOW_WIDTH * WINDOW_HEIGHT; ++i){
    tex[i] = 150 << 24; // ABGR
  }

  glGenFramebuffers(1, &_fbo);

  glGenTextures(2, _fboTexDens);
  glGenTextures(2, _fboTexVel);
  glGenTextures(2, _fboTexProjectP);
  glGenTextures(1, _fboTexProjectDiv);
  for(int i = 0; i < 2; ++i) {
    glBindTexture(GL_TEXTURE_2D, _fboTexDens[i]);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, WINDOW_WIDTH, WINDOW_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
    //glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, WINDOW_WIDTH, WINDOW_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, tex);
  }
  for(int i = 0; i < 2; ++i) {
    glBindTexture(GL_TEXTURE_2D, _fboTexVel[i]);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, WINDOW_WIDTH, WINDOW_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
    //glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, WINDOW_WIDTH, WINDOW_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, tex);
  }
  for(int i = 0; i < 2; ++i) {
    glBindTexture(GL_TEXTURE_2D, _fboTexProjectP[i]);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, WINDOW_WIDTH, WINDOW_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
    //glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, WINDOW_WIDTH, WINDOW_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, tex);
  }
  glBindTexture(GL_TEXTURE_2D, _fboTexProjectDiv[0]);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, WINDOW_WIDTH, WINDOW_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);

  free(tex);
  glBindTexture(GL_TEXTURE_2D, 0);


  //ADVECT CS
  compil_shader("./shaders/advect.comp", &_advect_cs, 3);
  _advect_program = glCreateProgram();
  glAttachShader(_advect_program, _advect_cs);
  glLinkProgram(_advect_program);
  cout << "Check ADVECT program :" << endl;
  check_attach_and_link(_advect_program);

  //DIFF CS
  compil_shader("./shaders/diff.comp", &_diff_cs, 3);
  _diff_program = glCreateProgram();
  glAttachShader(_diff_program, _diff_cs);
  glLinkProgram(_diff_program);
  cout << "Check DIFF program :" << endl;
  check_attach_and_link(_diff_program);

  //DENS ADD CS
  compil_shader("./shaders/dens_add.comp", &_dens_add_cs, 3);
  _dens_add_program = glCreateProgram();
  glAttachShader(_dens_add_program, _dens_add_cs);
  glLinkProgram(_dens_add_program);
  cout << "Check DENS ADD program :" << endl;
  check_attach_and_link(_dens_add_program);

  //VEL ADD CS
  compil_shader("./shaders/vel_add.comp", &_vel_add_cs, 3);
  _vel_add_program = glCreateProgram();
  glAttachShader(_vel_add_program, _vel_add_cs);
  glLinkProgram(_vel_add_program);
  cout << "Check VEL ADD program :" << endl;
  check_attach_and_link(_vel_add_program);

  //PROJECT 1 CS
  compil_shader("./shaders/project_1.comp", &_project_1_cs, 3);
  _project_1_program = glCreateProgram();
  glAttachShader(_project_1_program, _project_1_cs);
  glLinkProgram(_project_1_program);
  cout << "Check PROJECT 1 program :" << endl;
  check_attach_and_link(_project_1_program);
  //PROJECT 2 CS
  compil_shader("./shaders/project_2.comp", &_project_2_cs, 3);
  _project_2_program = glCreateProgram();
  glAttachShader(_project_2_program, _project_2_cs);
  glLinkProgram(_project_2_program);
  cout << "Check PROJECT 2 program :" << endl;
  check_attach_and_link(_project_2_program);
  //PROJECT 3 CS
  compil_shader("./shaders/project_3.comp", &_project_3_cs, 3);
  _project_3_program = glCreateProgram();
  glAttachShader(_project_3_program, _project_3_cs);
  glLinkProgram(_project_3_program);
  cout << "Check PROJECT  3 program :" << endl;
  check_attach_and_link(_project_3_program);

  //RENDER SHADERS
  compil_shader("./shaders/basic.vert", &_rend_vert_shader, 0);
  compil_shader("./shaders/basic.frag", &_rend_frag_shader, 2);
  _render_program = glCreateProgram();
  glAttachShader(_render_program, _rend_vert_shader);
  glAttachShader(_render_program, _rend_frag_shader);
  glLinkProgram(_render_program);
  check_attach_and_link(_render_program);

  GLfloat quad[] ={
  -1.0f, 1.0f, 0.0f, 1.0f,//
  -1.0f, -1.0f, 0.0f, 0.0f,//
  1.0f, -1.0f, 1.0f,  0.0f,//
  -1.0f, 1.0f, 0.0f, 1.0f,//
  1.0f,  -1.0f, 1.0f,  0.0f,//
  1.0f, 1.0f,  1.0f,  1.0f//
};
  glGenBuffers(1, &_quad_vbo);
  glBindBuffer(GL_ARRAY_BUFFER, _quad_vbo);
  glGenVertexArrays(1, &_quad_vao);
  glBindVertexArray(_quad_vao);
  glBufferData(GL_ARRAY_BUFFER, sizeof(quad), &quad, GL_STATIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)(2 * sizeof(float)));

  glClearColor(0.9f, 0.0f, 0.0f, 1.0f);
  glEnable(GL_TEXTURE_2D);
  glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
}

static inline void dens_add() {
  //cout << "add source xy = " << _xy_mouse_int[0] << " - " << _xy_mouse_int[1] << endl;
  glUseProgram(_dens_add_program);
  glBindTexture(GL_TEXTURE_2D, _fboTexDens[(_ntex_dens) % 2]);
  glBindImageTexture(0, _fboTexDens[(_ntex_dens) % 2], 0, GL_FALSE, 0, GL_WRITE_ONLY,
                     GL_RGBA32F);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, _fboTexDens[(_ntex_dens + 1) % 2]);
  glUniform1i(glGetUniformLocation(_dens_add_program, "tex"), 0);
  glUniform2iv(glGetUniformLocation(_dens_add_program, "add_source"), 1, _addmotion1);
  glUniform1f(glGetUniformLocation(_dens_add_program, "dt"), _dt);
  glUniform1i(glGetUniformLocation(_dens_add_program, "w"), WINDOW_WIDTH);
  glUniform4fv(glGetUniformLocation(_dens_add_program, "color"), 1, COLOR);
  glDispatchCompute(_dim_dispatch[0], _dim_dispatch[1], 1);
  glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
  glBindImageTexture(0, 0, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
  glBindTexture(GL_TEXTURE_2D, 0);
  glUseProgram(0);
  ++_ntex_dens;
}

static inline void vel_add() {
  cout << "motion x = " << _motion[0] << "  - motion y = " << _motion[1] << endl;
  glUseProgram(_vel_add_program);
  glBindTexture(GL_TEXTURE_2D, _fboTexVel[(_ntex_vel) % 2]);
  glBindImageTexture(0, _fboTexVel[(_ntex_vel) % 2], 0, GL_FALSE, 0, GL_WRITE_ONLY,
                     GL_RGBA32F);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, _fboTexVel[(_ntex_vel + 1) % 2]);
  glUniform1i(glGetUniformLocation(_vel_add_program, "tex"), 0);
  glUniform2iv(glGetUniformLocation(_vel_add_program, "add_source"), 1, _addmotion1);
  glUniform2fv(glGetUniformLocation(_vel_add_program, "motion"), 1, _motion);
  glUniform1f(glGetUniformLocation(_vel_add_program, "dt"), _dt);
  glUniform1i(glGetUniformLocation(_vel_add_program, "w"), WINDOW_WIDTH);
  glDispatchCompute(_dim_dispatch[0], _dim_dispatch[1], 1);
  glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
  glBindImageTexture(0, 0, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
  glBindTexture(GL_TEXTURE_2D, 0);
  glUseProgram(0);
  ++_ntex_vel;
}

static inline void dens_diff(){
  for (int i = 0; i < IDIFFDENS ; ++i) {
    glUseProgram(_diff_program);
    glBindTexture(GL_TEXTURE_2D, _fboTexDens[(_ntex_dens) % 2]);
    glBindImageTexture(0, _fboTexDens[(_ntex_dens) % 2], 0, GL_FALSE, 0, GL_WRITE_ONLY,
                       GL_RGBA32F);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, _fboTexDens[(_ntex_dens + 1) % 2]);
    glUniform1i(glGetUniformLocation(_diff_program, "tex"), 0);
    glUniform1f(glGetUniformLocation(_diff_program, "dt"), _dt);
    glUniform1i(glGetUniformLocation(_diff_program, "b"), 0);
    glUniform1i(glGetUniformLocation(_diff_program, "w"), WINDOW_WIDTH);
    glUniform1f(glGetUniformLocation(_diff_program, "loss"), DENS_LOSS);
    glUniform1f(glGetUniformLocation(_diff_program, "diff"), DENS_VISC);
    glDispatchCompute(_dim_dispatch[0], _dim_dispatch[1], 1);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    glBindImageTexture(0, 0, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(0);
    ++_ntex_dens;
  }
}

static inline void vel_diff(){
  for (int i = 0; i < IDIFFVEL ; ++i) {
    glUseProgram(_diff_program);
    glBindTexture(GL_TEXTURE_2D, _fboTexVel[(_ntex_vel) % 2]);
    glBindImageTexture(0, _fboTexVel[(_ntex_vel) % 2], 0, GL_FALSE, 0, GL_WRITE_ONLY,
                       GL_RGBA32F);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, _fboTexVel[(_ntex_vel + 1) % 2]);
    glUniform1i(glGetUniformLocation(_diff_program, "tex"), 0);
    glUniform1f(glGetUniformLocation(_diff_program, "dt"), _dt);
    glUniform1i(glGetUniformLocation(_diff_program, "b"), 1);
    glUniform1i(glGetUniformLocation(_diff_program, "w"), WINDOW_WIDTH);
    glUniform1f(glGetUniformLocation(_diff_program, "loss"), VEL_LOSS);
    glUniform1f(glGetUniformLocation(_diff_program, "diff"), VEL_VISC);
    glDispatchCompute(_dim_dispatch[0], _dim_dispatch[1], 1);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    glBindImageTexture(0, 0, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(0);
    ++_ntex_vel;
  }
}

static inline void dens_advect(){
  glUseProgram(_advect_program);
  glBindTexture(GL_TEXTURE_2D, _fboTexDens[(_ntex_dens) % 2]);
  glBindImageTexture(0, _fboTexDens[(_ntex_dens) % 2], 0, GL_FALSE, 0, GL_WRITE_ONLY,
                     GL_RGBA32F);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, _fboTexDens[(_ntex_dens + 1) % 2]);
  glUniform1i(glGetUniformLocation(_advect_program, "tex"), 0);
  glActiveTexture(GL_TEXTURE0 + 1);
  glBindTexture(GL_TEXTURE_2D, _fboTexVel[(_ntex_vel + 1) % 2]);
  glUniform1i(glGetUniformLocation(_advect_program, "tex_advect"), 1);
  glUniform1f(glGetUniformLocation(_advect_program, "dt"), _dt);
  glUniform1i(glGetUniformLocation(_advect_program, "b"), 0);
  glUniform1i(glGetUniformLocation(_advect_program, "w"), WINDOW_WIDTH);
  glDispatchCompute(_dim_dispatch[0], _dim_dispatch[1], 1);
  glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
  glBindImageTexture(0, 0, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
  glBindTexture(GL_TEXTURE_2D, 0);
  glUseProgram(0);
  ++_ntex_dens;
}

static inline void vel_advect(){
  glUseProgram(_advect_program);
  glBindTexture(GL_TEXTURE_2D, _fboTexVel[(_ntex_vel) % 2]);
  glBindImageTexture(0, _fboTexVel[(_ntex_vel) % 2], 0, GL_FALSE, 0, GL_WRITE_ONLY,
                     GL_RGBA32F);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, _fboTexVel[(_ntex_vel + 1) % 2]);
  glUniform1i(glGetUniformLocation(_advect_program, "tex"), 0);
  glActiveTexture(GL_TEXTURE0 + 1);
  glBindTexture(GL_TEXTURE_2D, _fboTexVel[(_ntex_vel + 1) % 2]);
  glUniform1i(glGetUniformLocation(_advect_program, "tex_advect"), 1);
  glUniform1f(glGetUniformLocation(_advect_program, "dt"), _dt);
  glUniform1i(glGetUniformLocation(_advect_program, "b"), 1);
  glUniform1i(glGetUniformLocation(_advect_program, "w"), WINDOW_WIDTH);
  glDispatchCompute(_dim_dispatch[0], _dim_dispatch[1], 1);
  glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
  glBindImageTexture(0, 0, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
  glBindTexture(GL_TEXTURE_2D, 0);
  glUseProgram(0);
  ++_ntex_vel;
}

static inline void clear_proj_tex(){
//TODO ne clear que la tex en lecture
  for(int i=2 ; i<2 ; ++i){
    glBindFramebuffer(GL_FRAMEBUFFER, _fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           _fboTexProjectP[i], 0);
    glClear(GL_COLOR_BUFFER_BIT);
    glBindVertexArray(_quad_vao);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
  }
}

static inline void project_1(){
  //Calcul de la divergence
  glUseProgram(_project_1_program);
  glBindTexture(GL_TEXTURE_2D, _fboTexProjectDiv[0]);
  glBindImageTexture(0, _fboTexProjectDiv[0], 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, _fboTexVel[(_ntex_vel + 1) % 2]);
  glUniform1i(glGetUniformLocation(_project_1_program, "tex"), 0);
  glUniform1i(glGetUniformLocation(_project_1_program, "b"), 0);
  glUniform1i(glGetUniformLocation(_project_1_program, "w"), WINDOW_WIDTH);
  glDispatchCompute(_dim_dispatch[0], _dim_dispatch[1], 1);
  glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
  glBindImageTexture(0, 0, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
  glBindTexture(GL_TEXTURE_2D, 0);
  glUseProgram(0);
}

static inline void project_2(int it_proj){
  //Clear texture project
  clear_proj_tex();//TODO ne clear que la tex en lecture
  //Calcul de la projection
  for (int i = 0; i < it_proj; ++i) {
    glUseProgram(_project_2_program);
    glBindTexture(GL_TEXTURE_2D, _fboTexProjectP[_ntex_p % 2]);
    glBindImageTexture(0, _fboTexProjectP[_ntex_p % 2], 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, _fboTexProjectP[(_ntex_p + 1) % 2]);
    glUniform1i(glGetUniformLocation(_project_2_program, "tex"), 0);
    glActiveTexture(GL_TEXTURE0 + 1);
    glBindTexture(GL_TEXTURE_2D, _fboTexProjectDiv[0]);
    glUniform1i(glGetUniformLocation(_project_2_program, "tex_div"), 1);
    glUniform1i(glGetUniformLocation(_project_2_program, "b"), 0);
    glUniform1i(glGetUniformLocation(_project_2_program, "w"), WINDOW_WIDTH);
    glDispatchCompute(_dim_dispatch[0], _dim_dispatch[1], 1);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    glBindImageTexture(0, 0, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(0);
    ++_ntex_p;
  }
}

static inline void project_3(){
  //Ajustement du champ de vitesse
  glUseProgram(_project_3_program);
  glBindTexture(GL_TEXTURE_2D, _fboTexVel[_ntex_vel % 2]);
  glBindImageTexture(0, _fboTexVel[_ntex_vel % 2], 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, _fboTexVel[(_ntex_vel + 1) % 2]);
  glUniform1i(glGetUniformLocation(_project_3_program, "tex_vel"), 0);
  glActiveTexture(GL_TEXTURE0 + 1);
  glBindTexture(GL_TEXTURE_2D, _fboTexProjectP[(_ntex_p + 1) % 2]);
  glUniform1i(glGetUniformLocation(_project_3_program, "tex_p"), 1);
  glUniform1i(glGetUniformLocation(_project_3_program, "b"), 1);
  glUniform1i(glGetUniformLocation(_project_3_program, "w"), WINDOW_WIDTH);
  glDispatchCompute(_dim_dispatch[0], _dim_dispatch[1], 1);
  glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
  glBindImageTexture(0, 0, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
  glBindTexture(GL_TEXTURE_2D, 0);
  glUseProgram(0);
  ++_ntex_vel;
}

static inline void render(){
  glBindFramebuffer(GL_FRAMEBUFFER, _fbo);
  //glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _fboTexVel[_ntex_vel % 2],  0);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _fboTexDens[_ntex_dens % 2],  0);
  glClear(GL_COLOR_BUFFER_BIT);
  glUseProgram(_render_program);
  glActiveTexture(GL_TEXTURE0);
  //glBindTexture(GL_TEXTURE_2D, _fboTexVel[(_ntex_vel + 1) % 2]);
  glBindTexture(GL_TEXTURE_2D, _fboTexDens[(_ntex_dens + 1) % 2]);
  glUniform1i(glGetUniformLocation(_render_program, "tex"), 0);
  glUniform1f(glGetUniformLocation(_render_program, "nl"), NON_LINEAR_RENDER);
  glBindVertexArray(_quad_vao);
  glDrawArrays(GL_TRIANGLES, 0, 6);
  //++_ntex_vel;
  ++_ntex_dens;

  glBindVertexArray(0);
  glBindTexture(GL_TEXTURE_2D, 0);
  glUseProgram(0);
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
  glBlitFramebuffer(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT,
                    0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, GL_COLOR_BUFFER_BIT, GL_NEAREST);
}

static inline void draw(float current_time) {
  _dt = current_time - _last_time;
  _last_time = current_time;
  _seconde += _dt;
  ++_fps;
  if(_seconde >= 1.0){
    cout << "fps = " << _fps << endl;
    _seconde = _seconde - 1.0;
    _fps = 0;
  }
  if(BOOL_CHANGE_COLOR) COLOR[2] = 0.45 + (1.0 + cos(COLOR_CHANGE_VIT * current_time)) / 4.0;
  //if(COLOR[1] > 1.0) COLOR[1] = 0.0;
  //cout << "dt = " << _dt << endl;

  if(_clickstate) vel_add();
  vel_diff();
  project_1();
  project_2(IPROJ1);
  project_3();
  vel_advect();
  project_1();
  project_2(IPROJ2);
  project_3();

  if(_clickstate) dens_add();
  dens_diff();
  dens_advect();

  render();

  _motion[0] = 0.0; _motion[1] = 0.0;
  /*
  Simulate step:
  -vel step  -dens step  -draw

  Vel step:
  -add source  -diffuse  -project  -advect  -project
  Dens step:
  -add source  -diffuse  -advect
   */
}

void quit() {
  glDeleteShader(_rend_vert_shader);
  glDeleteShader(_rend_frag_shader);
  glDeleteShader(_diff_cs);
  glDeleteShader(_advect_cs);
  glDeleteShader(_dens_add_cs);
  glDeleteShader(_vel_add_cs);
  glDeleteShader(_project_1_cs);
  glDeleteShader(_project_2_cs);
  glDeleteShader(_project_3_cs);

  if(_fboTexDens[0]){
    glDeleteTextures(2, _fboTexDens);
    _fboTexDens[0] = 0;
    _fboTexDens[1] = 0;
  }
  if(_fboTexVel[0]){
    glDeleteTextures(2, _fboTexVel);
    _fboTexVel[0] = 0;
    _fboTexVel[1] = 0;
  }
  if(_fboTexProjectP[0]){
    glDeleteTextures(2, _fboTexProjectP);
    _fboTexProjectP[0] = 0;
    _fboTexProjectP[1] = 0;
  }
  if(_fboTexProjectDiv[0]){
    glDeleteTextures(1, _fboTexProjectDiv);
    _fboTexProjectDiv[0] = 0;
  }
}

int main() {
  // Init GLFW (need to do this first)
  glfwInit();
  // Use OpenGL 4.3 Core
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  // Create window and context
  window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT,
                            "Simu fluid - without gl4d", NULL, NULL);
  glfwShowWindow(window);
  glfwMakeContextCurrent(window);

  // Initialise GLEW (needs to happen after we have a context)
  if (glewInit() != GLEW_OK) {
    throw std::runtime_error("Failed to initialise GLEW");
  }
  // Fonction callback pour gérer les mouse motions
  glfwSetCursorPosCallback(window, cursor_position_callback);
  // Fonction callback pour gérer les mouse clicks
  glfwSetMouseButtonCallback(window, mouse_button_callback);
  // Affichage des dimensions des groupes de travail GPU
  printWorkGroupsCapabilities();
  init();
  auto lastTime = std::chrono::high_resolution_clock::now();
  do {
    // Clear the screen. It's not mentioned before Tutorial 02, but it can cause
    // flickering, so it's there nonetheless.
    auto currentTime = std::chrono::high_resolution_clock::now();
    float current_time = std::chrono::duration<float>(currentTime - lastTime).count();
    draw(current_time);
    // Swap buffers
    glfwSwapBuffers(window);
    glfwPollEvents();
  } // Check if the ESC key was pressed or the window was closed
  while (glfwGetKey(window, GLFW_KEY_L) != GLFW_PRESS &&
         glfwWindowShouldClose(window) == 0);

  quit();
  glfwDestroyWindow(window);
  glfwTerminate();
  return 0;
}
