#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <iostream>
#include <fstream>

#include "helper.h"
#include "header.h"

#ifdef __APPLE__
#include <OpenGL/OpenGL.h>
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif
#define M_PI 3.14159265359

#define WHITE	100.0, 100.0, 100.0,	100.0
#define RED		100.0, 0.0, 0.0,		100.0
#define GREEN	0.0, 100.0, 0.0,		100.0
#define BLUE	0.0, 0.0, 100.0,		100.0
#define ORANGE	100.0, 65.0, 0.0,		100.0

#define RADPERDEG 0.0174533
float mapa32[32 * 32];
float mapa256[256 * 256];
float colorMap256[256 * 256][3];
int mapMinHeight = 20;
int windowWidth = 800, windowHeight = 600;
float check, colorScale;
float fps, pratoDead = false;
int countTiros = 0, countTirosOnTraget = 0;

GLfloat fov = 65.0, ratio = (GLdouble)windowWidth / windowHeight, nearDist = 0.1, farDist = 300.0;

float pratoOnScreen[4] = { 0 };
GLfloat pratoVelXYZ[3] = { 0, 0, 0 };
GLfloat pratoPosXYZ[3] = { 0, 0, 0 };
GLfloat pratoRotRand[15] = { 0 };
GLfloat pratoRandTrans = 0, pratoTransp = 100.0;
int pratoRaio = 5;

GLfloat playerPosXZ[2] = { 0, 0 };
GLfloat mouseScreenPosXY[2] = { 0, 0 };
GLfloat mousePosXYZ[3] = { 1, 0, 0 };
GLfloat cameraAngleY = 90.0;
GLfloat cameraAngleX = 0;
GLfloat wasd[4] = { 0, 0, 0, 0 };

/* teste para saltar*/
GLfloat velocidadeY = 0;
GLfloat saltoY = 0;
/* DEFINI‚ÌO DA COR DO NEVOEIRO */
GLfloat nevoeiroCor[] = { 0.0, 0.0, 0.0, 1.0 };

double lastTime = 0;
int nbFrames = 0;


