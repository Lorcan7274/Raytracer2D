#include <iostream>
#include <cmath>
#include <algorithm>
#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"
#include <vector>
#include <tuple>
struct Edge {
	float start_x, start_y;
	float end_x, end_y;
};

#define NORTH 0
#define SOUTH 1
#define EAST 2
#define WEST 3

struct Cell {
	int edge_id[4];
	bool edge_exist[4];
	bool cell_exist{ false };
};

class ShadowCasting2D : public olc::PixelGameEngine
{
public:
	ShadowCasting2D()
	{
		sAppName = "ShadowCasting2D";
	}

private:
	Cell* world;
	int world_width{ 40 };
	int world_height{ 30 };

	olc::Sprite* sprLightCast;
	olc::Sprite* buffLightRay;

	std::vector<Edge> vecEdges;
	std::vector<std::tuple<float, float, float>> vecVisibilityPolygonPoints;

	// Cursor used for the light and for placing blocks. Follows the mouse, but can
	// also be moved with the keyboard so the demo works without a mouse
	float cursor_x{ 0.0f };
	float cursor_y{ 0.0f };
	int last_mouse_x{ 0 };
	int last_mouse_y{ 0 };
	bool keyboard_cursor{ false };

	bool light_toggled{ false };
	bool show_help{ true };

	// Only cells inside the outer wall can be edited. Blocks on the outer ring never
	// get edges, and holes in the wall let the light escape the map.
	bool IsEditableCell(int x, int y) const
	{
		return x >= 2 && x < world_width - 2 && y >= 2 && y < world_height - 2;
	}

	void LoadLightTexture()
	{
		// The path is relative to the working directory, which depends on how the
		// program was started (VS debugger, double clicking the exe in x64/Debug, ...)
		const char* paths[] = { "assets/light_cast.png", "../assets/light_cast.png", "../../assets/light_cast.png" };

		sprLightCast = new olc::Sprite();
		for (const char* path : paths)
			if (sprLightCast->LoadFromFile(path) == olc::rcode::OK)
				return;

		// Not found, generate a radial glow matching light_cast.png so the light still works
		std::cerr << "Couldn't find assets/light_cast.png, using a generated light texture\n";
		const int size = 512;
		sprLightCast->SetSize(size, size);
		for (int x = 0; x < size; x++)
			for (int y = 0; y < size; y++)
			{
				float dx = x - size / 2 + 0.5f;
				float dy = y - size / 2 + 0.5f;
				float t = std::min(1.0f, std::sqrt(dx * dx + dy * dy) / (size / 2));
				float smooth = t * t * (3.0f - 2.0f * t);
				uint8_t v = uint8_t(255.0f * (1.0f - 0.5f * (t + smooth)));
				sprLightCast->SetPixel(x, y, olc::Pixel(v, v, v));
			}
	}

	void UpdateCursor(float fElapsedTime)
	{
		// The mouse takes over again as soon as it moves or clicks
		int mouse_x = GetMouseX();
		int mouse_y = GetMouseY();
		bool mouse_clicked = GetMouse(0).bPressed || GetMouse(1).bPressed || GetMouse(2).bPressed;
		if (mouse_x != last_mouse_x || mouse_y != last_mouse_y || mouse_clicked)
		{
			cursor_x = (float)mouse_x;
			cursor_y = (float)mouse_y;
			keyboard_cursor = false;
		}
		last_mouse_x = mouse_x;
		last_mouse_y = mouse_y;

		float dx = 0.0f, dy = 0.0f;
		if (GetKey(olc::Key::LEFT).bHeld || GetKey(olc::Key::A).bHeld) dx -= 1.0f;
		if (GetKey(olc::Key::RIGHT).bHeld || GetKey(olc::Key::D).bHeld) dx += 1.0f;
		if (GetKey(olc::Key::UP).bHeld || GetKey(olc::Key::W).bHeld) dy -= 1.0f;
		if (GetKey(olc::Key::DOWN).bHeld || GetKey(olc::Key::S).bHeld) dy += 1.0f;

		if (dx != 0.0f || dy != 0.0f)
		{
			const float cursor_speed = 200.0f; // pixels per second
			cursor_x = std::clamp(cursor_x + dx * cursor_speed * fElapsedTime, 0.0f, ScreenWidth() - 1.0f);
			cursor_y = std::clamp(cursor_y + dy * cursor_speed * fElapsedTime, 0.0f, ScreenHeight() - 1.0f);
			keyboard_cursor = true;
		}
	}

