/* Phase 1 PT, utiliser le cycle double textures avec fbo + click utilisateur
 * qui change la couleur des pixels du fbo (ajout de matière) */
#include <GL4D/gl4df.h>
#include <GL4D/gl4du.h>
#include <GL4D/gl4duw_SDL2.h>
#include <stdio.h>

#define IDIFFVEL 80  // Nombre tour pour le calcul de la diffusion
#define IDIFFDENS 80 // Nombre tour pour le calcul de la diffusion
#define IPROJ 210     // Nombre tour pour le calcul de la diffusion
#define VEL 0.0      // Valeur par défaut du champ de vitesse

/* Prototypes des fonctions statiques contenues dans ce fichier C */
static void init(void);
static void resize(int w, int h);
static void mouse(int button, int state, int x, int y);
static void motion_mouse(int x, int y);
static void draw(void);
static void quit(void);
/*!\brief dimensions de la fenêtre */
static int _wW = 800, _wH = 800;
/*!\brief identifiant du programme GLSL correspondant à l'ajout de densité par
 * le click*/
static GLuint _pIdAddDens = 0;
static GLuint _pIdAddVel = 0;
static GLuint _pIdAdvect = 0;
static GLuint _pIdDiff = 0;
static GLuint _pIdSetTexTo0 = 0;
static GLuint _pIdProject1 = 0;
static GLuint _pIdProject2 = 0;
static GLuint _pIdProject3 = 0;
/*!\brief identifiant pour un quadrilatère GL4Dummies */
static GLuint _quad = 0;
/*!\brief identifiant du FBO et des textures E/S */
static GLuint _fbo, _fboTexDens[2], _fboTexVel[2], _fboTexProjectDiv[1],
	_fboTexProjectP[2], _fboTexDensBnd[1], _fboTexVelBnd[1];
/*vecteur de mouvement calculer dans la fonction mouse : click2(x,y) -
 * click1(x,y)*/
static GLfloat _addmotion1[2] = {0.0f, 0.0f};
static GLfloat _addmotion2[2] = {0.0f, 0.0f};
/*valeur pour savoir si on doit dessiner une entrée utilisateur*/
static GLfloat _motion[2] = {0.0f, 0.0f};
/*valeur pour savoir si on doit dessiner une entrée utilisateur*/
static int _clickstate = 0;
/* valeur de perte pour la diffusion de densite */
static GLfloat _densloss = 0.9998;
/* valeur de perte pour la diffusion de vel */
static GLfloat _velloss = 1.0;

/*!\brief La fonction principale créé la fenêtre d'affichage,
 * initialise GL et les données, affecte les fonctions d'événements et
 * lance la boucle principale d'affichage.*/
