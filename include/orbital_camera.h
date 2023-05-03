#include "raylib.h"
#include "raymath.h"

class ORBITAL_CAMERA
{
    private:
        bool refresh = false;
    public:
        float rot_angle = 45.0;
        float tilt_angle = -45.0;
        float distance = 200.0;
        const float rot_speed = 0.25;
        const float move_speed = 0.25;
        const float zoom_speed = 5.0;
        const float fovy = 45.0;
        Vector3 position = {120.0, 120.0, 120.0};
        Vector3 target = {0.0, 50.0, 0.0};
        const Vector3 up = {0.0, 1.0, 0.0};
        const int type = CAMERA_CUSTOM;
        Camera3D camera;

        void loop();
        void update();
        ORBITAL_CAMERA();
        ~ORBITAL_CAMERA();
};

void ORBITAL_CAMERA::update()
{
    this->camera.position = this->position;
    this->camera.target = this->target;
    this->camera.up = this->up;
    this->camera.fovy = this->fovy;
    this->camera.projection = this->type;
}

void ORBITAL_CAMERA::loop()
{
    this->refresh = false;
    Vector2 delta_mouse = GetMouseDelta();
    if (IsMouseButtonDown(0))
    {
        this->refresh = true;
        this->rot_angle += delta_mouse.x * this->rot_speed;
        this->tilt_angle += delta_mouse.y * this->rot_speed;

        if(this->tilt_angle > 89.9) this->tilt_angle = 89.9f;
        if(this->tilt_angle < -89.9) this->tilt_angle = -89.9f;
        
    }

    Vector3 move_vec = {0.0, 0.0, 0.0};
    if (IsMouseButtonDown(1))
    {
        this->refresh = true;
        move_vec.x = -delta_mouse.x * this->move_speed;
        move_vec.z = -delta_mouse.y * this->move_speed;
    }

    float wheel_move = GetMouseWheelMove();
    if (wheel_move != 0.0)
    {
        this->refresh = true;
        this->distance -= wheel_move * this->zoom_speed;
        if (this->distance < 1.0) this->distance = 1.0;
    }
    
    if (this->refresh)
    {
        Vector3 cam_pos = {0.0, 0.0, this->distance};

        Matrix T = MatrixRotateX(this->tilt_angle*GetFrameTime());
        Matrix R = MatrixRotateY(this->rot_angle*GetFrameTime());
        Matrix M = MatrixMultiply(T, R);
        cam_pos = Vector3Transform(cam_pos, M);
        move_vec = Vector3Transform(move_vec, R);

        this->target = Vector3Add(this->target, move_vec);
        this->position = Vector3Add(this->target, cam_pos);
        this->update();
        
    }
}
ORBITAL_CAMERA::ORBITAL_CAMERA()
{
    this->update();
}

ORBITAL_CAMERA::~ORBITAL_CAMERA()
{
}