	void DrawHelp()
	{
		const std::vector<std::string> lines = {
			"CONTROLS",
			"",
			"Mouse / trackpad",
			"  Left click            add blocks",
			"  Right click [hold]    light on",
			"  Middle click          remove blocks",
			"  Shift + left click    remove blocks",
			"",
			"Keyboard (no mouse needed)",
			"  Arrows / WASD         move cursor",
			"  Space / L             toggle light",
			"  Enter                 add blocks",
			"  Backspace / Delete    remove blocks",
			"  H / F1                show / hide help",
		};

		size_t longest = 0;
		for (const auto& line : lines)
			longest = std::max(longest, line.size());

		int w = int(longest) * 8 + 24;
		int h = int(lines.size()) * 12 + 20;
		int x = (ScreenWidth() - w) / 2;
		int y = (ScreenHeight() - h) / 2;

		FillRect(x, y, w, h, olc::Pixel(8, 10, 14));
		DrawRect(x, y, w - 1, h - 1, olc::Pixel(100, 230, 255));
		for (size_t i = 0; i < lines.size(); i++)
			DrawString(x + 12, y + 12 + int(i) * 12, lines[i], i == 0 ? olc::Pixel(100, 230, 255) : olc::WHITE);
	}

	void ConvertTileMapToPolyMap(int start_x, int start_y, int width, int height, float block_width, int pitch)
	{
		vecEdges.clear();

		for (int x = 0; x < width; x++)
			for (int y = 0; y < height; y++)
				for (int j = 0; j < 4; j++)
				{
					world[(y + start_y) * pitch + (x + start_x)].edge_exist[j] = false;
					world[(y + start_y) * pitch + (x + start_x)].edge_id[j] = 0;
				}

		for (int x = 1; x < width - 1; x++)
			for (int y = 1; y < height - 1; y++)
			{
				int i = (y + start_y) * pitch + (x + start_x);
				int north = (y + start_y - 1) * pitch + (x + start_x);
				int south = (y + start_y + 1) * pitch + (x + start_x);
				int west = (y + start_y) * pitch + (x + start_x - 1);
				int east = (y + start_y) * pitch + (x + start_x + 1);

				if (world[i].cell_exist)
				{
					if (!world[west].cell_exist)
					{
						if (world[north].edge_exist[WEST])
						{
							vecEdges[world[north].edge_id[WEST]].end_y += block_width;
							world[i].edge_id[WEST] = world[north].edge_id[WEST];
							world[i].edge_exist[WEST] = true;
						}
						else
						{
							Edge edge;
							edge.start_x = (start_x + x) * block_width;
							edge.start_y = (start_y + y) * block_width;
							edge.end_x = edge.start_x;
							edge.end_y = edge.start_y + block_width;

							int edge_id = (int)vecEdges.size();
							vecEdges.push_back(edge);

							world[i].edge_id[WEST] = edge_id;
							world[i].edge_exist[WEST] = true;
						}
					}

					if (!world[east].cell_exist)
					{
						if (world[north].edge_exist[EAST])
						{
							vecEdges[world[north].edge_id[EAST]].end_y += block_width;
							world[i].edge_id[EAST] = world[north].edge_id[EAST];
							world[i].edge_exist[EAST] = true;
						}
						else
						{
							Edge edge;
							edge.start_x = (start_x + x + 1) * block_width;
							edge.start_y = (start_y + y) * block_width;
							edge.end_x = edge.start_x;
							edge.end_y = edge.start_y + block_width;

							int edge_id = (int)vecEdges.size();
							vecEdges.push_back(edge);

							world[i].edge_id[EAST] = edge_id;
							world[i].edge_exist[EAST] = true;
						}
					}

					if (!world[north].cell_exist)
					{
						if (world[west].edge_exist[NORTH])
						{
							vecEdges[world[west].edge_id[NORTH]].end_x += block_width;
							world[i].edge_id[NORTH] = world[west].edge_id[NORTH];
							world[i].edge_exist[NORTH] = true;
						}
						else
						{
							Edge edge;
							edge.start_x = (start_x + x) * block_width;
							edge.start_y = (start_y + y) * block_width;
							edge.end_x = edge.start_x + block_width;
							edge.end_y = edge.start_y;

							int edge_id = (int)vecEdges.size();
							vecEdges.push_back(edge);

							world[i].edge_id[NORTH] = edge_id;
							world[i].edge_exist[NORTH] = true;
						}
					}

					if (!world[south].cell_exist)
					{
						if (world[west].edge_exist[SOUTH])
						{
							vecEdges[world[west].edge_id[SOUTH]].end_x += block_width;
							world[i].edge_id[SOUTH] = world[west].edge_id[SOUTH];
							world[i].edge_exist[SOUTH] = true;
						}
						else
						{
							Edge edge;
							edge.start_x = (start_x + x) * block_width;
							edge.start_y = (start_y + y + 1) * block_width;
							edge.end_x = edge.start_x + block_width;
							edge.end_y = edge.start_y;

							int edge_id = (int)vecEdges.size();
							vecEdges.push_back(edge);

							world[i].edge_id[SOUTH] = edge_id;
							world[i].edge_exist[SOUTH] = true;
						}
					}
				}
			}
	}