int main(int argc, char **argv) {
  if (!gl4duwCreateWindow(argc, argv, "GL4Dummies", GL4DW_POS_UNDEFINED,
						  GL4DW_POS_UNDEFINED, _wW, _wH,
						  GL4DW_RESIZABLE | GL4DW_SHOWN))
	return 1;
  init();
  atexit(quit);
  SDL_GL_SetSwapInterval(0);
  gl4duwResizeFunc(resize);
  gl4duwMouseFunc(mouse);
  gl4duwMotionFunc(motion_mouse);
  gl4duwDisplayFunc(draw);
  gl4duwMainLoop();
  return 0;
}
static void init(void) {
  int i;
  GLuint *tex = malloc(_wW * _wH * sizeof *tex);
  assert(tex);
  GLfloat *tex_advect = malloc(_wW * _wH * 4 * sizeof *tex_advect);
  assert(tex_advect);
  GLfloat *tex_project_div = malloc(_wW * _wH * 4 * sizeof *tex_advect);
  assert(tex_project_div);
  GLfloat *tex_project_p = malloc(_wW * _wH * 4 * sizeof *tex_advect);
  assert(tex_project_p);
  GLfloat *tex_dens_bnd = malloc(_wW * _wH * 4 * sizeof *tex_dens_bnd);
  assert(tex_dens_bnd);
  GLfloat *tex_vel_bnd = malloc(_wW * _wH * 4 * sizeof *tex_vel_bnd);
  assert(tex_vel_bnd);
  /* remplir tex de noir*/
  for (i = 0; i < _wW * _wH; ++i)
	tex[i] = 150 << 24; // ABGR
  /* remplir tex_advect de nos données de vecteur de mouvement */
  for (i = 0; i < _wW * _wH * 4; ++i) {
	tex_advect[i] = VEL;
  }
  // remplir les texture pour les calcul de boundaries, offset et multiplication
  // pour inverser les vitesse
  for (int i = 0; i < _wH; ++i) {
	for (int j = 0, w = _wW * 4; j < w; j += 4) {
	  // X Y Z W  ---->
	  tex_dens_bnd[i * w + j + 0] = 0.0;
	  tex_dens_bnd[i * w + j + 1] = 0.0;
	  tex_dens_bnd[i * w + j + 2] = 0.0;
	  tex_dens_bnd[i * w + j + 3] = 0.0;
	  tex_vel_bnd[i * w + j + 0] = 0.0;
	  tex_vel_bnd[i * w + j + 1] = 0.0;
	  tex_vel_bnd[i * w + j + 2] = 0.0;
	  tex_vel_bnd[i * w + j + 3] = 0.0;
	}
  }
  /* Génération d'un framebuffer pour pouvoir passer d'une texture à l'autre */
  glGenFramebuffers(1, &_fbo);
  /* générer deux identifiants de texture */
  glGenTextures(2, _fboTexDens);
  for (i = 0; i < 2; ++i) {
	/* lier un identifiant de texture comme texture 2D (1D ou 3D
	 * possibles) */
	glBindTexture(GL_TEXTURE_2D, _fboTexDens[i]);
	/* paramétrer la texture, voir la doc de la fonction glTexParameter
	 * sur
	 * https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glTexParameter.xhtml
	 */
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	/* envoi de la donnée texture depuis la RAM CPU vers la RAM GPU voir
	 * la doc de glTexImage2D (voir aussi glTexImage1D et glTexImage3D)
	 * sur
	 * https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glTexImage2D.xhtml
	 */
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, _wW, _wH, 0, GL_RGBA,
				 GL_UNSIGNED_BYTE, tex);
  }
  glGenTextures(2, _fboTexVel);
  for (i = 0; i < 2; ++i) {
	/* lier un identifiant de texture comme texture 2D (1D ou 3D
	 * possibles) */
	glBindTexture(GL_TEXTURE_2D, _fboTexVel[i]);
	/* paramétrer la texture, voir la doc de la fonction glTexParameter
	 * sur
	 * https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glTexParameter.xhtml
	 */
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	/* envoi de la donnée texture depuis la RAM CPU vers la RAM GPU voir
	 * la doc de glTexImage2D (voir aussi glTexImage1D et glTexImage3D)
	 * sur
	 * https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glTexImage2D.xhtml
	 */
	// glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, _wW, _wH, 0, GL_RGBA,
	// GL_UNSIGNED_BYTE, tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, _wW, _wH, 0, GL_RGBA, GL_FLOAT,
				 tex_advect);
  }
  glGenTextures(1, _fboTexProjectDiv);
  /* lier un identifiant de texture comme texture 2D (1D ou 3D
   * possibles) */
  glBindTexture(GL_TEXTURE_2D, _fboTexProjectDiv[0]);
  /* paramétrer la texture, voir la doc de la fonction glTexParameter
   * sur
   * https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glTexParameter.xhtml
   */
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  /* envoi de la donnée texture depuis la RAM CPU vers la RAM GPU voir
   * la doc de glTexImage2D (voir aussi glTexImage1D et glTexImage3D)
   * sur
   * https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glTexImage2D.xhtml
   */
  // glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, _wW, _wH, 0, GL_RGBA,
  // GL_UNSIGNED_BYTE, tex);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, _wW, _wH, 0, GL_RGBA, GL_FLOAT,
			   tex_project_div);

  glGenTextures(2, _fboTexProjectP);
  for (i = 0; i < 2; ++i) {
	glBindTexture(GL_TEXTURE_2D, _fboTexProjectP[i]);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, _wW, _wH, 0, GL_RGBA, GL_FLOAT,
				 tex_project_p);
  }
  glGenTextures(1, _fboTexDensBnd);
  glBindTexture(GL_TEXTURE_2D, _fboTexDensBnd[0]);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, _wW, _wH, 0, GL_RGBA, GL_FLOAT,
			   tex_dens_bnd);

  glGenTextures(1, _fboTexVelBnd);
  glBindTexture(GL_TEXTURE_2D, _fboTexVelBnd[0]);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, _wW, _wH, 0, GL_RGBA, GL_FLOAT,
			   tex_vel_bnd);

  /* plus besoin de la texture en CPU */
  free(tex);
  free(tex_advect);
  free(tex_project_div);
  free(tex_project_p);
  free(tex_dens_bnd);
  free(tex_vel_bnd);
  /* dé-lier la texture 2D */
  glBindTexture(GL_TEXTURE_2D, 0);

  /* Création du programme shader advect (voir le dossier shader) */
  _pIdAdvect = gl4duCreateProgram("<vs>shaders/basic.vert",
								  "<fs>shaders/advect.frag", NULL);
  /* Création du programme shader click (voir le dossier shader) */
  _pIdAddDens = gl4duCreateProgram("<vs>shaders/basic.vert",
								   "<fs>shaders/basic.frag", NULL);
  /* Création du programme shader pour ajouter du mouvement */
  _pIdAddVel = gl4duCreateProgram("<vs>shaders/basic.vert",
								  "<fs>shaders/vel_add.frag", NULL);
  /* Création du programme shader diff */
  _pIdDiff = gl4duCreateProgram("<vs>shaders/basic.vert",
								"<fs>shaders/diff.frag", NULL);
  /* Création du programme shader pour set une texture avec des 0 */
  _pIdSetTexTo0 = gl4duCreateProgram("<vs>shaders/basic.vert",
									 "<fs>shaders/set_to_0.frag", NULL);
  /* Création du programme shader pour calculer la divergence (nécessaire pour
   * l'étape "project") */
  _pIdProject1 = gl4duCreateProgram("<vs>shaders/basic.vert",
									"<fs>shaders/project_1.frag", NULL);
  /* Création du programme shader pour faire project 2 */
  _pIdProject2 = gl4duCreateProgram("<vs>shaders/basic.vert",
									"<fs>shaders/project_2.frag", NULL);
  /* Création du programme shader pour faire project 3 */
  _pIdProject3 = gl4duCreateProgram("<vs>shaders/basic.vert",
									"<fs>shaders/project_3.frag", NULL);

  /* Création du quadrilatère */
  _quad = gl4dgGenQuadf();
  /* Set de la couleur (RGBA) d'effacement OpenGL */
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  /* activation de la texture 2D */
  glEnable(GL_TEXTURE_2D);
  /* dans quelle partie de l'écran on effectue le rendu */
  glViewport(0, 0, _wW, _wH);
}

