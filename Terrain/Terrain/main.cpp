#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <time.h>

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

float mapa32[32 * 32];
float mapa256[256 * 256];
int windowWidth = 800, windowHeight = 600;
float check;

GLfloat playerPosXY[2] = {0, 0};
GLfloat mouseScreenPosXY[2] = {0, 0};
GLfloat mouseScreenPosXYZ[3] = {0, 0, 0};
GLfloat cameraAngleY = 90.0;
GLfloat cameraAngleX = 0;

/* INTRODUÇÃO DE NEVOEIRO */
GLfloat nevoeiroCor[] = {0.75, 0.75, 0.75, 1.0}; // definição da cor do nevoeiro


float noise(int x, int y, int aleatorio) {
	int n = x + y * 57 + aleatorio * 131;
	n = (n << 13) ^ n;
	return (1.0f - ((n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff) / 1073741824.0f);
}

void setNoise(float  *mapa) {
	time_t t;
	float temp[34][34];
	int aleatorio = rand() % 5000;

	srand((unsigned)time(&t));

	for (int y = 1; y<33; y++)
		for (int x = 1; x<33; x++)
			temp[x][y] = 128.0f + noise(x, y, aleatorio)*10.0f;

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

void loop() {
	sobrepoeOitavas(mapa32, mapa256);
	expFilter(mapa256);
}

/* FUNÇÃO QUE INICIALIZA O NEVOEIRO */
void initNevoeiro(void){
    glFogfv(GL_FOG_COLOR, nevoeiroCor); //Cor do nevoeiro
    glFogi(GL_FOG_MODE, GL_LINEAR); //Equação do nevoeiro - linear
    //Outras opcoes: GL_EXP, GL_EXP2
    glFogf(GL_FOG_START, 200.0); // Distância a que terá início o nevoeiro
    glFogf(GL_FOG_END, 1.0); // Distância a que o nevoeiro terminará
    //glFogf (GL_FOG_DENSITY, 0.35); //Densidade do nevoeiro - não se especifica quando temos "nevoeiro linear"
    
    
}
void init() {
	//glClearColor(0, 0.75, 1, 1);
    
    /* enable ao nevoeiro */
    glEnable(GL_FOG);
    
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE );
    
    /* enable ao culling*/
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);
    initNevoeiro();
	setNoise(mapa32);
    
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHT1);
    glEnable(GL_DEPTH_TEST);
    
    loop();
    
}

void desenhaReferencial() {
	glColor4f(RED);
	glBegin(GL_LINES);
		glVertex3f(-512.0, 0.0, 0.0);
		glVertex3f(512.0, 0.0, 0.0);
	glEnd();
	glColor4f(GREEN);
	glBegin(GL_LINES);
		glVertex3f(0.0, -512.0, 0.0);
		glVertex3f(0.0, 512.0, 0.0);
	glEnd();
	glColor4f(BLUE);
	glBegin(GL_LINES);
		glVertex3f(0.0, 0.0, -512.0);
		glVertex3f(0.0, 0.0, 512.0);
	glEnd();
}

void desenhaObjecto() {
	GLfloat   bule = 1;
	GLfloat   buleP[] = { 0, 0, 0 };
	glTranslatef(buleP[0], buleP[1], buleP[2]);
	glColor4f(ORANGE);//Definir previamente o LARANJA (#define LARANJA Ö)
	glutSolidTeapot(bule); 
}

void desenhaTerreno() {
	for (int i = 0; i < 256 - 1; i++) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glShadeModel(GL_SMOOTH);
		//glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glBegin(GL_TRIANGLE_STRIP);
			for (int j = 0; j < 256; j++) {
				float tmp = mapa256[i * 256 + j] / 100;
				glColor3f(tmp, tmp, tmp);
				glVertex3f((i - 128) * 4.0, (mapa256[i * 256 + j] / 256.0) * 70, (j - 128) * 4.0);
				tmp = mapa256[(i + 1) * 256 + j] / 100;
				glColor3f(tmp, tmp, tmp);
				glVertex3f((i - 127) * 4.0, (mapa256[(i + 1) * 256 + j] / 256.0) * 70, (j - 128) * 4.0);
			}
		glEnd();
		glShadeModel(GL_FLAT);
	}
}

float billbilinearInterpolation(float x, float z) {
	x = x / 4 + 128;
	z = z / 4 + 128;
	int x1 = (int) x;
	int x2 = (int) x + 1;
	int y1 = (int) z;
	int y2 = (int) z + 1;

	float q11 = mapa256[x1 * 256 + y1];
	float q12 = mapa256[x1 * 256 + y2];
	float q21 = mapa256[x2 * 256 + y1];
	float q22 = mapa256[x2 * 256 + y2];

	float r1 = ((x2 - x) / (x2 - x1)) * q11 + ((x - x1) / (x2 - x1)) * q21;
	float r2 = ((x2 - x) / (x2 - x1)) * q12 + ((x - x1) / (x2 - x1)) * q22;
	//printf("%f.2, %f.2 = %f.4\n", x, z, ((y2 - z) / (y2 - y1)) * r1 + ((z - y1) / (y2 - y1)) * r2);
	return ((y2 - z) / (y2 - y1)) * r1 + ((z - y1) / (y2 - y1)) * r2;
}


