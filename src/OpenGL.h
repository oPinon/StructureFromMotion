
#include <iostream>
#include <GL/glew.h>
#include <GL/glut.h>
#include <vector>

using namespace std;

int currentW = 800;
int currentH = 600;

int targetFramerate = 60;

int mouseLastX, mouseLastY;
int viewRotX = 30, viewRotZ = 10, viewDist = 3;

int windowId;

vector<float> points;
vector<unsigned char> colors;

void init() {

	glClearColor(0.0, 0.0, 0.0, 1.0);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, 0, points.data());
	glEnableClientState(GL_COLOR_ARRAY);
	glColorPointer(3, GL_UNSIGNED_BYTE, 0, colors.data());
}

void display() {

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	gluLookAt(
		0, -viewDist, 0,
		0, 0, 0,
		0, 0, 1
		);

	if (viewRotX >= 360) { viewRotX -= 360; }
	if (viewRotZ >= 360) { viewRotZ -= 360; }

	glRotated(viewRotZ, 0, 0, 1);
	glRotated(viewRotX, 1, 0, 0);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	gluPerspective(50, ((double)currentW) / currentH, 0.1, 100);

	glDrawArrays(GL_POINTS, 0, points.size() / 3);

	glutSwapBuffers();
}

void idle() {

	static int nWaitUntil = glutGet(GLUT_ELAPSED_TIME);
	int nTimer = glutGet(GLUT_ELAPSED_TIME);
	if (nTimer >= nWaitUntil) {
		glutPostRedisplay();
		nWaitUntil = nTimer + (int)(1000.0f / targetFramerate);
	}
}

void keyboard(unsigned char key, int x, int y) {

	switch (key)
	{
	case 27: // escape
		glutDestroyWindow(windowId);
		exit(EXIT_SUCCESS);
		break;
	}

}

void keyboardSpecial(int key, int x, int y) {

}

void reshape(GLsizei w, GLsizei h) {

	currentW = w;
	currentH = h;
	glViewport(0, 0, w, h);
	display();
}

void mouseMove(int x, int y) {

	viewRotX += y - mouseLastY;
	viewRotZ += x - mouseLastX;
	mouseLastX = x;
	mouseLastY = y;
}

void mouseClick(int button, int state, int x, int y) {

	if (state == GLUT_DOWN) {

		switch (button)
		{
		case 3 :
			viewDist--; if (viewDist < 1) { viewDist = 1; }
			break;
		case 4 :
			viewDist++;
			break;
		default:
			mouseLastX = x;
			mouseLastY = y;
			break;
		}
	}
}

#include <fstream>
#include <sstream>

int OpenGLMain(int argc = 0, char *argv[] = NULL)
{
	// importing points
	ifstream in("BuildingNViewMatches.nvm");
	string line;
	getline(in, line);
	getline(in, line);
	getline(in, line);
	int nbPics = stoi(line);
	cout << nbPics << " views" << endl;
	for (int i = 0; i < nbPics; i++) {
		string path;
		getline(in, path, '	'); // weird delimiter (not actually a space)
		getline(in, line);
	}
	getline(in, line);
	getline(in, line);
	int nbPoints = stoi(line);
	cout << nbPoints << " points" << endl;
	points = vector<float>(3 * nbPoints);
	colors = vector<unsigned char>(3 * nbPoints);
	float x, y, z;
	int r, g, b;
	for (int i = 0; i < nbPoints; i++) {
		in >> x >> y >> z;
		points[3 * i + 0] = x;
		points[3 * i + 1] = y;
		points[3 * i + 2] = z;
		in >> r >> g >> b;
		colors[3 * i + 0] = r;
		colors[3 * i + 1] = g;
		colors[3 * i + 2] = b;
		getline(in, line);
	}

	// recentering
	float cX = 0, cY = 0, cZ = 0;
	for (int i = 0; i < nbPoints; i++) {
		cX += points[3 * i + 0];
		cY += points[3 * i + 1];
		cZ += points[3 * i + 2];
	}
	cX /= nbPoints;
	cY /= nbPoints;
	cZ /= nbPoints;
	float vX = 0, vY = 0, vZ = 0;
	for (int i = 0; i < nbPoints; i++) {
		points[3 * i + 0] -= cX;
		points[3 * i + 1] -= cY;
		points[3 * i + 2] -= cZ;
		vX += points[3 * i + 0] * points[3 * i + 0];
		vY += points[3 * i + 1] * points[3 * i + 1];
		vZ += points[3 * i + 2] * points[3 * i + 2];
	}
	vX = sqrt(vX / nbPoints);
	vY = sqrt(vY / nbPoints);
	vZ = sqrt(vZ / nbPoints);
	float ratio = sqrt((vX*vX + vY*vY + vZ*vZ) / 3);
	for (int i = 0; i < nbPoints; i++) {
		points[3 * i + 0] /= ratio;
		points[3 * i + 1] /= ratio;
		points[3 * i + 2] /= ratio;
	}

	glutInit(&argc, argv);

	glutInitWindowSize(currentW, currentH);

	windowId = glutCreateWindow("OpenGL window");

	glutDisplayFunc(display);
	glutIdleFunc(idle);
	glutKeyboardFunc(keyboard);
	glutSpecialFunc(keyboardSpecial);
	glutReshapeFunc(reshape);
	glutMotionFunc(mouseMove);
	glutMouseFunc(mouseClick);

	glewInit();

	init();

	glutMainLoop();

	return EXIT_SUCCESS;
}