/*!\brief Cette fonction paramétre la vue (viewport) OpenGL en
 * fonction des dimensions de la fenêtre.*/
static void resize(int w, int h) {
  _wW = w;
  _wH = h;
  glViewport(0, 0, _wW, _wH);
  gl4duBindMatrix("projectionMatrix");
  gl4duLoadIdentityf();
  gl4duFrustumf(-0.5, 0.5, -0.5 * _wH / _wW, 0.5 * _wH / _wW, 1.0, 1000.0);
  gl4duBindMatrix("modelViewMatrix");
}

static void mouse(int button, int state, int x, int y) {
  if (button == GL4D_BUTTON_LEFT) {
	_clickstate = (_clickstate + 1) % 2;
	if (_clickstate == 1) {
	  _addmotion1[0] = (float)x;
	  _addmotion1[1] = (float)y;
	  _addmotion2[0] = (float)x;
	  _addmotion2[1] = (float)y;
	} else {
	  _motion[0] = 0.0;
	  _motion[1] = 0.0;
	}
  }
}

static void motion_mouse(int x, int y) {
  // printf("x=%d,  y=%d\n", x, y);
  _addmotion1[0] = _addmotion2[0];
  _addmotion1[1] = _addmotion2[1];
  _addmotion2[0] = x;
  _addmotion2[1] = y;
  _motion[0] = x - _addmotion1[0];
  _motion[1] = y - _addmotion1[1];
  _motion[0] *= -1.0;
  _motion[1] *= -1.0;
  // if(_motion[1] > 0) _motion[1] = 0.0;
  // printf("motion %d   %d\n", x, y);
}
static inline int vel_add(int ntex_vel, GLfloat *step) {
  /*VEL ADD*/
  /* activer le fbo _fbo en lecture/écriture (donc on ne dessine pas à l'écran)
   */
  glBindFramebuffer(GL_FRAMEBUFFER, _fbo);
  /* lier une des deux textures sur le fbo (pour écrire dedans) */
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
						 _fboTexVel[ntex_vel % 2], 0);
  /* effacement du buffer de couleur, donc de la texture liée au fbo. */
  glClear(GL_COLOR_BUFFER_BIT);
  /* activation du programme _pIdAddVel */
  glUseProgram(_pIdAddVel);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, _fboTexVel[(ntex_vel + 1) % 2]);
  glUniform1i(glGetUniformLocation(_pIdAddVel, "tex"), 0);
  glUniform2fv(glGetUniformLocation(_pIdAddVel, "step"), 1, step);
  /* envoyer les coordonnées du pixel qu'on veut changé au fs */
  glUniform2fv(glGetUniformLocation(_pIdAddVel, "addsource"), 1, _addmotion1);
  /* envoyer la valeur de _clickstate pour savoir l'utilisateur vient de cliquer
   */
  glUniform2fv(glGetUniformLocation(_pIdAddVel, "motion"), 1, _motion);
  /*on indique qu'il n'y a pas d'entrée utilisateur à gérer*/
  /* dessiner le quadrilatère */
  gl4dgDraw(_quad);
  /* désactiver le programme shader */
  glUseProgram(0);
  /* le <<fbo>> écran passe en actif sur le mode écriture */
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
  /* on blit _fbo vers écran (0) */
  // glBlitFramebuffer(0, 0, _wW, _wH, 0, 0, _wW, _wH, GL_COLOR_BUFFER_BIT,
  // GL_NEAREST); glBindFramebuffer(GL_FRAMEBUFFER, 0);
  ++ntex_vel; /* pour échanger les rôles le coup d'après */
  return ntex_vel;
}

