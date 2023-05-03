#define RLIGHTS_IMPLEMENTATION

#include "raylib.h"
#include "rlights.h"
#include "orbital_camera.h"
#include "bvh.h"
#include <iostream>
#include <string>

#define screenWidth 800
#define screenHeight 600
#define textWidth 1024
#define textHeight 1024

void DrawRefFrame()
{
    DrawSphere(Vector3{0.0, 0.0, 0.0}, 0.4f, BLACK);
    DrawLine3D(Vector3{0.0, 0.0, 0.0}, Vector3{10.0, 0.0, 0.0}, RED);
    DrawLine3D(Vector3{0.0, 0.0, 0.0}, Vector3{0.0, 10.0, 0.0}, BLUE);
    DrawLine3D(Vector3{0.0, 0.0, 0.0}, Vector3{0.0, 0.0, 10.0}, GREEN);
}

float GetMouseBrushSize(float brush_sz, float distance, float reference)
{
    float mouse_brush_size = (float)(brush_sz*(200-distance)*reference);

    if (mouse_brush_size < 4.0f) mouse_brush_size = 4.0f;

    return mouse_brush_size;
}

void DrawOnTexture(RenderTexture2D& texture, float brush_sz, Color brush_color, 
                   Camera& camera, BVH& tree)
{
    Ray ray = GetMouseRay(GetMousePosition(), camera);
    RayCollisionUV results = tree.Search(ray);
    if (results.hit)
    {
        // Convert barrycentric coordinates to UV texture coordinates
        Vector2 uv = results.getUV();
        int x = (int)(uv.x*texture.texture.width);
        int y = (int)(texture.texture.height-(uv.y*texture.texture.height));
        
        // Draw Here
        BeginTextureMode(texture);
        DrawCircle(x,y, brush_sz, brush_color);
        EndTextureMode();
        
    }
}

int main(int argc, char *argv[])
{
    
    if (argc < 3)
    {
        std::cout << ".exe \"path to model\" \"directory to shaders\" "<<std::endl;
        return 1;
    }
    std::string shader_dir = argv[2];
    std::string filename = argv[1];

    InitWindow(screenWidth, screenHeight, "Model Viewer");
    SetTargetFPS(60);
    HideCursor();

    // Setup the model
    Model model = LoadModel(filename.c_str());
    if (model.meshCount == 0)
    {
        std::cout << "Could not find the model: "<< filename << std::endl;
        return 1;
    }
    Vector3 model_position = {0.0, 0.0, 0.0};
    BoundingBox model_bbox = GetModelBoundingBox(model);

    // Setup the LightPoints
    Shader shader = LoadShader((shader_dir + "vs.glsl").c_str(), (shader_dir + "fs.glsl").c_str());
    
    shader.locs[SHADER_LOC_VECTOR_VIEW] = GetShaderLocation(shader, "viewPos");
    shader.locs[SHADER_LOC_MATRIX_MODEL] = GetShaderLocation(shader, "matModel");
    const int ambient_loc = GetShaderLocation(shader, "ambient");
    const float light_value[4] = {0.1f, 0.1f, 0.1f, 1.0f};
    SetShaderValue(shader, ambient_loc, light_value, SHADER_UNIFORM_VEC4);
    Light light_source = CreateLight(LIGHT_DIRECTIONAL, Vector3{120.0, 120.0, 120.0},
                                     Vector3{0.0, 0.0, 0.0}, WHITE, shader);

    // Create Blank image and then load a texture with it
    Image image = GenImageColor(textWidth, textHeight, WHITE);
    Texture texture = LoadTextureFromImage(image);
    // Create a texture that can be drawn
    RenderTexture2D render_texture = LoadRenderTexture(1024, 1024);
    // Draw the original texture onto the one we are going to drawn
    BeginTextureMode(render_texture);
    DrawTextureRec(texture, 
                   Rectangle{0, (float)texture.height, (float)texture.width,-(float)texture.height},
                   Vector2{0,0}, WHITE);
    EndTextureMode();
    model.materials[MATERIAL_MAP_ALBEDO].maps[MATERIAL_MAP_DIFFUSE].texture = render_texture.texture;
    model.materials[MATERIAL_MAP_ALBEDO].shader = shader;

    // Construct BVH model Tree
    const int triangleCount = model.meshes[0].triangleCount;
    BVH model_tree = BVH(model.meshes[0], model.transform);

    // Define the camera to look into our 3d world
    ORBITAL_CAMERA orbital_camera;
    orbital_camera.target = Vector3{(model_bbox.max.x-model_bbox.min.x)/2,
                                    (model_bbox.max.y-model_bbox.min.y)/2,
                                    (model_bbox.max.z-model_bbox.min.z)/2};
    orbital_camera.update();

    // Create red brush
    Color brush_color = RED;
    float brush_sz = 0.8f;
    const float brush_reference = 0.08f;
    float mouse_brush_sz = GetMouseBrushSize(brush_sz, orbital_camera.distance, brush_reference);
    bool draw_it = false;

    while(!WindowShouldClose())
    {
        // Update Camera
        orbital_camera.loop();
        mouse_brush_sz = GetMouseBrushSize(brush_sz, orbital_camera.distance, brush_reference);
        // Update Shaders
        light_source.position = orbital_camera.position;
        UpdateLightValues(shader, light_source);

        BeginDrawing();
        ClearBackground(GRAY);
        BeginMode3D(orbital_camera.camera);
        // Draw Model
        DrawModel(model, model_position, 1.0f, WHITE);
        DrawRefFrame();
        EndMode3D();

        // Resize drawing brush
        if(IsKeyDown(KEY_A))
        {
            brush_sz += 0.1f;
            if (brush_sz > 2.5f) brush_sz = 2.5f;
        }
        if(IsKeyDown(KEY_Z))
        {
            brush_sz -= 0.1f;
            if (brush_sz < 0.8f) brush_sz = 0.8f;
        }

        // Draw Color on texture
        // Q for drawing RED and W to erase with WHITE color
        if(IsKeyDown(KEY_Q))
        {
            brush_color = {255, 0, 0, 255};
            draw_it = true;
        }
        if(IsKeyDown(KEY_W)) 
        {
            brush_color = WHITE;
            draw_it = true;
        }
        if (draw_it)
        {
            DrawOnTexture(render_texture, brush_sz, brush_color, 
                          orbital_camera.camera, model_tree);
            draw_it = false;
        }
        

        DrawCircle((int)GetMousePosition().x, (int)GetMousePosition().y, 
                   mouse_brush_sz, brush_color);
        EndDrawing();

    }

    Image output_image = LoadImageFromTexture(render_texture.texture);
    ExportImage(output_image, "output_diffuse.png");

    // Release memory
    UnloadImage(image);
    UnloadImage(output_image);
    UnloadRenderTexture(render_texture);
    UnloadTexture(texture);
    UnloadModel(model);
    UnloadShader(shader);
    CloseWindow();

    return 0;
}

