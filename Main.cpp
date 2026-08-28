/*
    TO DO

    fix find_smoke_density function
    implement keybind to toggle smoke and velocity vectors

*/

#include <iostream>
#include <cmath>
#include <algorithm>
#include <vector>
#include <ctime>
#include "raylib.h"

static constexpr unsigned int FPS = 60;
static constexpr float WINDOW_WIDTH = 2560.0f;
static constexpr float WINDOW_HEIGHT = 1440.0f;

#include "Color_coding.h"
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

        fluid_grid.velocity_setter();
        fluid_grid.velocity_brush(delta_time);
        fluid_grid.blocked_cells_brush();
        //fluid_grid.smoke_setter();

        fluid_grid.solve_incompressibility();

        //  rendering

        BeginDrawing();
        {
            ClearBackground(BLACK);

            fluid_grid.draw_cell_grid();
            fluid_grid.draw_interpolated_divergence_vectors();
            //fluid_grid.draw_divergence_vectors();
            //fluid_grid.draw_divergence_values();

            DrawText(TextFormat("FPS %i", GetFPS()), 10, 10, 30, BLUE);
        }
        EndDrawing();

        fluid_grid.advect_velocites(delta_time);
        //fluid_grid.advect_smoke(delta_time);
    }

    CloseWindow();

    return 0;
}