static inline int vel_diff(int ntex_vel, GLfloat *step) {
  /* VEL DIFF */
	/* activer le fbo _fbo en lecture/écriture (donc on ne dessine pas à
	 * l'écran) */
	glBindFramebuffer(GL_FRAMEBUFFER, _fbo);
	/* lier une des deux textures sur le fbo (pour écrire dedans) */
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
						   _fboTexVel[ntex_vel % 2], 0);
	/* effacement du buffer de couleur, donc de la texture liée au fbo. */
	// glClear(GL_COLOR_BUFFER_BIT);
	/* activation du programme _pIdDiff */
	glUseProgram(_pIdDiff);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, _fboTexVel[(ntex_vel + 1) % 2]);
	glUniform1i(glGetUniformLocation(_pIdDiff, "tex"), 0);
	glUniform2fv(glGetUniformLocation(_pIdDiff, "step"), 1, step);
	glUniform1f(glGetUniformLocation(_pIdDiff, "loss"), _velloss);
	glUniform1i(glGetUniformLocation(_pIdDiff, "b"), 1);
	glActiveTexture(GL_TEXTURE0 + 4);
	glBindTexture(GL_TEXTURE_2D, _fboTexVelBnd[0]);
	glUniform1i(glGetUniformLocation(_pIdDiff, "tex_bnd"), 4);
	/* dessiner le quadrilatère */
	gl4dgDraw(_quad);
	/* désactiver le programme shader */
	glUseProgram(0);
	/* le <<fbo>> écran passe en actif sur le mode écriture */
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	// glBindFramebuffer(GL_FRAMEBUFFER, 0);
	ntex_vel++; /* pour échanger les rôles le coup d'après */
	return ntex_vel;
}
static inline int dens_add(int ntex, GLfloat *step) {
  /* activer le fbo _fbo en lecture/écriture (donc on ne dessine pas à l'écran)
   */
  glBindFramebuffer(GL_FRAMEBUFFER, _fbo);
  /* lier une des deux textures sur le fbo (pour écrire dedans) */
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
						 _fboTexDens[ntex % 2], 0);
  /* effacement du buffer de couleur, donc de la texture liée au fbo. */
  glClear(GL_COLOR_BUFFER_BIT);
  /* activation du programme _pIdAddDens */
  glUseProgram(_pIdAddDens);
  /* activer l'étage de textures 0, plusieurs étages sont disponibles,
   * nous pouvons lier une texture par type et par étage */
  glActiveTexture(GL_TEXTURE0);
  /* utiliser l'autre texture (en écriture) */
  glBindTexture(GL_TEXTURE_2D, _fboTexDens[(ntex + 1) % 2]);
  /* envoyer une info au program shader indiquant que tex est une
   * texture d'étage 0, voir le type (sampler2D) de la variable tex
   * dans le shader */
  glUniform1i(glGetUniformLocation(_pIdAddDens, "tex"), 0);
  /* envoyer les tailles des pas pour passer au texel suivant en x et y */
  glUniform2fv(glGetUniformLocation(_pIdAddDens, "step"), 1, step);
  /* envoyer les coordonnées du pixel qu'on veut changé au fs */
  glUniform2fv(glGetUniformLocation(_pIdAddDens, "addsource"), 1, _addmotion1);
  /*on indique qu'il n'y a pas d'entrée utilisateur à gérer*/
  /* dessiner le quadrilatère */
  gl4dgDraw(_quad);
  /* désactiver le programme shader */
  glUseProgram(0);
  /* le <<fbo>> écran passe en actif sur le mode écriture */
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
  /* on blit _fbo vers écran (0) */
  // glBlitFramebuffer(0, 0, _wW, _wH, 0, 0, _wW, _wH, GL_COLOR_BUFFER_BIT,
  // GL_NEAREST); glBindFramebuffer(GL_FRAMEBUFFER, 0);
  return ++ntex; /* pour échanger les rôles le coup d'après */
}