float noise(int x, int y, int aleatorio) {
	int n = x + y * 57 + aleatorio * 131;
	n = (n << 13) ^ n;
	return (1.0f - ((n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff) / 1073741824.0f);
}

void setNoise(float  *mapa) {
	time_t t;
	float temp[34][34];
	srand(time(NULL));
	int aleatorio = rand() % 5000;

	srand((unsigned)time(&t));

	for (int y = 1; y<33; y++)
		for (int x = 1; x<33; x++)
			temp[x][y] = 128.0f + noise(x, y, aleatorio)*20.0f;

	for (int x = 1; x<33; x++) {
		temp[0][x] = temp[32][x];
		temp[33][x] = temp[1][x];
		temp[x][0] = temp[x][32];
		temp[x][33] = temp[x][1];
	}

	temp[0][0] = temp[32][32];
	temp[33][33] = temp[1][1];
	temp[0][33] = temp[32][1];
	temp[33][0] = temp[1][32];

	for (int y = 1; y<33; y++)
		for (int x = 1; x<33; x++) {
			float centro = temp[x][y] / 4.0f;
			float  lados = (temp[x + 1][y] + temp[x - 1][y] + temp[x][y + 1] + temp[x][y - 1]) / 8.0f;
			float cantos = (temp[x + 1][y + 1] + temp[x + 1][y - 1] + temp[x - 1][y + 1] + temp[x - 1][y - 1]) / 16.0f;

			mapa[((x - 1) * 32) + (y - 1)] = centro + lados + cantos;
		}
}

float interpola(float x, float y, float  *mapa) {
	int Xint = (int)x;
	int Yint = (int)y;

	float Xfrac = x - Xint;
	float Yfrac = y - Yint;

	int X0 = Xint % 32;
	int Y0 = Yint % 32;
	int X1 = (Xint + 1) % 32;
	int Y1 = (Yint + 1) % 32;

	float baixo = mapa[X0 * 32 + Y0] + Xfrac * (mapa[X1 * 32 + Y0] - mapa[X0 * 32 + Y0]);
	float topo = mapa[X0 * 32 + Y1] + Xfrac * (mapa[X1 * 32 + Y1] - mapa[X0 * 32 + Y1]);

	return (baixo + Yfrac * (topo - baixo));
}

void sobrepoeOitavas(float  *mapa32, float  *mapa256) {
	for (int x = 0; x<256 * 256; x++) {
		mapa256[x] = 0;
	}

	for (int oitava = 0; oitava<4; oitava++)
		for (int x = 0; x<256; x++)
			for (int y = 0; y<256; y++) {
				float escala = 1 / pow(2, 3 - oitava);
				float noise = interpola(x*escala, y*escala, mapa32);
				mapa256[(y * 256) + x] += noise / pow(2, oitava);
			}
}

void expFilter(float  *mapa) {
	float cover = 20.0f;
	float sharpness = 0.95f;

	for (int x = 0; x<256 * 256; x++) {
		float c = mapa[x] - (255.0f - cover);
		if (c<0) c = 0;
		mapa[x] = 255.0f - ((float)(pow(sharpness, c))*255.0f);
	}
}

void getNormal(GLfloat *nV, GLfloat *v1, GLfloat *v2, GLfloat *v3) {
	float tmpVec1[3], tmpVec2[3];
	tmpVec1[0] = v3[0] - v1[0];
	tmpVec1[1] = v3[1] - v1[1];
	tmpVec1[2] = v3[2] - v1[2];
	tmpVec2[0] = v2[0] - v1[0];
	tmpVec2[1] = v2[1] - v1[1];
	tmpVec2[2] = v2[2] - v1[2];
	nV[0] = v1[1] * v2[2] - v1[2]*v2[1];
	nV[1] = v1[2] * v2[0] - v1[0] * v2[2];
	nV[2] = v1[0] * v2[1] - v1[1] * v2[0];
}

float billbilinearInterpolation(float x, float z) {
	x = x / 4 + 128;
	z = z / 4 + 128;
	int x1 = (int)x;
	int x2 = (int)x + 1;
	int y1 = (int)z;
	int y2 = (int)z + 1;

	float q11 = mapa256[x1 * 256 + y1];
	float q12 = mapa256[x1 * 256 + y2];
	float q21 = mapa256[x2 * 256 + y1];
	float q22 = mapa256[x2 * 256 + y2];

	float r1 = ((x2 - x) / (x2 - x1)) * q11 + ((x - x1) / (x2 - x1)) * q21;
	float r2 = ((x2 - x) / (x2 - x1)) * q12 + ((x - x1) / (x2 - x1)) * q22;
	//printf("%f.2, %f.2 = %f.4\n", x, z, ((y2 - z) / (y2 - y1)) * r1 + ((z - y1) / (y2 - y1)) * r2);
	return ((y2 - z) / (y2 - y1)) * r1 + ((z - y1) / (y2 - y1)) * r2;
}

void loop() {
	sobrepoeOitavas(mapa32, mapa256);
	expFilter(mapa256);
}

void initNevoeiro(void) {
	glFogfv(GL_FOG_COLOR, nevoeiroCor); //Cor do nevoeiro
	glFogi(GL_FOG_MODE, GL_EXP2); //Equa‹o do nevoeiro - linear
									//Outras opcoes: GL_EXP, GL_EXP2
	glFogf(GL_FOG_START, 1.0); // Dist‰ncia a que ter‡ in’cio o nevoeiro
	glFogf(GL_FOG_END, 200.0); // Dist‰ncia a que o nevoeiro terminar‡
	glFogf(GL_FOG_DENSITY, 0.01);//glFogf (GL_FOG_DENSITY, 0.35); //Densidade do nevoeiro - n‹o se especifica quando temos "nevoeiro linear"
}

void defineLuzes() {
	glEnable(GL_LIGHTING); //turns the "lights" on 

	GLfloat lampPos[4] = { 10.0,100.0,10.0,1.0 };
	GLfloat lampDir[4] = { 0,-1,0,1.0 };


	GLfloat lampIntensity[4] = { 0.8,0.8,0.8,1.0 };
	GLfloat lampSpecular[4] = { 0.2,0.2,0.2,1.0 };
	GLfloat mat_amb[] = { 1, 1, 1, 1.0 };

	glLightfv(GL_LIGHT0, GL_POSITION, lampPos);
	glLightfv(GL_LIGHT0, GL_AMBIENT, lampIntensity);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, mat_amb);
	glLightfv(GL_LIGHT0, GL_SPECULAR, lampSpecular);
	glLightf(GL_LIGHT0, GL_SPOT_CUTOFF, 3.0);
	glLightfv(GL_LIGHT0, GL_SPOT_DIRECTION, lampDir);
	glLightf(GL_LIGHT0, GL_CONSTANT_ATTENUATION, 1);

	glEnable(GL_LIGHT0);

	glEnable(GL_COLOR_MATERIAL);
}

