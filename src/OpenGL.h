
#include <iostream>
#include <GL/glew.h>
#include <GL/glut.h>
#include <vector>
#include <unordered_map>
#include <string>

#include "Mesh.h"
#include "Shader.h"

#include <opencv2\opencv.hpp>

using namespace std;

int currentW = 800;
int currentH = 600;

int targetFramerate = 60;

int mouseLastX, mouseLastY;
float modelRotX = 30, modelRotZ = 10, viewRotX = 0, viewRotY = 0, viewDist = 3;

int windowId;
bool displayTriangles = true;
bool displayPoints = true;
bool isWireframe = false;
int camera = -1;
int currentImg = -1;
GLuint background;
float backgroundRatio = 1;
Shader bgShader;
GLuint texCoordsPos;
uint focalWidth;

Mesh mesh;

struct keyFunction {
	const string description;
	void (*function)(void);
};
unordered_map<char, keyFunction> keys = {
	{ 27 ,
		{ "Exits the program", [](void) {
			glutDestroyWindow(windowId);
			exit(EXIT_SUCCESS);
		} }
	},
	{ 't' ,
		{ "Switches triangles' display", [](void) {
			displayTriangles = !displayTriangles;
		} }
	},
	{ 'p' ,
		{ "Switches the point cloud's display", [](void) {
			displayPoints = !displayPoints;
		} }
	},
	{
		'w',
		{
			"Switches to wireframe mode", [](void) {
			isWireframe = !isWireframe;
			glPolygonMode(GL_FRONT_AND_BACK, isWireframe ? GL_LINE : GL_FILL);
		}
		}
	},
	{
		'c',
		{
			"Switches camera", [](void) {
			camera++; if (camera >= mesh.views.size()) { camera = -1; }

			// setting the background image
			// TODO :  mermory leak ?
			Mesh::CameraView& view = mesh.views[camera];
			cv::Mat image = cv::imread(view.imgPath);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, image.size().width, image.size().height, 0, GL_BGR, GL_UNSIGNED_BYTE, image.data);
			backgroundRatio = image.size().width / (float)image.size().height;
			focalWidth = max(image.size().width, image.size().height);
		}
		}
	},
	{
		'i',
		{
			"Displays the next view", [](void) {
			currentImg = (currentImg + 1) % mesh.views.size();
			Mesh::CameraView& view = mesh.views[currentImg];
			cv::Mat im = cv::imread(view.imgPath);
			for (const auto& pt : view.features) {
				cv::circle(im, {

					(int)(pt.x + im.size().width / 2),
					(int)(pt.y + im.size().height / 2)

				}, 10, cv::Scalar(255, 0, 0, 0.5), 5);
			}
			cv::resize(im, im, im.size() / 4);
			imshow("View", im); cv::waitKey(1);
		}
		}
	}
};

void init() {

	glClearColor(0.0, 0.0, 0.0, 1.0);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	// vertices
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, 0, mesh.vertices.data());
	glPointSize(3);

	// vertice colors
	glEnableClientState(GL_COLOR_ARRAY);
	glColorPointer(3, GL_UNSIGNED_BYTE, 0, mesh.colors.data());

	// background texture
	glGenTextures(1, &background);
	glBindTexture(GL_TEXTURE_2D, background);

	//cv::Mat image = cv::imread("armor0.jpg");
	//setBackground(image);
	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	bgShader = Shader("../../src/shaders/backgroundV.glsl", "../../src/shaders/backgroundF.glsl");
	texCoordsPos = bgShader.getAttribLocation("texCoordsV");
}

void display() {

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// displaying the background texture
	if (camera != -1) {

		float vX = ( backgroundRatio * currentH ) / currentW;
		bgShader.use();
		glDepthMask(GL_FALSE);
		glBegin(GL_QUADS);
			glVertexAttrib2f(texCoordsPos, 0, 1); glVertex2f(-vX, -1);
			glVertexAttrib2f(texCoordsPos, 1, 1); glVertex2f(vX, -1);	 
			glVertexAttrib2f(texCoordsPos, 1, 0); glVertex2f(vX, 1);		
			glVertexAttrib2f(texCoordsPos, 0, 0); glVertex2f(-vX, 1);
		glEnd();
		glDepthMask(GL_TRUE);
		glUseProgram(0);
	}

	// displaying the model
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	float fov = 50;
	if (camera == -1) {
		
		glRotated(viewRotX, 1, 0, 0);
		glRotated(viewRotY, 0, 1, 0);

		gluLookAt(
			0, -viewDist, 0,
			0, 0, 0,
			0, 0, 1
			);

		glRotated(modelRotZ, 0, 0, 1);
		glRotated(modelRotX, 1, 0, 0);
	}
	else {

		Mesh::CameraView& view = mesh.views[camera];
		// http://ccwu.me/vsfm/doc.html#basic
		Mesh::Vec3 up = view.orientation.transform({ 0,-1,0 });
		Mesh::Vec3 target = view.pos + view.orientation.transform({ 0,0,1 });
		gluLookAt(
			view.pos.x, view.pos.y, view.pos.z,
			target.x, target.y, target.z,
			up.x, up.y, up.z
			);
		fov = 35 * focalWidth / view.focal; // HACK 35mm ?
	}

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	gluPerspective(fov, ((double)currentW) / currentH, 0.1, 100);

	if (displayPoints) {
		glDrawArrays(GL_POINTS, 0, mesh.vertices.size() / 3);
	}

	if (displayTriangles) {
		glDrawElements(GL_TRIANGLES, mesh.triangleIndexes.size(), GL_UNSIGNED_INT, mesh.triangleIndexes.data());
	}

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

	auto function = keys.find(key);
	if (function != keys.end()) {
		function->second.function();
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

bool lastKeyIsLeft; // left or right

void mouseMove(int x, int y) {

	if (lastKeyIsLeft) {
		modelRotX += 0.2f*(y - mouseLastY);
		modelRotZ += 0.2f*(x - mouseLastX);
		if (modelRotX >= 360) { modelRotX -= 360; }
		if (modelRotZ >= 360) { modelRotZ -= 360; }
	}
	else {
		viewRotX += 0.2f*(y - mouseLastY);
		viewRotY += 0.2f*(x - mouseLastX);
		if (viewRotX >= 360) { viewRotX -= 360; }
		if (viewRotY >= 360) { viewRotY -= 360; }
	}
	mouseLastX = x;
	mouseLastY = y;
}

void mouseClick(int button, int state, int x, int y) {

	if (state == GLUT_DOWN) {

		switch (button)
		{
		case 3 : // scroll down
			viewDist-=0.3f; if (viewDist < 0.1f) { viewDist = 0.1f; }
			break;
		case 4 : // scroll up
			viewDist+=0.3f;
			break;
		case 0 : // left click
			lastKeyIsLeft = true;
			mouseLastX = x;
			mouseLastY = y;
			break;
		case 2: // right click
			lastKeyIsLeft = false;
			mouseLastX = x;
			mouseLastY = y;
			break;
		}
	}
}

int OpenGLMain(Mesh& m, int argc = 0, char *argv[] = NULL)
{
	cout << "Keys : " << endl;
	for (const auto& key : keys) {
		cout << "[ " << key.first << " ] : " << key.second.description << endl;
	}

	mesh = m;
	mesh.preRender();

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