static inline int dens_diff(int ntex, GLfloat *step) {
  for (int i = 0; i < IDIFFDENS; ++i) {
	/* activer le fbo _fbo en lecture/écriture (donc on ne dessine pas à
	 * l'écran) */
	glBindFramebuffer(GL_FRAMEBUFFER, _fbo);
	/* lier une des deux textures sur le fbo (pour écrire dedans) */
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
						   _fboTexDens[ntex % 2], 0);
	/* effacement du buffer de couleur, donc de la texture liée au fbo. */
	// glClear(GL_COLOR_BUFFER_BIT);
	/* activation du programme _pIdDiff */
	glUseProgram(_pIdDiff);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, _fboTexDens[(ntex + 1) % 2]);
	glUniform1i(glGetUniformLocation(_pIdDiff, "tex"), 0);
	glUniform2fv(glGetUniformLocation(_pIdDiff, "step"), 1, step);
	/* envoie de la variable de perte de diffusion */
	glUniform1f(glGetUniformLocation(_pIdDiff, "loss"), _densloss);
	glUniform1i(glGetUniformLocation(_pIdDiff, "b"), 0);
	glActiveTexture(GL_TEXTURE0 + 4);
	glBindTexture(GL_TEXTURE_2D, _fboTexDensBnd[0]);
	glUniform1i(glGetUniformLocation(_pIdDiff, "tex_bnd"), 4);
	/* dessiner le quadrilatère */
	gl4dgDraw(_quad);
	/* désactiver le programme shader */
	glUseProgram(0);
	/* le <<fbo>> écran passe en actif sur le mode écriture */
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	++ntex; /* pour échanger les rôles le coup d'après */
  }
  return ntex;
}

