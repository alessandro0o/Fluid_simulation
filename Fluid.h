#pragma once

static constexpr int grid_dimension = 35;
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
	bool is_blocked = false;
	Vector2 velocity_vector = { 0,0 };
};

class Fluid_grid
{
private:

	std::vector<Fluid_cell> fluid_cell_grid;
	std::vector<float> divergence_vectors_x;
	std::vector<float> divergence_vectors_y;
	std::vector<float> divergence_vectors_x_previous;
	std::vector<float> divergence_vectors_y_previous;
	Vector2 mouse_pos_previous = { 0.0f, 0.0f };


	float find_velocity_x(Vector2 pos)
	{
		float vel_x = 0.0f;

		if (pos.x <= 0.0f || pos.x >= grid_dimension || pos.y <= 0.0f || pos.y >= grid_dimension) return vel_x;

		int x_index = std::floor(pos.x);
		int y_index = std::floor(pos.y - 0.5f);

		if (pos.y < 0.5f)
		{
			y_index = 0;
		}

		float x_offset = pos.x - x_index;
		float y_offset = pos.y - 0.5f - y_index;

		float top_vel = 0.0f;
		float bottom_vel = 0.0f;

		top_vel += divergence_vectors_x[XYtoIndex(x_index, y_index, grid_dimension + 1)] * (1.0f - x_offset);
		top_vel += divergence_vectors_x[XYtoIndex(x_index + 1, y_index, grid_dimension + 1)] * (x_offset);

		if (pos.y < 0.5f)
		{
			vel_x += top_vel;
			return vel_x * (pos.y + 0.5f);
		}

		if (pos.y < grid_dimension - 0.5f)
		{
			bottom_vel += divergence_vectors_x[XYtoIndex(x_index, y_index + 1, grid_dimension + 1)] * (1.0f - x_offset);
			bottom_vel += divergence_vectors_x[XYtoIndex(x_index + 1, y_index + 1, grid_dimension + 1)] * (x_offset);

			vel_x += top_vel * (1.0f - y_offset);
			vel_x += bottom_vel * (y_offset);

			return vel_x;
		}

		vel_x += top_vel;
		return vel_x * (1.0f - y_offset);
	}

