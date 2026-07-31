#pragma once

static constexpr int grid_dimension = 14;
static constexpr int grid_dimension_sqr = grid_dimension * grid_dimension;
static constexpr int vector_num = grid_dimension * (grid_dimension + 1);
static constexpr float cell_size = WINDOW_HEIGHT / grid_dimension;

struct Vec2Int
{
	int x;
	int y;
};

inline int XYtoIndex(int x, int y, int width)
{
	return y * width + x;
}

inline Vec2Int IndextoXY(int index, int width)
{
	int x = index % width;
	int y = index / width;

	return { x, y };
}

struct Fluid_cell
{
	float divergence = 0.0f;
	float pressure = 0.0f;
	Vector2 velocity_vector = { 0,0 };
};

class Fluid_grid
{
private:

	std::vector<Fluid_cell> fluid_cell_grid;
	std::vector<float> divergence_vectors_x;
	std::vector<float> divergence_vectors_y;

	Texture2D cell_grid_texture;
	Image cell_grid_image;
	Color* cell_grid_pixels;

	void init_cell_grid_texture()
	{
		cell_grid_image = GenImageColor(
			grid_dimension,
			grid_dimension,
			BLACK
		);

		cell_grid_texture = LoadTextureFromImage(cell_grid_image);

		SetTextureFilter(cell_grid_texture, TEXTURE_FILTER_POINT);

		cell_grid_pixels = LoadImageColors(cell_grid_image);

		UnloadImage(cell_grid_image);
	}

	void unload_cell_grid_texture()
	{
		UnloadImageColors(cell_grid_pixels);
		UnloadTexture(cell_grid_texture);
	}

public:

	void velocity_setter(float delta_time)
	{
		for (size_t i = grid_dimension_sqr / 2 - grid_dimension / 3; i < grid_dimension_sqr / 2 - grid_dimension / 5 + grid_dimension / 2 + 1; i++)
		{
			if (IsKeyDown(KEY_Q))
			{
				divergence_vectors_x[i] -= 5.0f * delta_time;
				divergence_vectors_x[i + grid_dimension + 1] -= 5.0f * delta_time;
				divergence_vectors_x[i + 2 * grid_dimension + 2] -= 5.0f * delta_time;
				divergence_vectors_x[i + 3 * grid_dimension + 3] -= 5.0f * delta_time;
			}
			else if (IsKeyDown(KEY_E))
			{
				divergence_vectors_x[i] += 5.0f * delta_time;
				divergence_vectors_x[i + grid_dimension + 1] += 5.0f * delta_time;
				divergence_vectors_x[i + 2 * grid_dimension + 2] += 5.0f * delta_time;
				divergence_vectors_x[i + 3 * grid_dimension + 3] += 5.0f * delta_time;
			}
		}
	}

	Fluid_grid()
	{
		fluid_cell_grid = std::vector<Fluid_cell>(grid_dimension_sqr);
		divergence_vectors_x = std::vector<float>(vector_num, 0.0f);
		divergence_vectors_y = std::vector<float>(vector_num, 0.0f);
		//random_divergence_vectors_init();
		//compute_divergence();
		init_cell_grid_texture();
	};
	~Fluid_grid()
	{
		unload_cell_grid_texture();
	};

	void compute_divergence()
	{
		const float overrelaxation_parameter = 1.9f;

		for (size_t i = 0; i < grid_dimension_sqr; i++)
		{
			fluid_cell_grid[i].divergence = 0.0f;

			fluid_cell_grid[i].divergence = -(divergence_vectors_x[XYtoIndex(IndextoXY(i, grid_dimension).x, IndextoXY(i, grid_dimension).y, grid_dimension + 1)] -
				divergence_vectors_x[XYtoIndex(IndextoXY(i, grid_dimension).x + 1, IndextoXY(i, grid_dimension).y, grid_dimension + 1)] +
				divergence_vectors_y[XYtoIndex(IndextoXY(i, grid_dimension).x, IndextoXY(i, grid_dimension).y, grid_dimension)] -
				divergence_vectors_y[XYtoIndex(IndextoXY(i, grid_dimension).x, IndextoXY(i, grid_dimension).y + 1, grid_dimension)]) * overrelaxation_parameter;
		}
	}