static inline int dens_advect(int ntex_dens, int ntex_vel, GLfloat *step) {
  /* activer le fbo _fbo en lecture/écriture (donc on ne dessine pas à l'écran)
   */
  glBindFramebuffer(GL_FRAMEBUFFER, _fbo);
  /* lier une des deux textures sur le fbo (pour écrire dedans) */
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
						 _fboTexDens[ntex_dens % 2], 0);
  /* effacement du buffer de couleur, donc de la texture liée au fbo. */
  glClear(GL_COLOR_BUFFER_BIT);
  glUseProgram(_pIdAdvect);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, _fboTexDens[(ntex_dens + 1) % 2]);
  glUniform1i(glGetUniformLocation(_pIdAdvect, "tex"), 0);
  glUniform2fv(glGetUniformLocation(_pIdAdvect, "step"), 1, step);
  glActiveTexture(GL_TEXTURE0 + 2);
  glBindTexture(GL_TEXTURE_2D, _fboTexVel[(ntex_vel + 1) % 2]);
  glUniform1i(glGetUniformLocation(_pIdAdvect, "tex_advect"), 2);
  glUniform1i(glGetUniformLocation(_pIdAdvect, "b"), 0);
  glActiveTexture(GL_TEXTURE0 + 4);
  glBindTexture(GL_TEXTURE_2D, _fboTexDensBnd[0]);
  glUniform1i(glGetUniformLocation(_pIdDiff, "tex_bnd"), 4);
  /* dessiner le quadrilatère */
  gl4dgDraw(_quad);
  /* désactiver le programme shader */
  glUseProgram(0);
  /* le <<fbo>> écran passe en actif sur le mode écriture */
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
  // glBindFramebuffer(GL_FRAMEBUFFER, 0);
  /* on blit _fbo vers écran (0) */
  glBlitFramebuffer(0, 0, _wW, _wH, 0, 0, _wW, _wH, GL_COLOR_BUFFER_BIT,
					GL_NEAREST);
  return ++ntex_dens; /* pour échanger les rôles le coup d'après */
}

//Mis à zero des deux textures _fboTexProject
static inline void set_tex_proj_to_0(){
  for (int i = 0; i < 2; ++i) {
	glBindFramebuffer(GL_FRAMEBUFFER, _fbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
						   _fboTexProjectP[i], 0);
	glClear(GL_COLOR_BUFFER_BIT);
	glUseProgram(_pIdSetTexTo0);
	gl4dgDraw(_quad);
	glUseProgram(0);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
  }
}

