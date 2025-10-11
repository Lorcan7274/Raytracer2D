#include <iostream>
#include <cmath>
#include <algorithm>
#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"
#include <vector>
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
	olc::Sprite* buffLightTex;

	std::vector<Edge> vecEdges;
	std::vector<std::tuple<float, float, float>> vecVisibilityPolygonPoints;

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

		sprLightCast = new olc::Sprite("assets/light_cast.png");

		buffLightTex = new olc::Sprite(ScreenWidth(), ScreenHeight());
		buffLightRay = new olc::Sprite(ScreenWidth(), ScreenHeight());
		return true;
	}

	bool OnUserUpdate(float fElapsedTime) override
	{
		float block_width{ 16.0f };
		float source_x = GetMouseX();
		float source_y = GetMouseY();

		if (GetMouse(0).bHeld)
		{
			int grid_x = (int)(source_x / block_width);
			int grid_y = (int)(source_y / block_width);

			if (grid_x >= 0 && grid_x < world_width && grid_y >= 0 && grid_y < world_height)
			{
				int i = grid_y * world_width + grid_x;
				world[i].cell_exist = true;
			}
		}

		if (GetMouse(2).bHeld) {
			int i = ((int)source_y / (int)block_width * world_width + (int)source_x / (int)block_width);
			world[i].cell_exist = !world[i].cell_exist;
		}

		ConvertTileMapToPolyMap(0, 0, 40, 30, block_width, world_width);

		if (GetMouse(1).bHeld) {
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

		if (GetMouse(1).bHeld && vecVisibilityPolygonPoints.size() > 1)
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

		
			SetDrawTarget(buffLightTex);
			Clear(olc::BLACK);

			static float totalTime = 0.0f;
			totalTime += fElapsedTime;
			float pulse = 1.0f + 0.2f * sinf(totalTime * 2.0f);

			for (int x = 0; x < sprLightCast->width; x++)
				for (int y = 0; y < sprLightCast->height; y++)
				{
					olc::Pixel p = sprLightCast->GetPixel(x, y);
if (p.a > 0) 
{
    int nx = int(source_x - sprLightCast->width / 2 + x);
    int ny = int(source_y - sprLightCast->height / 2 + y);
    if (nx >= 0 && ny >= 0 && nx < buffLightTex->width && ny < buffLightTex->height)
    {
        
        

       
		float brightness = 0.6f + 0.1f * sinf(totalTime * 1.6f); 

		
		int redTint = int(100 * brightness);
		int greenTint = int(150 * brightness);
		int blueTint = int(200 * brightness);

		
		float intensity = p.r / 255.0f;
		float a = intensity * 0.95f;

		
		int lr = std::clamp(int(redTint * pulse), 0, 255);
		int lg = std::clamp(int(greenTint * pulse), 0, 255);
		int lb = std::clamp(int(blueTint * pulse), 0, 255);

		olc::Pixel base = buffLightTex->GetPixel(nx, ny);
		int r = std::clamp(int(base.r * (1.0f - a) + lr * a), 0, 255);
		int g = std::clamp(int(base.g * (1.0f - a) + lg * a), 0, 255);
		int b = std::clamp(int(base.b * (1.0f - a) + lb * a), 0, 255);

		buffLightTex->SetPixel(nx, ny, olc::Pixel(r, g, b, 255));

    }
}
				}

	
		
			SetDrawTarget(nullptr);
			for (int x = 0; x < ScreenWidth(); x++)
			{
				for (int y = 0; y < ScreenHeight(); y++)
				{
					olc::Pixel mask = buffLightRay->GetPixel(x, y);
					olc::Pixel tex = buffLightTex->GetPixel(x, y);

					float fade = powf(mask.r / 255.0f, 1.4f); 
					if (fade > 0.001f)
					{
						int r = int(tex.r * fade);
						int g = int(tex.g * fade);
						int b = int(tex.b * fade);
						Draw(x, y, olc::Pixel(r, g, b));
					}
				}
			}
		}
		static float colorTime = 0.0f;
		colorTime += fElapsedTime * 1.5f; 


int baseR = 50;   
int baseG = 70;   
int baseB = 60;  


int modR = baseR + int(5 * sinf(colorTime * 0.7f));
int modG = baseG + int(8 * sinf(colorTime * 0.5f));
int modB = baseB + int(4 * sinf(colorTime * 0.9f));

olc::Pixel dynamicWallColor(
    std::clamp(modR, 0, 255),
    std::clamp(modG, 0, 255),
    std::clamp(modB, 0, 255)
);

		
		for (int x = 0; x < world_width; x++)
			for (int y = 0; y < world_height; y++)
				if (world[y * world_width + x].cell_exist)
					FillRect(x * block_width, y * block_width, block_width, block_width, dynamicWallColor);

		return true; 

	}
};

int main() {
	ShadowCasting2D demo;
	if (demo.Construct(640, 480, 2, 2)) {
		demo.Start();
	}
}