	float find_velocity_y(Vector2 pos)
	{
		float vel_y = 0.0f;

		if (pos.x <= 0.0f || pos.x >= grid_dimension || pos.y <= 0.0f || pos.y >= grid_dimension) return 0.0f;

		int x_index = std::floor(pos.x - 0.5f);
		int y_index = std::floor(pos.y);

		if (pos.x < 0.5f)
		{
			x_index = 0;
		}

		float x_offset = pos.x - 0.5f - x_index;
		float y_offset = pos.y - y_index;

		float left_vel = 0.0f;
		float right_vel = 0.0f;

		left_vel += divergence_vectors_y[XYtoIndex(x_index, y_index, grid_dimension)] * (1.0f - y_offset);
		left_vel += divergence_vectors_y[XYtoIndex(x_index, y_index + 1, grid_dimension)] * (y_offset);

		if (pos.x < 0.5f)
		{
			vel_y += left_vel;
			return vel_y * (pos.x + 0.5f);
		}

		if (pos.x < grid_dimension - 0.5f)
		{
			right_vel += divergence_vectors_y[XYtoIndex(x_index + 1, y_index, grid_dimension)] * (1.0f - y_offset);
			right_vel += divergence_vectors_y[XYtoIndex(x_index + 1, y_index + 1, grid_dimension)] * (y_offset);

			vel_y += left_vel * (1.0f - x_offset);
			vel_y += right_vel * (x_offset);

			return vel_y;
		}

		vel_y += left_vel;
		return vel_y * (1.0f - x_offset);
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

	Fluid_grid()
	{
		fluid_cell_grid = std::vector<Fluid_cell>(grid_dimension_sqr);
		divergence_vectors_x = std::vector<float>(vector_num, 0.0f);
		divergence_vectors_y = std::vector<float>(vector_num, 0.0f);
		divergence_vectors_x_previous = std::vector<float>(vector_num, 0.0f);
		divergence_vectors_y_previous = std::vector<float>(vector_num, 0.0f);
		init_cell_grid_texture();
	};
	~Fluid_grid()
	{
		unload_cell_grid_texture();
	};

	const unsigned int solver_iterations = 30;

	void velocity_setter()
	{
		const float vel = 50.0f;

		if (IsKeyDown(KEY_W) || IsKeyDown(KEY_E))
		{
			divergence_vectors_y[XYtoIndex(grid_dimension / 2, grid_dimension, grid_dimension)] = -vel;
		}
		if (IsKeyDown(KEY_S) || IsKeyDown(KEY_E))
		{
			divergence_vectors_y[XYtoIndex(grid_dimension / 2, 0, grid_dimension)] = vel;
		}
		if (IsKeyDown(KEY_D) || IsKeyDown(KEY_E))
		{
			divergence_vectors_x[XYtoIndex(0, grid_dimension / 2, grid_dimension + 1)] = vel;
			//divergence_vectors_x[XYtoIndex(0, grid_dimension / 2 + 1, grid_dimension + 1)] = vel;
			//divergence_vectors_x[XYtoIndex(0, grid_dimension / 2 - 1, grid_dimension + 1)] = vel;
			//divergence_vectors_x[XYtoIndex(0, grid_dimension / 2 + 2, grid_dimension + 1)] = vel;
			//divergence_vectors_x[XYtoIndex(0, grid_dimension / 2 - 2, grid_dimension + 1)] = vel;
			//divergence_vectors_x[XYtoIndex(0, grid_dimension / 2 + 3, grid_dimension + 1)] = vel;
			//divergence_vectors_x[XYtoIndex(0, grid_dimension / 2 - 3, grid_dimension + 1)] = vel;
			//divergence_vectors_x[XYtoIndex(0, grid_dimension / 2 + 4, grid_dimension + 1)] = vel;
			//divergence_vectors_x[XYtoIndex(0, grid_dimension / 2 - 4, grid_dimension + 1)] = vel;
		}
		if (IsKeyDown(KEY_A) || IsKeyDown(KEY_E))
		{
			divergence_vectors_x[XYtoIndex(grid_dimension, grid_dimension / 2, grid_dimension + 1)] = -vel;
		}
		if (IsKeyDown(KEY_R))
		{
			for (size_t i = 0; i < vector_num; i++)
			{
				divergence_vectors_x[i] = 0.0f;
				divergence_vectors_x_previous[i] = 0.0f;
				divergence_vectors_y[i] = 0.0f;
				divergence_vectors_y_previous[i] = 0.0f;
			}
		}
	}

	void velocity_brush(float delta_time)
	{
		const float effect_radius = 75.0f;
		const float effect_strength = 0.005f;

		const float offset = (WINDOW_WIDTH - WINDOW_HEIGHT) / 2.0f;
		Vector2 mouse_pos_current = GetMousePosition();

		float mouse_vel_x = (mouse_pos_current.x - mouse_pos_previous.x) / delta_time;
		float mouse_vel_y = (mouse_pos_current.y - mouse_pos_previous.y) / delta_time;

		for (size_t i = 0; i < vector_num; i++)
		{
			int x_1 = IndextoXY(i, grid_dimension + 1).x;
			int y_1 = IndextoXY(i, grid_dimension + 1).y;

			int x_2 = IndextoXY(i, grid_dimension).x;
			int y_2 = IndextoXY(i, grid_dimension).y;

			if (CheckCollisionPointCircle({ x_1 * cell_size + offset, y_1 * cell_size + cell_size / 2.0f }, mouse_pos_current, effect_radius) &&
				x_1 > 0 &&
				x_1 < grid_dimension &&
				y_1 > 0 &&
				y_1 < grid_dimension - 1
				&& IsMouseButtonDown(MOUSE_BUTTON_LEFT))
			{
				divergence_vectors_x[i] += mouse_vel_x * effect_strength;
			}

			if (CheckCollisionPointCircle({ x_2 * cell_size + offset + cell_size / 2.0f, y_2 * cell_size }, mouse_pos_current, effect_radius) &&
				x_2 > 0 &&
				x_2 < grid_dimension - 1 &&
				y_2 > 0 &&
				y_2 < grid_dimension
				&& IsMouseButtonDown(MOUSE_BUTTON_LEFT))
			{
				divergence_vectors_y[i] += mouse_vel_y * effect_strength;
			}
		}
		mouse_pos_previous = mouse_pos_current;
	}

	void blocked_cells_brush()
	{
		const float offset = (WINDOW_WIDTH - WINDOW_HEIGHT) / 2.0f;
		Vector2 mouse_pos = GetMousePosition();

		for (size_t i = 0; i < grid_dimension_sqr; i++)
		{
			int x = IndextoXY(i, grid_dimension).x;
			int y = IndextoXY(i, grid_dimension).y;

			float cell_pos_x = x * cell_size + offset;
			float cell_pos_y = y * cell_size;

			if (CheckCollisionPointRec(mouse_pos, { cell_pos_x, cell_pos_y, cell_size, cell_size }) && IsMouseButtonPressed(MOUSE_BUTTON_RIGHT))
			{
				if(fluid_cell_grid[i].is_blocked == true) fluid_cell_grid[i].is_blocked = false;
				else fluid_cell_grid[i].is_blocked = true;

			}
		}
	}

	void add_gravity(float delta_time)
	{
		const float gravitational_acceleration = 10.0f;

		for (size_t i = 0; i < vector_num; i++)
		{
			if(IndextoXY(i, grid_dimension).y < grid_dimension) divergence_vectors_y[i] += gravitational_acceleration * delta_time;
		}
	}

	void enable_blocked_cells()
	{
		const int N = grid_dimension;

		for (size_t i = 0; i < grid_dimension_sqr; i++)
		{
			const int x = IndextoXY(i, grid_dimension).x;
			const int y = IndextoXY(i, grid_dimension).y;

			float& div_up = divergence_vectors_y[XYtoIndex(x, y, N)];
			float& div_down = divergence_vectors_y[XYtoIndex(x, y + 1, N)];
			float& div_left = divergence_vectors_x[XYtoIndex(x, y, N + 1)];
			float& div_right = divergence_vectors_x[XYtoIndex(x + 1, y, N + 1)];

			if (fluid_cell_grid[i].is_blocked)
			{
				div_up = 0.0f;
				div_down = 0.0f;
				div_left = 0.0f;
				div_right = 0.0f;
			}
		}
	}

	void solve_incompressibility()
	{
		constexpr float overrelaxation_parameter = 1.7f;

		const int N = grid_dimension;

		for (size_t i = 0; i < grid_dimension_sqr; i++)
		{
			const int x = IndextoXY(i, grid_dimension).x;
			const int y = IndextoXY(i, grid_dimension).y;

			float& div_up = divergence_vectors_y[XYtoIndex(x, y, N)];
			float& div_down = divergence_vectors_y[XYtoIndex(x, y + 1, N)];
			float& div_left = divergence_vectors_x[XYtoIndex(x, y, N + 1)];
			float& div_right = divergence_vectors_x[XYtoIndex(x + 1, y, N + 1)];

			fluid_cell_grid[i].divergence = -(div_left - div_right + div_up - div_down) * overrelaxation_parameter;

			bool is_free_up = true;
			bool is_free_down = true;
			bool is_free_left = true;
			bool is_free_right = true;

			if (x > 0)
			{
				if (fluid_cell_grid[XYtoIndex(x - 1, y, grid_dimension)].is_blocked == true)
				{
					is_free_left = false;
				}
			}
			if (x < N - 1)
			{
				if (fluid_cell_grid[XYtoIndex(x + 1, y, grid_dimension)].is_blocked == true)
				{
					is_free_right = false;
				}
			}
			if (y > 0)
			{
				if (fluid_cell_grid[XYtoIndex(x, y - 1, grid_dimension)].is_blocked == true)
				{
					is_free_up = false;
				}
			}
			if (y < N - 1)
			{
				if (fluid_cell_grid[XYtoIndex(x, y + 1, grid_dimension)].is_blocked == true)
				{
					is_free_down = false;
				}
			}

			int divisor = 4;

			if (x == 0	||		!is_free_left)		divisor--;
			if (x == N - 1 ||	!is_free_right)		divisor--;
			if (y == 0 ||		!is_free_up)		divisor--;
			if (y == N - 1 ||	!is_free_down)		divisor--;

			if (divisor == 0) divisor = 1;

			float d = fluid_cell_grid[i].divergence / divisor;

			if (x > 0 && is_free_left) div_left += d;
			if (x < N - 1 && is_free_right) div_right -= d;
			if (y > 0 && is_free_up) div_up += d;
			if (y < N - 1 && is_free_down) div_down -= d;
		}
	}

	//void solve_incompressibility()
	//{
	//	constexpr float overrelaxation_parameter = 1.7f;

	//	const int N = grid_dimension;

	//	for (size_t i = 0; i < grid_dimension_sqr; i++)
	//	{
	//		const int x = IndextoXY(i, grid_dimension).x;
	//		const int y = IndextoXY(i, grid_dimension).y;

	//		float& div_up = divergence_vectors_y[XYtoIndex(x, y, N)];
	//		float& div_down = divergence_vectors_y[XYtoIndex(x, y + 1, N)];
	//		float& div_left = divergence_vectors_x[XYtoIndex(x, y, N + 1)];
	//		float& div_right = divergence_vectors_x[XYtoIndex(x + 1, y, N + 1)];

	//		fluid_cell_grid[i].divergence = -(div_left - div_right + div_up - div_down) * overrelaxation_parameter;

	//		int divisor = 4;

	//		if (x == 0)		divisor--;
	//		if (x == N - 1)	divisor--;
	//		if (y == 0)		divisor--;
	//		if (y == N - 1)	divisor--;

	//		if (divisor == 0) divisor = 1;

	//		float d = fluid_cell_grid[i].divergence / divisor;

	//		if (x > 0)		div_left	+= d;
	//		if (x < N - 1)	div_right	-= d;
	//		if (y > 0)		div_up		+= d;
	//		if (y < N - 1)	div_down	-= d;
	//	}
	//}

	void advect_velocites(float delta_time)
	{
		// x velocities

		for (size_t i = 0; i < vector_num; i++)
		{
			float x_component = divergence_vectors_x[i];
			float y_component = 0.0f;

			int x = IndextoXY(i, grid_dimension + 1).x;
			int y = IndextoXY(i, grid_dimension + 1).y;

			if (x == grid_dimension)
			{
				y_component = (divergence_vectors_y[XYtoIndex(x - 1, y, grid_dimension)] +
					divergence_vectors_y[XYtoIndex(x - 1, y + 1, grid_dimension)]) / 2.0f;
			}
			else if (x == 0)
			{
				y_component = (divergence_vectors_y[XYtoIndex(x, y, grid_dimension)] +
					divergence_vectors_y[XYtoIndex(x, y + 1, grid_dimension)]) / 2.0f;
			}
			else
			{
				y_component = (divergence_vectors_y[XYtoIndex(x - 1, y, grid_dimension)] +
					divergence_vectors_y[XYtoIndex(x - 1, y + 1, grid_dimension)] +
					divergence_vectors_y[XYtoIndex(x, y, grid_dimension)] +
					divergence_vectors_y[XYtoIndex(x, y + 1, grid_dimension)]) / 4.0f;
			}

			float previous_position_x = x - x_component * delta_time;
			float previous_position_y = y + 0.5f - y_component * delta_time;

			divergence_vectors_x_previous[i] = find_velocity_x({ previous_position_x, previous_position_y });
		}
		


		// y velocities

		for (size_t i = 0; i < vector_num; i++)
		{
			float y_component = divergence_vectors_y[i];
			float x_component;

			float x = IndextoXY(i, grid_dimension).x;
			float y = IndextoXY(i, grid_dimension).y;

			if (y == grid_dimension)
			{
				x_component = (divergence_vectors_x[XYtoIndex(x, y - 1, grid_dimension + 1)] +
					divergence_vectors_x[XYtoIndex(x + 1, y - 1, grid_dimension + 1)]) / 2;
			}
			else if (y == 0)
			{
				x_component = (divergence_vectors_x[XYtoIndex(x, y, grid_dimension + 1)] +
					divergence_vectors_x[XYtoIndex(x + 1, y, grid_dimension + 1)]) / 2;
			}
			else
			{
				x_component = (divergence_vectors_x[XYtoIndex(x, y - 1, grid_dimension + 1)] +
					divergence_vectors_x[XYtoIndex(x + 1, y - 1, grid_dimension + 1)] +
					divergence_vectors_x[XYtoIndex(x, y, grid_dimension + 1)] +
					divergence_vectors_x[XYtoIndex(x + 1, y, grid_dimension + 1)]) / 4;
			}

			float previous_position_x = x + 0.5f - x_component * delta_time;
			float previous_position_y = y - y_component * delta_time;

			divergence_vectors_y_previous[i] = find_velocity_y({ previous_position_x, previous_position_y });
		}

		for (size_t i = 0; i < vector_num; i++)
		{
			divergence_vectors_x[i] = divergence_vectors_x_previous[i];
			divergence_vectors_y[i] = divergence_vectors_y_previous[i];
		}
	}


	void draw_cell_grid()
	{
		const float offset = (WINDOW_WIDTH - WINDOW_HEIGHT) / 2.0f;

		for (size_t i = 0; i < grid_dimension_sqr; i++)
		{
			//cell_grid_pixels[i] = getColor(fluid_cell_grid[i].divergence);
			if (!fluid_cell_grid[i].is_blocked) cell_grid_pixels[i] = { 33, 46, 82, 255 };
			else cell_grid_pixels[i] = { 156, 37, 66, 255 };
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
		const float length_multiplier = 2.0f;

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
		const unsigned int vectors_per_side = 2;
		const float thickness = 150.0f;
		const float length_multiplier = 2.0f;

		const int N = grid_dimension;
		const float offset = (WINDOW_WIDTH - WINDOW_HEIGHT) / 2.0f;
		const int vectors_per_cell = vectors_per_side * vectors_per_side;
		const float cell_offset = cell_size / (vectors_per_side * 2);
		const float m = 1.0f / (vectors_per_side * 2);

		for (size_t i = 0; i < grid_dimension_sqr; i++)
		{
			if (fluid_cell_grid[i].is_blocked == true) continue;

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
