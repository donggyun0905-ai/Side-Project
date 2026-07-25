#include <GL/glut.h>
#include <cmath>     
#include <cstdlib>       
#include <string>
#include <cstdio>  

using namespace std;

#define PI 3.14         
#define Window_Width 1200   
#define Window_Height 1200  

// 공 위치 및 속도
typedef struct Vector {
    float x;            
    float y;             
} vec;

typedef struct Point {
    float x;          
    float y;              
} point;

// 공 구조체
typedef struct Ball {
    point center; 
    vec velocity; 
    float radius;     
    bool isMoving;           // 공이 움직이고 있는지 여부 (true면 움직임)
} ball;

ball theBall; //게임에서 사용하는 공 객체

point paddle1[3];   //첫 번째 곡선 패들
point paddle2[3];   // 두 번째 곡선 패들

float paddleAngle = 0.0f;   // 곡선 패들 회전 각도

// 점수판 변수
int score = 0;      
int highscore = 0;    
int lives = 5;
bool gameWon = false;     // 승리 상태 플래그
bool effectVisible = false; // 이펙트(메시지) 출력 여부
bool ballMoving = false;  // 공 움직임 여부

bool isPaused = false;  // 일시정지

float paddlerange = 1.0;

// 벽돌 구조체
typedef struct Brick {
    float x, y;              
    float width, height;   
    bool isVisible;        
    float r, g, b;       
} brick;

brick bricks[6][4];     

// 벽돌 초기화 함수
void initBricks() {
    float startX = -0.26;   
    float startY = 0.13;    
    float gap = 0.01;       
    float brickWidth = 0.12;
    float brickHeight = 0.05;

    // 점수 초기화 코드(score=0) 삭제됨
    for (int i = 0; i < 6; i++) {       
        for (int j = 0; j < 4; j++) {   
            if ((i + j == 0) || (i + j == 8) || (i == 0 && j == 3) || (i == 5 && j == 0)) continue;

            bricks[i][j].x = startX + j * (brickWidth + gap);
            bricks[i][j].y = startY - i * (brickHeight + gap);

            bricks[i][j].width = brickWidth;
            bricks[i][j].height = brickHeight;

            bricks[i][j].isVisible = true; 

            bricks[i][j].r = (float)(rand() % 256) / 255.0f;
            bricks[i][j].g = (float)(rand() % 256) / 255.0f;
            bricks[i][j].b = (float)(rand() % 256) / 255.0f;
        }
    }
}

// 초기화 함수
void init() {
    theBall.center.x = 0.0f;  
    theBall.center.y = -0.23f; 
    theBall.radius = 0.03f;   
    theBall.velocity.x = 0.0f;  
    theBall.velocity.y = 0.0f;  
    theBall.isMoving = false;  

    paddle1[0] = { -0.5, -0.4 };
    paddle1[1] = { 0.0, -0.84 };
    paddle1[2] = { 0.5, -0.4 };

    paddle2[0] = { -0.5, 0.4 };
    paddle2[1] = { 0.0, 0.84 };
    paddle2[2] = { 0.5, 0.4 };

    initBricks();  
}

// 팩토리얼
float factorial(int t, int n) {
    float r = 1;
    for (int i = 0; i < n; i++) {
        r *= (t - i);   
        r /= (i + 1);     
    }
    return r;
}

//회전 함수
point rotatePoint(point p, float angle) {
    float s = sin(angle);   // 회전 각도에 대한 사인값
    float c = cos(angle);   // 회전 각도에 대한 코사인값
    point result;
    result.x = p.x * c - p.y * s; 
    result.y = p.x * s + p.y * c;  
    return result;      
}

// 곡선 패들을 그리는 함수 (베지에 곡선, 회전 포함)
void drawPaddleArc(point paddle[]) {
    glColor3f(1.0, 1.0, 1.0);  

    // 회전된 제어점 배열 생성
    point rotated[3];
    for (int i = 0; i < 3; i++) {
        rotated[i] = rotatePoint(paddle[i], paddleAngle);// 각 제어점 회전
    }

    glBegin(GL_LINE_STRIP);
    for (float t = 0.0; t <= paddlerange; t += 0.01f) {
        float x = 0.0f;
        float y = 0.0f;
        for (int i = 0; i < 3; i++) {
            float r = factorial(2, i); 
            x += r * pow(1 - t, 2 - i) * pow(t, i) * rotated[i].x;
            y += r * pow(1 - t, 2 - i) * pow(t, i) * rotated[i].y;
        }
        glVertex2f(x, y);  
    }
    glEnd(); 
}

// 공 그리는 함수
void drawBall() {
    glColor3f(1.0, 0.0, 0.0);  
    glBegin(GL_POLYGON); 
    float angle = 2 * PI / 30;  
    for (int i = 0; i <= 30; i++) {

        float x = theBall.center.x + cos(angle * i) * theBall.radius;
        float y = theBall.center.y + sin(angle * i) * theBall.radius;
        glVertex2f(x, y);
    }
    glEnd();             
}