void desenha() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(65.0, (GLdouble)windowWidth / windowHeight, 1.0, 200.0);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	/*gluLookAt(playerPosXY[0], (billbilinearInterpolation(playerPosXY[0], playerPosXY[1]) / 256.0) * 70 + 2, playerPosXY[1], playerPosXY[0] + mouseScreenPosXYZ[0], (billbilinearInterpolation(playerPosXY[0], playerPosXY[1]) / 256.0) * 70 + 2 + mouseScreenPosXYZ[1], playerPosXY[1] + mouseScreenPosXYZ[2], 0.0, 1.0, 0.0);
     */
    gluLookAt(playerPosXY[0], (billbilinearInterpolation(playerPosXY[0], playerPosXY[1]) / 256.0) * 70 + 2, playerPosXY[1], playerPosXY[0] + mouseScreenPosXYZ[0], (billbilinearInterpolation(playerPosXY[0], playerPosXY[1]) / 256.0) * 70 + 2 + mouseScreenPosXYZ[1], playerPosXY[1] + mouseScreenPosXYZ[2], 0.0, 1.0, 0.0);
    
    
	/*desenhaReferencial();
	glPushMatrix();
		glRotatef(23, 0, 1, 0);
		glScalef(1, 1.4, 2);
		desenhaObjecto();
	glPopMatrix();*/

	desenhaTerreno();

	glutSwapBuffers();
}

void timer(int value) {
	glutPostRedisplay();
	glutTimerFunc(20, timer, 1);
}

void teclasNotAscii(int key, int x, int y)
{
	if (key == GLUT_KEY_LEFT) {
		playerPosXY[0]++;
		glutPostRedisplay();
	}
	if (key == GLUT_KEY_RIGHT) {
		playerPosXY[0]--;
		glutPostRedisplay();
	}
	if (key == GLUT_KEY_DOWN) {
		playerPosXY[1]--;
		glutPostRedisplay();
	}
	if (key == GLUT_KEY_UP) {
		playerPosXY[1]++;
		glutPostRedisplay();
	}
}

void teclado(unsigned char key, int x, int y) {
	switch (key) {
		case 27:
			exit(0);
			break;
		default:
			break;
	}
}

void onMouse(int button, int state, int x, int y) {
	/*if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
		arcball_on = true;
		last_mx = cur_mx = x;
		last_my = cur_my = y;
	}
	else {
		arcball_on = false;
	}*/
}

void onMotion(int x, int y) {
	if (x != windowWidth / 2 || y != windowHeight / 2) {
		cameraAngleY += windowWidth / 2 - x;
		cameraAngleX += windowHeight / 2 - y;
		if (cameraAngleX > 90)
			cameraAngleX = 90;
		if (cameraAngleX < -90)
			cameraAngleX = -90;
		mouseScreenPosXYZ[0] = sin(cameraAngleY * M_PI / 180);
		mouseScreenPosXYZ[2] = cos(cameraAngleY * M_PI / 180);
		mouseScreenPosXYZ[1] = sin(cameraAngleX * M_PI / 180);
		mouseScreenPosXYZ[0] *= cos(cameraAngleX * M_PI / 180);
		mouseScreenPosXYZ[2] *= cos(cameraAngleX * M_PI / 180);
		
		mouseScreenPosXY[0] = x;
		mouseScreenPosXY[1] = y;
		
		printf("x: %f z: %f y: %f\n", mouseScreenPosXYZ[0], mouseScreenPosXYZ[2], mouseScreenPosXYZ[1]);
		glutPostRedisplay();

		glutWarpPointer(windowWidth/2,windowHeight/2);
		//SetCursorPos(windowWidth / 2, windowHeight / 2);
	}
}

int main(int argc, char** argv) {
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
	glutInitWindowSize(windowWidth, windowHeight);
	glutInitWindowPosition(600, 50);
	glutCreateWindow("{pjmm,jpa}@dei.uc.pt-CG ::::::::::::::: Clouds (Perlin Noise) ");
	glutFullScreen();
	init();
	glViewport(0, 0, (GLsizei)windowWidth, (GLsizei)windowHeight);

	glutDisplayFunc(desenha);
	glutKeyboardFunc(teclado);
	glutMouseFunc(onMouse);
	glutPassiveMotionFunc(onMotion);
	glutSpecialFunc(teclasNotAscii);
	glutTimerFunc(60, timer, 1);

	glutSetCursor(GLUT_CURSOR_NONE);
	
	windowWidth = glutGet(GLUT_WINDOW_WIDTH);
	windowHeight = glutGet(GLUT_WINDOW_HEIGHT);
	glutMainLoop();
	
	
	return 0;
}