void resetPrato() {
	pratoPosXYZ[0] = playerPosXZ[0] + (rand() % 200 - 100);
	pratoPosXYZ[2] = playerPosXZ[1] + (rand() % 200 - 100);
	pratoPosXYZ[1] = billbilinearInterpolation(pratoPosXYZ[0], pratoPosXYZ[2]) / 256.0 * 70;
	pratoVelXYZ[1] = 0.5;
	pratoVelXYZ[0] = (rand() % 1) * 0.2 - 0.4;
	pratoVelXYZ[2] = (rand() % 1) * 0.2 - 0.4;
}

void drawText(char *string, GLfloat x, GLfloat y, GLfloat z) {
	glRasterPos3f(x, y, z);
	while (*string)
		glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, *string++);
}

void showFps() {
	/* HUD */
	glViewport(0, 0, windowWidth, windowHeight);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(10, -10, 10, -10);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	char text[100] = "";
	glColor4f(0, 1, 1, 1);
	sprintf(text, "FPS: %.1f", fps);
	drawText(text, -7, -8.5f, 1);
}

void showNTiros() {
	/* HUD */
	glViewport(0, 0, windowWidth, windowHeight);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(10, -10, 10, -10);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	char text[100] = "";
	glColor4f(0, 1, 1, 1);
	sprintf(text, "Shots: %d, Landed: %d", countTiros, countTirosOnTraget);
	drawText(text, 0, -8.5, 1);
}

void init() {
	//glClearColor(0, 0.75, 1, 1);
	setNoise(mapa32);

	/* ACTIVAR NEVOEIRO */
	glEnable(GL_FOG);
	initNevoeiro();

	glEnable(GL_COLOR_MATERIAL);
	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);


	/*ACTIVAR FRONT FACE CULLING*/

	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CW);
	// ENABLE DEPTH TEST
	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);
	

	loop();
	/*if (mapa256[0] < mapMinHeight)
		mapa256[0] = mapMinHeight;*/
	float min = mapa256[0], max = mapa256[0];
	for (int i = 0; i < 256; i++)
		for (int j = 0; j < 256; j++) {
			if (mapa256[i * 256 + j] < min)
				min = mapa256[i * 256 + j];
			if (mapa256[i * 256 + j] > max)
				max = mapa256[i * 256 + j];
		}
	srand(time(NULL));
	for (int i = 0; i < 256; i++)
		for (int j = 0; j < 256; j++) {
			if (mapa256[i * 256 + j] < mapMinHeight) {
				mapa256[i * 256 + j] = mapMinHeight + ((rand() % 6) - 3);
				colorMap256[i * 256 + j][0] = 0;
				colorMap256[i * 256 + j][1] = rand() % 5;
				colorMap256[i * 256 + j][2] = rand() % 25 + 10
					;
			}
			else {
				colorMap256[i * 256 + j][0] = mapa256[i * 256 + j];
				colorMap256[i * 256 + j][1] = mapa256[i * 256 + j];
				colorMap256[i * 256 + j][2] = mapa256[i * 256 + j];
			}
		}

	colorScale = abs(min - max);

	pratoTransp = 100.0;
	pratoRandTrans = 0;
	for (int i = 0; i < 15; i++) {
		pratoRotRand[i] = rand() % 360;
	}

	//printf("min: %d - max: %d - diff: %d\n", (int)min, (int)max, (int)abs(min - max));
	//defineLuzes();
	//glEnable(GL_LIGHT0);
	resetPrato();
}