	void CalculateVisibilityPolygon(float ox, float oy, float radius)
	{
		vecVisibilityPolygonPoints.clear();

		for (auto& e1 : vecEdges)
		{
			for (int i = 0; i < 2; i++)
			{
				float rdx = (i == 0 ? e1.start_x : e1.end_x) - ox;
				float rdy = (i == 0 ? e1.start_y : e1.end_y) - oy;

				float base_ang = atan2f(rdy, rdx);

				for (int j = 0; j < 3; j++)
				{
					float ang = base_ang;
					if (j == 0) ang -= 0.001f;
					if (j == 2) ang += 0.001f;

					rdx = radius * cosf(ang);
					rdy = radius * sinf(ang);
					float min_t1 = INFINITY;
					float min_px = 0.0f, min_py = 0.0f, min_ang = 0.0f;
					bool bValid = false;

					for (auto& e2 : vecEdges)
					{
						float sdx = e2.end_x - e2.start_x;
						float sdy = e2.end_y - e2.start_y;
						float denom = (sdx * rdy - sdy * rdx);
						if (fabs(denom) < 1e-6f) continue;

						float t2 = (rdx * (e2.start_y - oy) + (rdy * (ox - e2.start_x))) / denom;
						float t1;
						if (fabs(rdx) > fabs(rdy))
							t1 = (e2.start_x + sdx * t2 - ox) / rdx;
						else
							t1 = (e2.start_y + sdy * t2 - oy) / rdy;

						if (t1 > 0.0f && t2 >= 0.0f && t2 <= 1.0f)
						{
							if (t1 < min_t1)
							{
								min_t1 = t1;
								min_px = ox + rdx * t1;
								min_py = oy + rdy * t1;
								min_ang = atan2f(min_py - oy, min_px - ox);
								if (min_ang < -3.14159f) min_ang += 6.28318f;
								if (min_ang > 3.14159f)  min_ang -= 6.28318f;
								bValid = true;
							}
						}
					}

					if (bValid)
						vecVisibilityPolygonPoints.push_back({ min_ang, min_px, min_py });
				}
			}
		}

		std::sort(
			vecVisibilityPolygonPoints.begin(),
			vecVisibilityPolygonPoints.end(),
			[&](const std::tuple<float, float, float>& t1, const std::tuple<float, float, float>& t2)
			{
				return std::get<0>(t1) < std::get<0>(t2);
			});
	}

public:
	bool OnUserCreate() override
	{
		world = new Cell[world_height * world_width];
		for (int x = 1; x < (world_width - 1); x++)
		{
			world[1 * world_width + x].cell_exist = true;
			world[(world_height - 2) * world_width + x].cell_exist = true;
		}

		for (int x = 1; x < (world_height - 1); x++)
		{
			world[x * world_width + 1].cell_exist = true;
			world[x * world_width + (world_width - 2)].cell_exist = true;
		}

		LoadLightTexture();

		buffLightRay = new olc::Sprite(ScreenWidth(), ScreenHeight());

		// Start in the middle of the room until the mouse or keyboard moves the cursor
		cursor_x = ScreenWidth() / 2.0f;
		cursor_y = ScreenHeight() / 2.0f;
		last_mouse_x = GetMouseX();
		last_mouse_y = GetMouseY();
		return true;
	}