/*!\brief Cette fonction dessine dans le contexte OpenGL actif. */
static void draw(void) {
  /* variable qui va permettre de passer d'une texture à l'autre, l'une en
   * écriture, l'autre en lecture */
  static int ntex_dens = 0;
  int ntex_vel = 0;
  int ntex_p = 0;
  GLfloat step[] = {1.0f / _wW, 1.0f / _wH};

  if (_clickstate) ntex_dens = dens_add(ntex_dens, step);
  if (_clickstate) ntex_vel = vel_add(ntex_vel, step);

  for (int i = 0; i < IDIFFVEL; ++i) {
	  ntex_vel = vel_diff(ntex_vel, step);
  }

  /* PROJECT */
  // Mis à 0 des deux textures project_p
  set_tex_proj_to_0();
  // Calcul de la divergence
  glBindFramebuffer(GL_FRAMEBUFFER, _fbo);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
						 _fboTexProjectDiv[0], 0);
  glClear(GL_COLOR_BUFFER_BIT);
  glUseProgram(_pIdProject1);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, _fboTexVel[(ntex_vel + 1) % 2]);
  glUniform1i(glGetUniformLocation(_pIdProject1, "tex"), 0);
  glUniform2fv(glGetUniformLocation(_pIdProject1, "step"), 1, step);
  glUniform1f(glGetUniformLocation(_pIdProject1, "h"), 1.0 / (GLfloat)_wW);
  glActiveTexture(GL_TEXTURE0 + 3);
  glBindTexture(GL_TEXTURE_2D, _fboTexDensBnd[0]);
  glUniform1i(glGetUniformLocation(_pIdDiff, "tex_bnd"), 3);
  gl4dgDraw(_quad);
  glUseProgram(0);
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

  glBindFramebuffer(GL_FRAMEBUFFER, _fbo);
  // Calcul de la projection
  for (int i = 0; i < IPROJ; ++i) {
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
						   _fboTexProjectP[ntex_p % 2], 0);
	glClear(GL_COLOR_BUFFER_BIT);
	glUseProgram(_pIdProject2);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, _fboTexProjectP[(ntex_p + 1) % 2]);
	glUniform1i(glGetUniformLocation(_pIdProject2, "tex"), 0);
	glUniform2fv(glGetUniformLocation(_pIdProject2, "step"), 1, step);
	glActiveTexture(GL_TEXTURE0 + 2);
	glBindTexture(GL_TEXTURE_2D, _fboTexProjectDiv[0]);
	glUniform1i(glGetUniformLocation(_pIdProject2, "tex_div"), 2);
	glActiveTexture(GL_TEXTURE0 + 3);
	glBindTexture(GL_TEXTURE_2D, _fboTexDensBnd[0]);
	glUniform1i(glGetUniformLocation(_pIdDiff, "tex_bnd"), 3);
	gl4dgDraw(_quad);
	// glUseProgram(0);
	++ntex_p;
  }
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

  // Ajustement du champ de vitesse
  glBindFramebuffer(GL_FRAMEBUFFER, _fbo);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
						 _fboTexVel[ntex_vel % 2], 0);
  glClear(GL_COLOR_BUFFER_BIT);
  glUseProgram(_pIdProject3);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, _fboTexVel[(ntex_vel + 1) % 2]);
  glUniform1i(glGetUniformLocation(_pIdProject3, "tex_vel"), 0);
  glUniform2fv(glGetUniformLocation(_pIdProject3, "step"), 1, step);
  glActiveTexture(GL_TEXTURE0 + 2);
  glBindTexture(GL_TEXTURE_2D, _fboTexProjectP[(ntex_p + 1) % 2]);
  glUniform1i(glGetUniformLocation(_pIdProject3, "tex_p"), 2);
  glUniform1f(glGetUniformLocation(_pIdProject3, "h"), 1.0 / (GLfloat)_wW);
  glActiveTexture(GL_TEXTURE0 + 3);
  glBindTexture(GL_TEXTURE_2D, _fboTexVelBnd[0]);
  glUniform1i(glGetUniformLocation(_pIdDiff, "tex_bnd"), 3);
  gl4dgDraw(_quad);
  glUseProgram(0);
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
  ++ntex_vel;

  /* VEL ADVECT */
  /* activer le fbo _fbo en lecture/écriture (donc on ne dessine pas à l'écran)
   */
  glBindFramebuffer(GL_FRAMEBUFFER, _fbo);
  /* lier une des deux textures sur le fbo (pour écrire dedans) */
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
						 _fboTexVel[ntex_vel % 2], 0);
  /* effacement du buffer de couleur, donc de la texture liée au fbo. */
  glClear(GL_COLOR_BUFFER_BIT);
  glUseProgram(_pIdAdvect);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, _fboTexVel[(ntex_vel + 1) % 2]);
  glUniform1i(glGetUniformLocation(_pIdAdvect, "tex"), 0);
  glUniform2fv(glGetUniformLocation(_pIdAdvect, "step"), 1, step);
  glActiveTexture(GL_TEXTURE0 + 2);
  glBindTexture(GL_TEXTURE_2D, _fboTexVel[(ntex_vel + 1) % 2]);
  glUniform1i(glGetUniformLocation(_pIdAdvect, "tex_advect"), 2);
  glUniform1i(glGetUniformLocation(_pIdAdvect, "b"), 1);
  glActiveTexture(GL_TEXTURE0 + 4);
  glBindTexture(GL_TEXTURE_2D, _fboTexVelBnd[0]);
  glUniform1i(glGetUniformLocation(_pIdDiff, "tex_bnd"), 4);
  /* dessiner le quadrilatère */
  gl4dgDraw(_quad);
  /* désactiver le programme shader */
  glUseProgram(0);
  /* le <<fbo>> écran passe en actif sur le mode écriture */
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
  ++ntex_vel;

  /* PROJECT */
  // Mis à 0 des deux textures project_p
  for (int i = 0; i < 2; ++i) {
	glBindFramebuffer(GL_FRAMEBUFFER, _fbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
						   _fboTexProjectP[i], 0);
	glClear(GL_COLOR_BUFFER_BIT);
	glUseProgram(_pIdSetTexTo0);
	gl4dgDraw(_quad);
	glUseProgram(0);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
  }

  // Calcul de la divergence
  glBindFramebuffer(GL_FRAMEBUFFER, _fbo);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
						 _fboTexProjectDiv[0], 0);
  glClear(GL_COLOR_BUFFER_BIT);
  glUseProgram(_pIdProject1);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, _fboTexVel[(ntex_vel + 1) % 2]);
  glUniform1i(glGetUniformLocation(_pIdProject1, "tex"), 0);
  glUniform2fv(glGetUniformLocation(_pIdProject1, "step"), 1, step);
  glUniform1f(glGetUniformLocation(_pIdProject1, "h"), 1.0 / (GLfloat)_wW);
  glActiveTexture(GL_TEXTURE0 + 3);
  glBindTexture(GL_TEXTURE_2D, _fboTexDensBnd[0]);
  glUniform1i(glGetUniformLocation(_pIdDiff, "tex_bnd"), 3);
  gl4dgDraw(_quad);
  glUseProgram(0);
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

  // Calcul de la projection
  glBindFramebuffer(GL_FRAMEBUFFER, _fbo);
  for (int i = 0; i < IPROJ; ++i) {
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
						   _fboTexProjectP[ntex_p % 2], 0);
	glClear(GL_COLOR_BUFFER_BIT);
	glUseProgram(_pIdProject2);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, _fboTexProjectP[(ntex_p + 1) % 2]);
	glUniform1i(glGetUniformLocation(_pIdProject2, "tex"), 0);
	glUniform2fv(glGetUniformLocation(_pIdProject2, "step"), 1, step);
	glActiveTexture(GL_TEXTURE0 + 2);
	glBindTexture(GL_TEXTURE_2D, _fboTexProjectDiv[0]);
	glUniform1i(glGetUniformLocation(_pIdProject2, "tex_div"), 2);
	glActiveTexture(GL_TEXTURE0 + 3);
	glBindTexture(GL_TEXTURE_2D, _fboTexDensBnd[0]);
	glUniform1i(glGetUniformLocation(_pIdDiff, "tex_bnd"), 3);
	gl4dgDraw(_quad);
	glUseProgram(0);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	++ntex_p;
  }

  // Ajustement du champ de vitesse
  glBindFramebuffer(GL_FRAMEBUFFER, _fbo);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
						 _fboTexVel[ntex_vel % 2], 0);
  glClear(GL_COLOR_BUFFER_BIT);
  glUseProgram(_pIdProject3);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, _fboTexVel[(ntex_vel + 1) % 2]);
  glUniform1i(glGetUniformLocation(_pIdProject3, "tex_vel"), 0);
  glUniform2fv(glGetUniformLocation(_pIdProject3, "step"), 1, step);
  glActiveTexture(GL_TEXTURE0 + 2);
  glBindTexture(GL_TEXTURE_2D, _fboTexProjectP[(ntex_p + 1) % 2]);
  glUniform1i(glGetUniformLocation(_pIdProject3, "tex_p"), 2);
  glUniform1f(glGetUniformLocation(_pIdProject3, "h"), 1.0 / (GLfloat)_wW);
  glActiveTexture(GL_TEXTURE0 + 3);
  glBindTexture(GL_TEXTURE_2D, _fboTexVelBnd[0]);
  glUniform1i(glGetUniformLocation(_pIdDiff, "tex_bnd"), 3);
  gl4dgDraw(_quad);
  glUseProgram(0);
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
  ++ntex_vel; // FIN PROJECT

  ntex_dens = dens_diff(ntex_dens, step);
  ntex_dens = dens_advect(ntex_dens, ntex_vel, step);

  _motion[0] = 0.0;
  _motion[1] = 0.0;
}
/*!\brief appelée au moment de sortir du programme (atexit), elle
 *  libère les éléments OpenGL utilisés.*/
static void quit(void) {
  /* suppression des deux textures en GPU */
  if (_fboTexDens[0]) {
	glDeleteTextures(2, _fboTexDens);
	_fboTexDens[0] = 0;
	_fboTexDens[1] = 0;
  }
  if (_fboTexVel[0]) {
	glDeleteTextures(2, _fboTexVel);
	_fboTexVel[0] = 0;
	_fboTexVel[1] = 0;
  }
  if (_fboTexProjectDiv[0]) {
	glDeleteTextures(1, _fboTexProjectDiv);
	_fboTexProjectDiv[0] = 0;
  }
  if (_fboTexProjectP[0]) {
	glDeleteTextures(2, _fboTexProjectP);
	_fboTexProjectP[0] = 0;
	_fboTexProjectP[1] = 0;
  }
  if (_fboTexDensBnd[0]) {
	glDeleteTextures(1, _fboTexDensBnd);
	_fboTexDensBnd[0] = 0;
  }
  if (_fboTexVelBnd[0]) {
	glDeleteTextures(1, _fboTexVelBnd);
	_fboTexVelBnd[0] = 0;
  }
  /* nettoyage des éléments utilisés par la bibliothèque GL4Dummies */
  gl4duClean(GL4DU_ALL);
}
