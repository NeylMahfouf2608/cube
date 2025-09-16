#include <iostream>
#include <vector>
#include <cmath>
#include <chrono>
#include <thread>
#include <algorithm>
#include <string>

#define NOMINMAX
#include <windows.h>

const int WIDTH = 80;
const int HEIGHT = 40;

// Constants for 3D rendering
const float CUBE_SIZE = 12.0f;
const float CAMERA_DISTANCE = 4.0f;
const float FOV = 90.0f;

template <typename T>
T clamp(const T& min, const T& max, const T& value)
{
	return std::min(std::max(value, min), max);
}

const char SHADES[] = ".:-=+*#%@";

struct Point3D
{
	float x, y, z;
};

struct Point2D
{
	int x, y;
	float depth;
};

std::vector<Point3D> cube_vertices =
{
	{-1,-1,-1}, {1,-1,-1}, {1,1,-1}, {-1,1,-1},
	{-1,-1,1}, {1,-1,1}, {1,1,1}, {-1,1,1}
};

std::vector<std::vector<int>> cube_faces = {
	{0,1,2,3}, // back face
	{4,5,6,7}, // front face
	{0,1,5,4}, // bottom face
	{2,3,7,6}, // top face
	{1,2,6,5}, // right face
	{0,3,7,4}  // left face
};

float edgeFunction(const Point2D& A, const Point2D& B, const Point2D& P) {
	return (P.x - A.x) * (B.y - A.y) - (P.y - A.y) * (B.x - A.x);
}

void drawTriangle(std::vector<std::vector<char>>& screen,
	std::vector<std::vector<float>>& zbuffer,
	const Point2D& p0, const Point2D& p1, const Point2D& p2)
{
	int minX = std::min({ p0.x, p1.x, p2.x });
	int maxX = std::max({ p0.x, p1.x, p2.x });
	int minY = std::min({ p0.y, p1.y, p2.y });
	int maxY = std::max({ p0.y, p1.y, p2.y });

	float area = edgeFunction(p0, p1, p2);

	for (int y = minY; y <= maxY; ++y) {
		for (int x = minX; x <= maxX; ++x) {
			Point2D P{ x, y };

			float w0 = edgeFunction(p1, p2, P);
			float w1 = edgeFunction(p2, p0, P);
			float w2 = edgeFunction(p0, p1, P);

			if ((w0 >= 0 && w1 >= 0 && w2 >= 0) ||
				(w0 <= 0 && w1 <= 0 && w2 <= 0)) {

				float alpha = w0 / area;
				float beta = w1 / area;
				float gamma = w2 / area;

				float depth = alpha * p0.depth + beta * p1.depth + gamma * p2.depth;

				if (x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT) {
					if (depth < zbuffer[y][x]) {
						zbuffer[y][x] = depth;

						int shadeIdx = static_cast<int>((depth - CAMERA_DISTANCE) * 2);
						shadeIdx = clamp(shadeIdx, 0, (int)sizeof(SHADES) - 2);
						char c = SHADES[shadeIdx];

						screen[y][x] = c;
					}
				}
			}
		}
	}
}

Point2D project(const Point3D& p)
{
	float aspect = static_cast<float>(WIDTH) / HEIGHT;
	float fovScale = 1.0f / tan(FOV * 0.5f * 3.14159f / 180.0f);

	float z = p.z + CAMERA_DISTANCE;
	float scale = fovScale / z * CUBE_SIZE;

	return
	{
		static_cast<int>(WIDTH / 2 + p.x * scale * aspect),
		static_cast<int>(HEIGHT / 2 - p.y * scale),
		z
	};
}

void rotateX(Point3D& p, float angle)
{
	float y = p.y * cos(angle) - p.z * sin(angle);
	float z = p.y * sin(angle) + p.z * cos(angle);
	p.y = y;
	p.z = z;
}

void rotateY(Point3D& p, float a)
{
	float x = p.x * cos(a) + p.z * sin(a);
	float z = -p.x * sin(a) + p.z * cos(a);
	p.x = x;
	p.z = z;
}

int main()
{
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

	// Hide cursor
	CONSOLE_CURSOR_INFO cursorInfo;
	GetConsoleCursorInfo(hConsole, &cursorInfo);
	cursorInfo.bVisible = FALSE;
	SetConsoleCursorInfo(hConsole, &cursorInfo);

	float angleX = 0, angleY = 0;

	while (true)
	{
		std::vector<std::vector<char>> screen(HEIGHT, std::vector<char>(WIDTH, ' '));
		std::vector<std::vector<float>> zbuffer(HEIGHT, std::vector<float>(WIDTH, 1e9));

		std::vector<Point3D> rotated = cube_vertices;
		for (auto& v : rotated)
		{
			rotateX(v, angleX);
			rotateY(v, angleY);
		}

		std::vector<Point2D> projected;
		for (const auto& v : rotated)
		{
			projected.push_back(project(v));
		}

		for (const auto& f : cube_faces) {
			Point2D p0 = projected[f[0]];
			Point2D p1 = projected[f[1]];
			Point2D p2 = projected[f[2]];
			Point2D p3 = projected[f[3]];

			drawTriangle(screen, zbuffer, p0, p1, p2);
			drawTriangle(screen, zbuffer, p0, p2, p3);
		}

		// Convert screen buffer into CHAR_INFO[]
		CHAR_INFO buffer[WIDTH * HEIGHT];
		SMALL_RECT rect = { 0, 0, WIDTH - 1, HEIGHT - 1 };

		for (int y = 0; y < HEIGHT; y++) {
			for (int x = 0; x < WIDTH; x++) {
				buffer[y * WIDTH + x].Char.AsciiChar = screen[y][x];
				buffer[y * WIDTH + x].Attributes = FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_RED;
			}
		}

		COORD bufferSize = { WIDTH, HEIGHT };
		COORD bufferCoord = { 0, 0 };
		WriteConsoleOutputA(hConsole, buffer, bufferSize, bufferCoord, &rect);

		angleX += 0.03f;
		angleY += 0.02f;

		std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60 FPS
	}

	return 0;
}