	void random_divergence_vectors_init()
	{
		SetRandomSeed(time(NULL));
		for (size_t i = 0; i < vector_num; i++)
		{
			if (IndextoXY(i, grid_dimension + 1).x > 0 && IndextoXY(i, grid_dimension + 1).x < grid_dimension)
			{
				divergence_vectors_x[i] = (float)(GetRandomValue(-100, 100)) / 100;
			}

			if (IndextoXY(i, grid_dimension).y > 0 && IndextoXY(i, grid_dimension).y < grid_dimension)
			{
				divergence_vectors_y[i] = (float)(GetRandomValue(-100, 100)) / 100;
			}
		}
	}

	void solve_incompressibility()
	{
		constexpr float overrelaxation_parameter = 1.7f;

		const int N = grid_dimension;

		for (size_t i = 0; i < grid_dimension_sqr; i++)
		{
			const int x = i % N;
			const int y = i / N;

			float& div_up = divergence_vectors_y[XYtoIndex(x, y, N)];
			float& div_down = divergence_vectors_y[XYtoIndex(x, y + 1, N)];
			float& div_left = divergence_vectors_x[XYtoIndex(x, y, N + 1)];
			float& div_right = divergence_vectors_x[XYtoIndex(x + 1, y, N + 1)];

			float divergence = -(div_left - div_right + div_up - div_down) * overrelaxation_parameter;

			fluid_cell_grid[i].divergence = divergence;

			int divisor = 4;

			if (x == 0 || x == N - 1) divisor--;

			if (y == 0 || y == N - 1) divisor--;

			float d = divergence / divisor;

			if (x > 0)
				div_left += d;

			if (x < N - 1)
				div_right -= d;

			if (y > 0)
				div_up += d;

			if (y < N - 1)
				div_down -= d;
		}
	}

	void add_gravity(float delta_time)
	{
		const float gravitational_acceleration = 10.0f;

		for (size_t i = 0; i < vector_num; i++)
		{
			divergence_vectors_y[i] += gravitational_acceleration * delta_time;
		}
	}

	void draw_cell_grid()
	{
		const float offset = (WINDOW_WIDTH - WINDOW_HEIGHT) / 2.0f;

		for (size_t i = 0; i < grid_dimension_sqr; i++)
		{
			//cell_grid_pixels[i] = getColor(fluid_cell_grid[i].divergence);
			cell_grid_pixels[i] = { 33, 46, 82, 255 };
		}

		UpdateTexture(
			cell_grid_texture,
			cell_grid_pixels
		);

		DrawTextureEx(
			cell_grid_texture,
			{ offset, 0 },
			0.0f,
			cell_size,
			WHITE
		);
	}

	void draw_divergence_values()
	{
		const float offset = (WINDOW_WIDTH - WINDOW_HEIGHT) / 2;

		for (size_t i = 0; i < grid_dimension_sqr; i++)
		{
			float divergence = fluid_cell_grid[i].divergence;
			if (divergence < 0.01f && divergence > 0.01f) divergence = 0.0f;

			DrawText(TextFormat("%.1f", divergence),
				IndextoXY(i, grid_dimension).x * cell_size + offset + cell_size / 4,
				IndextoXY(i, grid_dimension).y * cell_size + cell_size / 3,
				(400 / grid_dimension),
				WHITE);
		}
	}

	void draw_divergence_vectors()
	{
		const float offset = (WINDOW_WIDTH - WINDOW_HEIGHT) / 2;
		const float length_multiplier = 50.0f;

		for (size_t i = 0; i < vector_num; i++)
		{
			DrawLineEx({ IndextoXY(i, grid_dimension + 1).x * cell_size + offset, IndextoXY(i, grid_dimension + 1).y * cell_size + cell_size / 2 },
				{ IndextoXY(i, grid_dimension + 1).x * cell_size + offset + divergence_vectors_x[i] * length_multiplier, IndextoXY(i, grid_dimension + 1).y * cell_size + cell_size / 2 },
				100 / grid_dimension, DARKPURPLE);

			DrawLineEx({ IndextoXY(i, grid_dimension).x * cell_size + offset + cell_size / 2, IndextoXY(i, grid_dimension).y * cell_size },
				{ IndextoXY(i, grid_dimension).x * cell_size + offset + cell_size / 2, IndextoXY(i, grid_dimension).y * cell_size + divergence_vectors_y[i] * length_multiplier },
				100 / grid_dimension, DARKPURPLE);
		}
	}

