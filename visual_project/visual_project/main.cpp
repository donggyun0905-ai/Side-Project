// main.cpp
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <cstdio>
#include "Scene.h"
#include "ShowControl.h"
#include "Controller.h"

#define WIDTH 800
#define HEIGHT 600

Scene g_scene;
ShowControl g_show;
Controller g_controller;

int g_prevTime = 0;

void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    g_scene.render();

    glutSwapBuffers();
}

void idle() {
    int current = glutGet(GLUT_ELAPSED_TIME);
    float dt = (current - g_prevTime) / 1000.0f;
    g_prevTime = current;
    if (dt < 0.0f) dt = 0.0f;

    g_show.update(dt, g_scene);
    g_scene.update(dt);

    glutPostRedisplay();
}

void reshape(int w, int h) {
    glViewport(0, 0, w, h);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, (float)w / (float)h, 0.1, 100.0);
}

void specialKey(int key, int x, int y) {
    g_controller.onSpecialKey(key);
    glutPostRedisplay();
}

void keyboard(unsigned char key, int x, int y) {
    if (key == 27) { // ESC
        std::exit(0);
    }
    g_controller.onKey(key);
    glutPostRedisplay();
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
    glutInitWindowSize(WIDTH, HEIGHT);
    glutCreateWindow("Drone Show Skeleton");

    glewInit();

    glEnable(GL_DEPTH_TEST);

    g_scene.init();
    g_controller.attachCamera(&g_scene.camera);

    g_prevTime = glutGet(GLUT_ELAPSED_TIME);

    glutDisplayFunc(display);
    glutIdleFunc(idle);
    glutReshapeFunc(reshape);
    glutSpecialFunc(specialKey);
    glutKeyboardFunc(keyboard);

    glutMainLoop();
    return 0;
}
