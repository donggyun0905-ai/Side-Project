#pragma once
class Scene;

class ShowControl {
public:
    ShowControl();
    void update(float dt, Scene& scene);

private:
    float time;
    bool textFormed;   // ★ 글자 포메이션이 이미 실행됐는지 체크하는 플래그
    bool  groupMoved[5];
};