void drawpoint(float cx, float cy, float r, int num_segments) {
    glBegin(GL_TRIANGLE_FAN);
    for (int i = 0; i <= num_segments; i++) {
        float theta = 2.0f * 3.1415926f * float(i) / float(num_segments);
        float x = r * cosf(theta);
        float y = r * sinf(theta);
        glVertex2f(cx + x, cy + y);
    }
    glEnd();
}

void drawLives() {
    float startX = -0.202;  
    float y = 0.85;   
    float spacing = 0.1f;    // 원 간격
    float radius = 0.035f;     // 원 크기

    glColor3f(0.0f, 1.0f, 0.2f); 
    for (int i = 0; i < lives; i++) {
        drawpoint(startX + i * spacing, y, radius, 20);
    }
}

// 벽돌 그리기 함수
void drawBricks() {
    for (int i = 0; i < 6; i++) {              
        for (int j = 0; j < 4; j++) {         
            if (bricks[i][j].isVisible) {     
                glColor3f(bricks[i][j].r, bricks[i][j].g, bricks[i][j].b); 
                glBegin(GL_POLYGON);            
                glVertex2f(bricks[i][j].x, bricks[i][j].y);                             // 왼쪽 아래 점
                glVertex2f(bricks[i][j].x + bricks[i][j].width, bricks[i][j].y);       // 오른쪽 아래 점
                glVertex2f(bricks[i][j].x + bricks[i][j].width, bricks[i][j].y + bricks[i][j].height); // 오른쪽 위 점
                glVertex2f(bricks[i][j].x, bricks[i][j].y + bricks[i][j].height);      // 왼쪽 위 점
                glEnd();                  
            }
        }
    }
}

// 공과 벽돌 충돌 체크
void checkBrickCollision() {
    for (int i = 0; i < 6; i++) {            
        for (int j = 0; j < 4; j++) {           
            Brick& b = bricks[i][j];            
            if (!b.isVisible) continue;         

            bool collisionX = theBall.center.x + theBall.radius > b.x &&
                theBall.center.x - theBall.radius < b.x + b.width;

            bool collisionY = theBall.center.y + theBall.radius > b.y &&
                theBall.center.y - theBall.radius < b.y + b.height;

            if (collisionX && collisionY) {   
                // 공의 각 면 좌표 계산
                float ballLeft = theBall.center.x - theBall.radius;
                float ballRight = theBall.center.x + theBall.radius;
                float ballTop = theBall.center.y + theBall.radius;
                float ballBottom = theBall.center.y - theBall.radius;

                // 벽돌 각 면 좌표 계산
                float brickLeft = b.x;
                float brickRight = b.x + b.width;
                float brickTop = b.y + b.height;
                float brickBottom = b.y;

                bool reflected = false;

                // 공이 벽돌 왼쪽 면과 충돌 시
                if (!reflected && theBall.velocity.x < 0 && ballLeft < brickRight && ballRight > brickRight) {
                    theBall.velocity.x *= -1; 
                    theBall.center.x = brickRight + theBall.radius + 0.001f;
                    reflected = true; 
                }
                // 공이 벽돌 오른쪽 면과 충돌 시
                if (!reflected && theBall.velocity.x > 0 && ballRight > brickLeft && ballLeft < brickLeft) {
                    theBall.velocity.x *= -1;   
                    theBall.center.x = brickLeft - theBall.radius - 0.001f;
                    reflected = true;           
                }
                // 공이 벽돌 상단 면과 충돌 시
                if (!reflected && theBall.velocity.y > 0 && ballTop > brickBottom && ballBottom < brickBottom) {
                    theBall.velocity.y *= -1; 
                    theBall.center.y = brickBottom - theBall.radius - 0.001f; // 공 위치 보정
                    reflected = true;             
                }
                // 공이 벽돌 하단 면과 충돌 시
                if (!reflected && theBall.velocity.y < 0 && ballBottom < brickTop && ballTop > brickTop) {
                    theBall.velocity.y *= -1;  
                    theBall.center.y = brickTop + theBall.radius + 0.001f; // 공 위치 보정
                    reflected = true;           
                }

                if (reflected) {              
                    b.isVisible = false;     
                    score += 10;                 
                }
            }
        }
    }

}

// 모든 벽돌이 깨졌는지 체크하는 함수
bool allBricksBroken() {
    for (int i = 0; i < 6; i++) {          
        for (int j = 0; j < 4; j++) {        
            if (bricks[i][j].isVisible) {   
                return false;                 
            }
        }
    }
    return true;                             
}

