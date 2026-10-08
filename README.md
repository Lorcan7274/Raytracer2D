<h1 align="center">Raytracer2D</h1>

<p align="center">
  <img src="https://github.com/user-attachments/assets/0d9a9cbf-58a3-4e0d-8459-0f6df2a3829d" width="420" alt="Raytracer demo 2">
</p>

<p align="center">
  A simple raycasting implementation built using the OLC Pixel Game Engine.
</p>



## Controls
Press **H** (or **F1**) in the app to show or hide the controls.

| Action | Mouse / trackpad | Keyboard |
| --- | --- | --- |
| Move cursor / light | Move the mouse | Arrow keys or WASD |
| Light source | Right Click [hold] | Space or L (toggle) |
| Add blocks | Left Click | Enter [hold] |
| Remove blocks | Middle Mouse Button, or Shift + Left Click | Backspace or Delete [hold] |

Everything can be done from the keyboard, so you don't need a mouse. When you use the keyboard, the cell under the cursor is outlined in yellow. Blocks can only be placed inside the outer wall, and the outer wall itself can't be removed.

## Building

**Visual Studio (Windows):** open `Raytracer2D.sln`, pick `x64` or `x86`, and press F5.

**Linux:** install the X11, OpenGL and libpng development packages, then from the repository root run:
```sh
g++ -std=c++17 -O2 -I. src/Source.cpp -o Raytracer2D -lX11 -lGL -lpthread -lpng
./Raytracer2D
```

The program looks for `assets/light_cast.png` relative to where it's run from, including the Visual Studio output folders. If it can't find the file, it generates a matching light texture.


### How it works

The program builds a grid-based world where each filled cell forms part of a continuous edge map. From the light source, rays are cast toward every vertex and slightly offset around each one to detect where they intersect with walls or pass into open space. These intersection points are then sorted by angle to form a visibility polygon, which is filled to simulate light and shadow.  

You can see individual rays instead of filled polygons by replacing:
```cpp
FillTriangle(source_x, source_y, x1, y1, x2, y2, olc::WHITE);
 ```
with:
```cpp
DrawTriangle(source_x, source_y, x1, y1, x2, y2, olc::WHITE);
```

<p align="center">
  <img width="665" height="494" alt="demo_img" src="https://github.com/user-attachments/assets/ff880f06-0e61-4cb3-bf11-b8ef042666d5" />
</p>

### Colors

To change the glow colors:
```cpp
// Change light source brightness / glow / color here
float brightness = 0.6f + 0.1f * sinf(totalTime * 1.6f); 

int redTint   = int(100 * brightness);
int greenTint = int(230 * brightness);
int blueTint  = int(255 * brightness);
```
To change the block colors
```cpp
// Change block gradient and color here
int baseR = 35;
int baseG = 50;
int baseB = 65;

int modR = baseR + int(4 * sinf(colorTime * 0.5f));
int modG = baseG + int(12 * sinf(colorTime * 1.2f));
int modB = baseB + int(15 * sinf(colorTime * 0.9f));
```

### Acknowledgments

This project uses the olcPixelGameEngine by [OneLoneCoder (javidx9)](https://github.com/OneLoneCoder/olcPixelGameEngine),  
distributed under the [OLC-3 License](https://github.com/OneLoneCoder/olcPixelGameEngine/blob/master/license.txt).


