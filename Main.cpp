#include <cmath>
#include <algorithm>
#include <vector>
#include <ctime>
#include "raylib.h"

static constexpr float FPS = 60.0f;
static constexpr float WINDOW_WIDTH = 2560.0f;
static constexpr float WINDOW_HEIGHT = 1440.0f;

//#include "Color_coding.h"
#include "Fluid.h"

int main(void)
{
    //  window init

    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Fluid_simulation_V1.0");
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    SetTargetFPS(FPS);
    ToggleBorderlessWindowed();
    
    //  time variables

    float delta_time = 0.0f;

    //  grid init

    Fluid_grid fluid_grid;

    while (!WindowShouldClose())
    {
        //  fluid computation

        delta_time = GetFrameTime();

        //fluid_grid.add_gravity(delta_time);
        
        fluid_grid.velocity_setter(delta_time);

        //if (IsKeyDown(KEY_S))
        {
            for (size_t i = 0; i < 20; i++)
            {
                fluid_grid.solve_incompressibility();
            }
        }
        


        //  rendering

        BeginDrawing();
        {
            ClearBackground(DARKGRAY);

            fluid_grid.draw_cell_grid();
            fluid_grid.draw_interpolated_divergence_vectors();
            //fluid_grid.draw_divergence_vectors();
            //fluid_grid.draw_divergence_values();

            DrawText(TextFormat("FPS %i", GetFPS()), 10, 10, 30, BLUE);
        }
        EndDrawing();
    }

    CloseWindow();

    return 0;
}