// 곡선 패들과 공 충돌 체크 함수
void checkPaddleCollision() {
    for (int p = 0; p < 2; p++) {                 // 두 패들 반복 검사
        point* paddle = (p == 0) ? paddle1 : paddle2; 
        point rotated[3];             
        for (int i = 0; i < 3; i++)          
            rotated[i] = rotatePoint(paddle[i], paddleAngle);

        for (float t = 0.0; t <= paddlerange; t += 0.01f) { 
            float x = 0.0f, y = 0.0f;      
            for (int i = 0; i < 3; i++) {
                float coeff = factorial(2, i);  
                x += coeff * pow(1 - t, 2 - i) * pow(t, i) * rotated[i].x;
                y += coeff * pow(1 - t, 2 - i) * pow(t, i) * rotated[i].y;
            }
            float dx = theBall.center.x - x;    
            float dy = theBall.center.y - y;   
            float dist = sqrt(dx * dx + dy * dy);


            if (dist < theBall.radius) {          // 공과 곡선 충돌 시
                // 법선 벡터 구하기 (접선에 수직인 벡터)
                float nextT = t + 0.01f;   
                if (nextT > 1.0f) nextT = 1.0f; 

                float nextX = 0.0f, nextY = 0.0f;
                for (int i = 0; i < 3; i++) {
                    float coeff = factorial(2, i);
                    nextX += coeff * pow(1 - nextT, 2 - i) * pow(nextT, i) * rotated[i].x;
                    nextY += coeff * pow(1 - nextT, 2 - i) * pow(nextT, i) * rotated[i].y;
                }
                float tangentX = nextX - x;       // 접선 벡터
                float tangentY = nextY - y;  
                float length = sqrt(tangentX * tangentX + tangentY * tangentY);
                tangentX /= length;             
                tangentY /= length;             

                float normalX = -tangentY;        // 법선 벡터 
                float normalY = tangentX;   

                float dot = theBall.velocity.x * normalX + theBall.velocity.y * normalY;
                theBall.velocity.x -= 2 * dot * normalX;
                theBall.velocity.y -= 2 * dot * normalY;


                float overlap = theBall.radius - dist;
                theBall.center.x += normalX * overlap;
                theBall.center.y += normalY * overlap;

                return;    
            }
        }
    }
}


void drawScore() {
    glColor3f(1.0, 1.0, 1.0); 
    glRasterPos2f(-0.9f, 0.9f); 

    char buffer[50];
#ifdef _MSC_VER
    sprintf_s(buffer, "Score: %d", score); 
#else
    sprintf(buffer, "Score: %d", score);
#endif

    for (int i = 0; buffer[i] != '\0'; i++) { 
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, buffer[i]);
    }
}

void drawhighScore() {
    if (score > highscore) {
        highscore = score;
    }

    glColor3f(1.0, 1.0, 0.0);
    glRasterPos2f(-0.9f, 0.85f); 

    char buffer[50];
#ifdef _MSC_VER
    sprintf_s(buffer, "High Score: %d", highscore);
#else
    sprintf(buffer, "High Score: %d", highscore); 
#endif

    for (int i = 0; buffer[i] != '\0'; i++) { 
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, buffer[i]); 
    }
}

void checkGameWon() {
    if (score >= highscore) {
        gameWon = true;
        effectVisible = true;
    }
}

//공 위치 초기화
void updateBall() {
    if (!theBall.isMoving) return;

    float gravityAmount = 0.00015f; // 중력

    float cx = theBall.center.x;
    float cy = theBall.center.y;

    // 공 위치에 따른 중력 방향 적용 (사분면 별)
    if (cx >= 0 && cy >= 0) {  // 1사분면
        theBall.velocity.x += gravityAmount;
        theBall.velocity.y += gravityAmount;
    }
    else if (cx < 0 && cy >= 0) {  // 2사분면
        theBall.velocity.x -= gravityAmount; 
        theBall.velocity.y += gravityAmount;
    }
    else if (cx < 0 && cy < 0) {  // 3사분면
        theBall.velocity.x -= gravityAmount;
        theBall.velocity.y -= gravityAmount;
    }
    else {  // 4사분면
        theBall.velocity.x += gravityAmount;
        theBall.velocity.y -= gravityAmount; 
    }

    theBall.center.x += theBall.velocity.x;
    theBall.center.y += theBall.velocity.y;

    // 공이 좌우 경계를 벗어나면 초기화
    if (theBall.center.x < -1.0f + theBall.radius || theBall.center.x > 1.0f - theBall.radius) {
        theBall.isMoving = false;         
        theBall.center.x = 0.0f;         
        theBall.center.y = -0.23f;      
        theBall.velocity.x = 0.0f;        
        theBall.velocity.y = 0.0f;    
        lives -= 1;
    }
    // 공이 상단 경계를 벗어나면 초기화
    if (theBall.center.y > 1.0f - theBall.radius) {
        theBall.isMoving = false;      
        theBall.center.x = 0.0f;     
        theBall.center.y = -0.23f;
        theBall.velocity.x = 0.0f;      
        theBall.velocity.y = 0.0f;
        lives -= 1;
    }
    // 공이 하단 경계를 벗어나면 초기화
    if (theBall.center.y < -1.0f + theBall.radius) {
        theBall.isMoving = false;  
        theBall.center.x = 0.0f;     
        theBall.center.y = -0.23f;
        theBall.velocity.x = 0.0f;    
        theBall.velocity.y = 0.0f;
        lives -= 1;
    }
    //게임오버
    if (lives <= 0) {
        checkGameWon();
        theBall.center.x = 0.0f;
        theBall.center.y = -0.23f;
        theBall.velocity.x = 0.0f;
        theBall.velocity.y = 0.0f;
        theBall.isMoving = false;
        score = 0;
        lives = 5; 
        initBricks(); 
        isPaused = false; 
    }

    checkBrickCollision(); 
    checkPaddleCollision();

    if (allBricksBroken()) {
        initBricks();   
    }
}