void updateAndDisplayFPS() {
	double currentTime = glutGet(GLUT_ELAPSED_TIME);
	nbFrames++;
	if (currentTime - lastTime >= 1000.0) {
		//printf("%.2f ms/frame\n", float(nbFrames*1000.0 / (currentTime - lastTime)));
		fps = float(nbFrames*1000.0 / (currentTime - lastTime));
		nbFrames = 0;
		lastTime = currentTime;
	}
}

void handleMovement() {
	GLfloat speed = 1;
	if (wasd[0]) {
		playerPosXZ[0] += sin(cameraAngleY * M_PI / 180) * speed;
		playerPosXZ[1] += cos(cameraAngleY * M_PI / 180) * speed;
	}
	if (wasd[1]) {
		playerPosXZ[0] += sin((cameraAngleY + 90) * M_PI / 180) * speed;
		playerPosXZ[1] += cos((cameraAngleY + 90)* M_PI / 180) * speed;
	}
	if (wasd[2]) {
		playerPosXZ[0] += sin((cameraAngleY - 180)* M_PI / 180) * speed;
		playerPosXZ[1] += cos((cameraAngleY - 180)* M_PI / 180) * speed;
	}
	if (wasd[3]) {
		playerPosXZ[0] += sin((cameraAngleY - 90)* M_PI / 180) * speed;
		playerPosXZ[1] += cos((cameraAngleY - 90)* M_PI / 180) * speed;
	}
	//printf("%f, %f\n", playerPosXZ[0], playerPosXZ[1]);
}

void desenhaTerreno() {
	glShadeModel(GL_SMOOTH);
	glEnable(GL_POLYGON_OFFSET_FILL); // Avoid Stitching!
	glPolygonOffset(1.0, 1.0);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glColor3f(0.0, 0.0, 0.0);
	GLfloat v1[3], v2[3], v3[3], nV[3];
	/*glEnable(GL_COLOR_MATERIAL);
	GLfloat whiteSpecularMaterial[] = { 0.0, 0.0, 0.0 };
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, whiteSpecularMaterial);*/
	for (int i = 0; i < 256 - 1; i++) {
		glBegin(GL_TRIANGLE_STRIP);
		for (int j = 0; j < 256; j++) {
			if (j > 0) {
				v3[0] = (i - 128) * 4.0;
				v3[1] = (mapa256[i * 256 + j] / 256.0) * 70;
				v3[2] = (j - 128) * 4.0;
				getNormal(nV, v1, v2, v3);
				glNormal3f(nV[0], nV[1], nV[2]);
			}
			glVertex3f((i - 128) * 4.0, (mapa256[i * 256 + j] / 256.0) * 70, (j - 128) * 4.0);
			if (j > 0) {
				v1[0] = v3[0];
				v1[1] = v3[1];
				v1[2] = v3[2];
				v3[0] = (i - 127) * 4.0;
				v3[1] = (mapa256[(i + 1) * 256 + j] / 256.0) * 70;
				v3[2] = (j - 128) * 4.0;
				getNormal(nV, v1, v2, v3);
				glNormal3f(nV[0], nV[1], nV[2]);
			}
			glVertex3f((i - 127) * 4.0, (mapa256[(i + 1) * 256 + j] / 256.0) * 70, (j - 128) * 4.0);
			v1[0] = (i - 128) * 4.0;
			v1[1] = (mapa256[i * 256 + j] / 256.0) * 70;
			v1[2] = (j - 128) * 4.0;
			v2[0] = (i - 127) * 4.0;
			v2[1] = (mapa256[(i + 1) * 256 + j] / 256.0) * 70;
			v2[2] = (j - 128) * 4.0;
		}
		glEnd();
	}
	glDisable(GL_POLYGON_OFFSET_FILL);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	for (int i = 0; i < 256 - 1; i++) {
		glBegin(GL_TRIANGLE_STRIP);
		for (int j = 0; j < 256; j++) {
			glColor3f(colorMap256[i * 256 + j][0], colorMap256[i * 256 + j][1], colorMap256[i * 256 + j][2]);
			glVertex3f((i - 128) * 4.0, (mapa256[i * 256 + j] / 256.0) * 70, (j - 128) * 4.0);
			glColor3f(colorMap256[i * 256 + j][0], colorMap256[(i + 1) * 256 + j][1], colorMap256[i * 256 + j][2]);
			glVertex3f((i - 127) * 4.0, (mapa256[(i + 1) * 256 + j] / 256.0) * 70, (j - 128) * 4.0);
		}
		glEnd();
	}
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glShadeModel(GL_FLAT);
	
}