	bool OnUserDestroy() override
	{
		delete[] world;
		delete sprLightCast;
		delete buffLightRay;
		return true;
	}

	bool OnUserUpdate(float fElapsedTime) override
	{
		float block_width{ 16.0f };

		if (GetKey(olc::Key::H).bPressed || GetKey(olc::Key::F1).bPressed)
			show_help = !show_help;

		// The light can be held on with the right mouse button, or toggled from the
		// keyboard, which also works on trackpads and without a mouse
		if (GetKey(olc::Key::SPACE).bPressed || GetKey(olc::Key::L).bPressed)
			light_toggled = !light_toggled;
		bool light_on = light_toggled || GetMouse(1).bHeld;

		UpdateCursor(fElapsedTime);

		// Shift + left click removes blocks on trackpads without a middle button
		bool shift = GetKey(olc::Key::SHIFT).bHeld;
		bool add_block = (GetMouse(0).bHeld && !shift) || GetKey(olc::Key::ENTER).bHeld;
		bool remove_block = GetMouse(2).bHeld || (GetMouse(0).bHeld && shift)
			|| GetKey(olc::Key::BACK).bHeld || GetKey(olc::Key::DEL).bHeld;

		int grid_x = (int)(cursor_x / block_width);
		int grid_y = (int)(cursor_y / block_width);

		if (IsEditableCell(grid_x, grid_y))
		{
			int i = grid_y * world_width + grid_x;
			if (remove_block)
				world[i].cell_exist = false;
			else if (add_block)
				world[i].cell_exist = true;
		}

		ConvertTileMapToPolyMap(0, 0, world_width, world_height, block_width, world_width);

		// Keep the light inside the outer wall, outside it the rays escape the map and the
		// visibility polygon breaks. Using the pixel centre keeps it off the block edges.
		float source_x = std::clamp(floorf(cursor_x) + 0.5f, 2 * block_width + 0.5f, (world_width - 2) * block_width - 0.5f);
		float source_y = std::clamp(floorf(cursor_y) + 0.5f, 2 * block_width + 0.5f, (world_height - 2) * block_width - 0.5f);

		if (light_on) {
			CalculateVisibilityPolygon(source_x, source_y, 1000.0f);
		}
		SetDrawTarget(nullptr);
		Clear(olc::Pixel(12, 14, 18));

		auto it = unique(
			vecVisibilityPolygonPoints.begin(),
			vecVisibilityPolygonPoints.end(),
			[&](const std::tuple<float, float, float>& t1, const std::tuple<float, float, float>& t2)
			{
				return fabs(std::get<1>(t1) - std::get<1>(t2)) < 0.1f && fabs(std::get<2>(t1) - std::get<2>(t2)) < 0.1f;
			});
		vecVisibilityPolygonPoints.resize(std::distance(vecVisibilityPolygonPoints.begin(), it));

		if (light_on && vecVisibilityPolygonPoints.size() > 1)
		{
			SetDrawTarget(buffLightRay);
			Clear(olc::BLACK);

			const float EDGE_EPS = 0.6f;
			auto expand = [&](float px, float py)
				{
					float dx = px - source_x;
					float dy = py - source_y;
					float inv = 1.0f / std::max(1e-6f, std::sqrt(dx * dx + dy * dy));
					dx *= inv; dy *= inv;
					return std::make_pair(px + dx * EDGE_EPS, py + dy * EDGE_EPS);
				};

			for (int i = 0; i < (int)vecVisibilityPolygonPoints.size() - 1; i++)
			{
				auto [x1, y1] = expand(std::get<1>(vecVisibilityPolygonPoints[i]), std::get<2>(vecVisibilityPolygonPoints[i]));
				auto [x2, y2] = expand(std::get<1>(vecVisibilityPolygonPoints[i + 1]), std::get<2>(vecVisibilityPolygonPoints[i + 1]));
				FillTriangle(source_x, source_y, x1, y1, x2, y2, olc::WHITE);
			}
			{
				auto [x1, y1] = expand(std::get<1>(vecVisibilityPolygonPoints.back()), std::get<2>(vecVisibilityPolygonPoints.back()));
				auto [x2, y2] = expand(std::get<1>(vecVisibilityPolygonPoints.front()), std::get<2>(vecVisibilityPolygonPoints.front()));
				FillTriangle(source_x, source_y, x1, y1, x2, y2, olc::WHITE);
			}

			static float totalTime = 0.0f;
			totalTime += fElapsedTime;
			float pulse = 1.0f + 0.2f * sinf(totalTime * 2.0f);

			// Change light source brightness / glow / colour here 
			float brightness = 0.6f + 0.1f * sinf(totalTime * 1.6f);

			int redTint = int(100 * brightness);
			int greenTint = int(230 * brightness);
			int blueTint = int(255 * brightness);

			int lr = std::clamp(int(redTint * pulse), 0, 255);
			int lg = std::clamp(int(greenTint * pulse), 0, 255);
			int lb = std::clamp(int(blueTint * pulse), 0, 255);

			// Colour every pixel inside the visibility polygon using the glow texture centred
			// on the light. Works on the raw pixel data so it stays fast in debug builds.
			SetDrawTarget(nullptr);
			olc::Pixel* screen = GetDrawTarget()->GetData();
			const olc::Pixel* mask = buffLightRay->GetData();
			const olc::Pixel* glow = sprLightCast->GetData();
			int glow_x = int(source_x) - sprLightCast->width / 2;
			int glow_y = int(source_y) - sprLightCast->height / 2;

			for (int y = 0; y < ScreenHeight(); y++)
			{
				for (int x = 0; x < ScreenWidth(); x++)
				{
					int i = y * ScreenWidth() + x;
					if (mask[i].r == 0)
						continue;

					float a = 0.0f;
					int gx = x - glow_x;
					int gy = y - glow_y;
					if (gx >= 0 && gy >= 0 && gx < sprLightCast->width && gy < sprLightCast->height)
					{
						olc::Pixel p = glow[gy * sprLightCast->width + gx];
						if (p.a > 0)
							a = p.r / 255.0f * 0.95f;
					}

					screen[i] = olc::Pixel(int(lr * a), int(lg * a), int(lb * a));
				}
			}
		}

		static float colorTime = 0.0f;
		colorTime += fElapsedTime * 1.5f;

		// Change block gradient and colour here
		int baseR = 35;
		int baseG = 50;
		int baseB = 65;

		int modR = baseR + int(4 * sinf(colorTime * 0.5f));
		int modG = baseG + int(12 * sinf(colorTime * 1.2f));
		int modB = baseB + int(15 * sinf(colorTime * 0.9f));

		olc::Pixel dynamicWallColor(
			std::clamp(modR, 0, 255),
			std::clamp(modG, 0, 255),
			std::clamp(modB, 0, 255)
		);

		for (int x = 0; x < world_width; x++)
			for (int y = 0; y < world_height; y++)
				if (world[y * world_width + x].cell_exist)
					FillRect(x * block_width, y * block_width, block_width, block_width, dynamicWallColor);

		// There's no system cursor when using the keyboard, so mark the cursor's cell
		if (keyboard_cursor)
		{
			olc::Pixel marker = IsEditableCell(grid_x, grid_y) ? olc::YELLOW : olc::DARK_GREY;
			int cell = int(block_width);
			DrawRect(grid_x * cell, grid_y * cell, cell - 1, cell - 1, marker);
			Draw(int(cursor_x), int(cursor_y), marker);
		}

		int text_y = (world_height - 1) * int(block_width) + 4;
		DrawString(4, text_y, "[H] Controls", olc::GREY);
		DrawString(ScreenWidth() - 4 - 8 * 10, text_y, light_on ? "Light: ON " : "Light: OFF", light_on ? olc::Pixel(100, 230, 255) : olc::GREY);

		if (show_help)
			DrawHelp();

		return true;
	}
};

int main() {
	ShadowCasting2D demo;
	if (demo.Construct(640, 480, 2, 2)) {
		demo.Start();
	}
}