float glowIntensity = 0.0f;
bool glowIncreasing = true;

void drawWinEffect() {
    const char* msg = "You Win!";

    // 반짝임 강도 조절
    if (glowIncreasing) {
        glowIntensity += 0.02f;
        if (glowIntensity >= 1.0f) glowIncreasing = false;
    }
    else {
        glowIntensity -= 0.02f;
        if (glowIntensity <= 0.0f) glowIncreasing = true;
    }

    float alpha = 0.5f + 0.5f * glowIntensity;  // 반짝임 투명도

    // 기본 색상 (노란색, 투명도 포함)
    glColor4f(1.0f, 1.0f, 0.0f, alpha);

    float baseX = -0.2f;
    float baseY = 0.02f;

    // 여러 위치에 글자를 겹쳐서 두껍게 표현
    float offsets[5][2] = {
        {0.0f, 0.0f},
        {0.002f, 0.0f},
        {-0.002f, 0.0f},
        {0.0f, 0.002f},
        {0.0f, -0.002f},
    };

    for (int o = 0; o < 5; o++) {
        for (int i = 0; msg[i] != '\0'; i++) {
            glRasterPos2f(baseX + i * 0.06f + offsets[o][0], baseY + offsets[o][1]);
            glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, msg[i]);
        }
    }
}

void display() {
    glClearColor(0, 0, 0, 0);     
    glClear(GL_COLOR_BUFFER_BIT);   

    glColor3f(1.0f, 1.0f, 1.0f);  
    glBegin(GL_LINES);           
    glVertex2f(0.0f, -1.0f);     
    glVertex2f(0.0f, 1.0f);   
    glVertex2f(-1.0f, 0.0f);    
    glVertex2f(1.0f, 0.0f);     
    glEnd();                     

    drawScore();                 
    drawhighScore();
    drawLives();
    drawBricks();              
    drawBall();               
    drawPaddleArc(paddle1);
    drawPaddleArc(paddle2);
    if (effectVisible) {
        drawWinEffect();
    }

    glutSwapBuffers();   
}


void timer(int) {
    if (!isPaused && theBall.isMoving) {
        updateBall();
    }

    glutPostRedisplay();
    glutTimerFunc(16, timer, 0);  // 약 16ms 후 다시 타이머 호출 (60fps)
}


void keyboard(unsigned char key, int x, int y) {
    if (key == ' ') {
        if (!theBall.isMoving) {
            theBall.center.x = 0.0f;
            theBall.center.y = -0.23f;
            theBall.velocity.x = 0.01f;
            theBall.velocity.y = 0.01f;
            gameWon = false;
            effectVisible = false;
            theBall.isMoving = true;
            isPaused = false; 
        }
        else {
            isPaused = !isPaused;
        }
    }
    else if (key == 'r') {
        theBall.center.x = 0.0f;
        theBall.center.y = -0.23f;
        theBall.velocity.x = 0.0f;
        theBall.velocity.y = 0.0f;
        theBall.isMoving = false;
        score = 0;
        lives = 5; 
        initBricks();
        isPaused = false;
    }
}

void specialKeys(int key, int, int) {
    if (key == GLUT_KEY_LEFT) paddleAngle -= 0.1; 
    else if (key == GLUT_KEY_RIGHT) paddleAngle += 0.1;
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);                      
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);  
    glutInitWindowSize(Window_Width, Window_Height);
    glutCreateWindow("무한의 벽돌");  

    init();  

    glutDisplayFunc(display);       
    glutKeyboardFunc(keyboard);      
    glutSpecialFunc(specialKeys);     
    glutTimerFunc(0, timer, 0);     
    glutMainLoop();                   
    return 0;              
}