void desenhaMaquina() {
	glPushMatrix();
		glColor3d(50.0, 0.0, 0.0);
		glTranslatef(0.0, 25.0, 0.0);
		glScalef(2.0, 2.0, 2.0);
	glPopMatrix();
}

void drawCircle(float cx, float cy, float r, int num_segments)
{
	float theta = 2 * 3.1415926 / float(num_segments);
	float tangetial_factor = tanf(theta);//calculate the tangential factor 

	float radial_factor = cosf(theta);//calculate the radial factor 

	float x = r;//we start at angle = 0 

	float y = 0;

	glBegin(GL_TRIANGLE_FAN);
	for (int ii = 0; ii < num_segments; ii++)
	{
		glVertex2f(x + cx, y + cy);//output vertex 

								   //calculate the tangential vector 
								   //remember, the radial vector is (x, y) 
								   //to get the tangential vector we flip those coordinates and negate one of them 

		float tx = -y;
		float ty = x;

		//add the tangential vector 

		x += tx * tangetial_factor;
		y += ty * tangetial_factor;

		//correct using the radial factor 

		x *= radial_factor;
		y *= radial_factor;
	}
	glEnd();
}

void desenhaPratoFalso() {
	for (int i = 0; i < 15; i++) {
		glPushMatrix();
			glRotatef(pratoRotRand[i], 0, 1, 0);
			glTranslatef(pratoRandTrans, 0, 0);
			drawCircle(0, 0, 1, 300);
		glPopMatrix();
	}
}

void desenhaPrato() {
	glDisable(GL_CULL_FACE);
	//glShadeModel(GL_SMOOTH);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glColor4f(100.0, 65.0, 0.0, pratoTransp);
	if (!pratoDead) {
		glPushMatrix();
		glTranslatef(pratoPosXYZ[0], pratoPosXYZ[1], pratoPosXYZ[2]);
		float prjMat[4][4] = { 0 }, modViewMat[4][4] = { 0 };
		glGetFloatv(GL_PROJECTION_MATRIX, &prjMat[0][0]);
		glGetFloatv(GL_MODELVIEW_MATRIX, &modViewMat[0][0]);
		for (int i = 0; i < 4; i++) {
			float val = 0;
			for (int j = 0; j < 4; j++) {
				val += prjMat[i][j] * modViewMat[3][j];
			}
			pratoOnScreen[i] = val;
		}
		//printf("%f, %f, %f\n", ve[0], ve[1], ve[2]);
		//glScalef(1, 0.8, 1);
		glRotatef(cameraAngleY, 0, 1, 0);
		drawCircle(0, 0, pratoRaio, 300);
		glPopMatrix();

		pratoPosXYZ[1] += pratoVelXYZ[1];
		pratoVelXYZ[1] -= 0.0025;
		pratoPosXYZ[0] += pratoVelXYZ[0];
		pratoPosXYZ[2] += pratoVelXYZ[2];
		if (pratoPosXYZ[1] <= billbilinearInterpolation(pratoPosXYZ[0], pratoPosXYZ[2]) / 256.0 * 70)
			resetPrato();
	}
	else {
		glPushMatrix();
			glTranslatef(pratoPosXYZ[0], pratoPosXYZ[1], pratoPosXYZ[2]);
			desenhaPratoFalso();
		glPopMatrix();
		pratoRandTrans += 1;
		pratoTransp -= 3;
		//printf("%f\n", pratoTransp);
		if(pratoTransp < 0) {
			pratoTransp = 100.0;
			pratoRandTrans = 0;
			for (int i = 0; i < 15; i++) {
				pratoRotRand[i] = rand() % 360;
			}
			pratoDead = false;
			resetPrato();
		}
	}
	glEnable(GL_CULL_FACE);
}