	void draw_interpolated_divergence_vectors()
	{
		const int vectors_per_side = 3;
		const float thickness = 50.0f;

		const int N = grid_dimension;
		const float offset = (WINDOW_WIDTH - WINDOW_HEIGHT) / 2;
		const float length_multiplier = 40.0f;
		const int vectors_per_cell = vectors_per_side * vectors_per_side;
		const float cell_offset = cell_size / (vectors_per_side * 2);
		const float m = 1.0f / (vectors_per_side * 2);

		for (size_t i = 0; i < grid_dimension_sqr; i++)
		{
			const int x = i % N;
			const int y = i / N;

			float div_up = divergence_vectors_y[XYtoIndex(x, y, N)];
			float div_down = divergence_vectors_y[XYtoIndex(x, y + 1, N)];
			float div_left = divergence_vectors_x[XYtoIndex(x, y, N + 1)];
			float div_right = divergence_vectors_x[XYtoIndex(x + 1, y, N + 1)];

			float index_x = IndextoXY(i, grid_dimension).x;
			float index_y = IndextoXY(i, grid_dimension).y;

			for (size_t j = 0; j < vectors_per_side; j++)
			{
				for (size_t k = 0; k < vectors_per_side; k++)
				{
					//DrawCircleV({ index_x * cell_size + 2 * cell_offset * k + cell_offset + offset, index_y * cell_size + 2 * cell_offset * j + cell_offset }, 3.0f, RED);

					float x_distance = m * (2 * k + 1);
					float y_distance = m * (2 * j + 1);

					float x_component = 0.0f;
					float y_component = 0.0f;

					if (j < vectors_per_side / 2)
					{
						x_component = (div_left * (1.0f - x_distance) * (y_distance + 0.5f)) +
							(div_right * x_distance * (y_distance + 0.5f));

						if (y > 0)
						{
							x_component += (divergence_vectors_x[XYtoIndex(x, y - 1, N + 1)] * (1.0f - x_distance) * (1.0f - (y_distance + 0.5f))) +
								(divergence_vectors_x[XYtoIndex(x + 1, y - 1, N + 1)] * x_distance * (1.0f - (y_distance + 0.5f)));
						}
					}
					else
					{
						x_component = (div_left * (1.0f - x_distance) * (1.0f - (y_distance - 0.5f))) +
							(div_right * x_distance * (1.0f - (y_distance - 0.5f)));

						if (y < N - 1)
						{
							x_component += (divergence_vectors_x[XYtoIndex(x, y + 1, N + 1)] * (1.0f - x_distance) * (y_distance - 0.5f)) +
								(divergence_vectors_x[XYtoIndex(x + 1, y + 1, N + 1)] * x_distance * (y_distance - 0.5f));
						}
					}

					if (k < vectors_per_side / 2)
					{
						y_component = (div_up * (x_distance + 0.5f) * (1.0f - y_distance)) +
							(div_down * (x_distance + 0.5f) * y_distance);

						if (x > 0)
						{
							y_component += ((divergence_vectors_y[XYtoIndex(x - 1, y, N)] * (1.0f - (x_distance + 0.5f)) * (1.0f - y_distance)) +
								(divergence_vectors_y[XYtoIndex(x - 1, y + 1, N)] * (1.0f - (x_distance + 0.5f)) * y_distance));
						}
					}
					else
					{
						y_component = (div_up * (1.0f - (x_distance - 0.5f)) * (1.0f - y_distance)) +
							(div_down * (1.0f - (x_distance - 0.5f)) * y_distance);

						if (x < N - 1)
						{
							y_component += ((divergence_vectors_y[XYtoIndex(x + 1, y, N)] * (x_distance - 0.5f) * (1.0f - y_distance)) +
								(divergence_vectors_y[XYtoIndex(x + 1, y + 1, N)] * (x_distance - 0.5f) * y_distance));
						}
					}

					DrawLineEx({ index_x * cell_size + cell_offset * (2 * k + 1) + offset,									 index_y * cell_size + cell_offset * (2 * j + 1) },
							   { index_x * cell_size + cell_offset * (2 * k + 1) + offset + x_component * length_multiplier, index_y * cell_size + cell_offset * (2 * j + 1) + y_component * length_multiplier},
								thickness / grid_dimension, GRAY);
				}
			}
		}
	}
};