void drawCrosshair() {
	glPushMatrix();
	glViewport(0, 0, windowWidth, windowHeight);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, windowWidth, windowHeight, 0, -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();


	glColor3ub(0, 240, 100);//white
	glLineWidth(2.0);
	glBegin(GL_LINES);
	//horizontal line
	glVertex2i(windowWidth / 2 - 7, windowHeight / 2);
	glVertex2i(windowWidth / 2 + 7, windowHeight / 2);
	glEnd();
	//vertical line
	glBegin(GL_LINES);
	glVertex2i(windowWidth / 2, windowHeight / 2 + 7);
	glVertex2i(windowWidth / 2, windowHeight / 2 - 7);
	glEnd();
	glLineWidth(1.0);
	glPopMatrix();
}

void drawWeapon() {
	glPushMatrix();
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluPerspective(fov, ratio, nearDist, farDist);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		gluLookAt(0, -100, 0, 1, -100, 0, 0.0, 1.0, 0.0);
		glFrontFace(GL_CCW);
		glTranslatef(0.9, -100.2, 0.3);
		glRotatef(190, 0, 1, 0);
		glRotatef(-5, 0, 0, 1);
		DrawFrame();
		glFrontFace(GL_CW);
	glPopMatrix();
}

void desenha() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	showFps();
	showNTiros();
	
	handleMovement();
	//glEnable(GL_DEPTH_TEST);
	
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(fov, ratio, nearDist, farDist);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	//defineLuzes();
	GLfloat tmpPosY = (billbilinearInterpolation(playerPosXZ[0], playerPosXZ[1]) / 256.0) * 70 + 2;
	saltoY += velocidadeY;
	tmpPosY += saltoY;

	velocidadeY -= 0.06;
	if ((billbilinearInterpolation(playerPosXZ[0], playerPosXZ[1]) / 256.0) * 70 + 2 > tmpPosY) {
		tmpPosY = (billbilinearInterpolation(playerPosXZ[0], playerPosXZ[1]) / 256.0) * 70 + 2;
		velocidadeY = 0;
	}

	gluLookAt(playerPosXZ[0], tmpPosY, playerPosXZ[1], playerPosXZ[0] + mousePosXYZ[0], tmpPosY + mousePosXYZ[1], playerPosXZ[1] + mousePosXYZ[2], 0.0, 1.0, 0.0);
	
	desenhaTerreno();
	desenhaPrato();

	updateAndDisplayFPS();
	drawWeapon();
	drawCrosshair();
	
	glutSwapBuffers();
}

void timer(int value) {
	glutPostRedisplay();
	glutTimerFunc(20, timer, 1);
}

void teclasNotAscii(int key, int x, int y)
{
	/*
	if (key == GLUT_KEY_LEFT) {
	playerPosXZ[0]--;
	glutPostRedisplay();
	}
	if (key == GLUT_KEY_RIGHT) {
	playerPosXZ[0]++;
	glutPostRedisplay();
	}
	if (key == GLUT_KEY_DOWN) {
	playerPosXZ[1]--;
	glutPostRedisplay();
	}
	if (key == GLUT_KEY_UP) {
	playerPosXZ[1]++;
	glutPostRedisplay();
	}
	*/
}

void teclado(unsigned char key, int x, int y) {
	switch (key) {
	case 27:	// ESC
		UnloadModel();
		exit(0);
		break;
	case 119:	// W
		wasd[0] = 1;
		break;
	case 97:	// A
		wasd[1] = 1;
		break;
	case 115:	// S
		wasd[2] = 1;
		break;
	case 100:	// D
		wasd[3] = 1;
		break;
	case 32: // ESPA‚O
		if (saltoY < 0) {
			velocidadeY = 1;
			saltoY = 0;
		}
		break;
	default:
		break;
	}
	//glutPostRedisplay();
}

void tecladoUp(unsigned char key, int x, int y) {
	switch (key) {
	case 27:	// ESC
		exit(0);
		break;
	case 119:	// W
		wasd[0] = 0;
		break;
	case 97:	// A
		wasd[1] = 0;
		break;
	case 115:	// S
		wasd[2] = 0;
		break;
	case 100:	// D
		wasd[3] = 0;
		break;
	default:
		break;
	}
	//glutPostRedisplay();
}

void onMouse(int button, int state, int x, int y) {
	if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
		countTiros++;
		//printf("SHOOOT: %f, %f, %f\n", pratoOnScreen[0] , pratoOnScreen[1], pratoOnScreen[2]);
		if ((pratoOnScreen[0] < pratoRaio && pratoOnScreen[0] > -pratoRaio) && (pratoOnScreen[1] < pratoRaio && pratoOnScreen[1] > -pratoRaio) && !pratoDead) {
			//printf("SHOOOT ON TARGET WELELELELELELELEEE\n");
			
			countTirosOnTraget++;
			pratoDead = true;
		}
	}
}

void onMotion(int x, int y) {
	if (x != windowWidth / 2 || y != windowHeight / 2) {
		float reduce = 3;
		cameraAngleY += (windowWidth / 2 - x) / reduce;
		cameraAngleX += (windowHeight / 2 - y) / reduce;
		if (cameraAngleX > 90)
			cameraAngleX = 90;
		if (cameraAngleX < -90)
			cameraAngleX = -90;
		mousePosXYZ[0] = sin(cameraAngleY * M_PI / 180);
		mousePosXYZ[2] = cos(cameraAngleY * M_PI / 180);
		mousePosXYZ[1] = sin(cameraAngleX * M_PI / 180);
		mousePosXYZ[0] *= cos(cameraAngleX * M_PI / 180);
		mousePosXYZ[2] *= cos(cameraAngleX * M_PI / 180);

		mouseScreenPosXY[0] = x;
		mouseScreenPosXY[1] = y;

		//printf("x: %f z: %f y: %f\n", mousePosXYZ[0], mousePosXYZ[2], mousePosXYZ[1]);
		//glutPostRedisplay();

		glutWarpPointer(windowWidth / 2, windowHeight / 2);
		//SetCursorPos(windowWidth / 2, windowHeight / 2);
	}
}



int main(int argc, char** argv) {
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
	glutInitWindowSize(windowWidth, windowHeight);
	glutInitWindowPosition(600, 50);
	glutCreateWindow("jpcorr@student.dei.uc.pt ::::::::::::::: Terrain (Perlin Noise) ");
	//glutFullScreen();
	init();
	glViewport(0, 0, (GLsizei)windowWidth, (GLsizei)windowHeight);
	InitApp("model/autorifle.obj");
	glutDisplayFunc(desenha);
	glutKeyboardFunc(teclado);
	glutKeyboardUpFunc(tecladoUp);
	glutMouseFunc(onMouse);
	glutPassiveMotionFunc(onMotion);
	glutSpecialFunc(teclasNotAscii);
	glutTimerFunc(60, timer, 1);

	glutSetCursor(GLUT_CURSOR_NONE);

	windowWidth = glutGet(GLUT_WINDOW_WIDTH);
	windowHeight = glutGet(GLUT_WINDOW_HEIGHT);
	glutWarpPointer(windowWidth / 2, windowHeight / 2);
	mouseScreenPosXY[0] = windowWidth / 2;
	mouseScreenPosXY[1] = windowHeight / 2;
	glutMainLoop();

	

	return 0;
}