<h1 align="center">Raytracer2D</h1>

<p align="center">
  <img src="https://github.com/user-attachments/assets/c834f71d-ffa6-4aa9-8eea-777cd3f05f26" width="420" alt="Raytracer demo 1">
  <img src="https://github.com/user-attachments/assets/0d9a9cbf-58a3-4e0d-8459-0f6df2a3829d" width="420" alt="Raytracer demo 2">
</p>

<p align="center">
  A simple raycasting implementation built using the OLC Pixel Game Engine.
</p>



## Controls
- **Left Click** : Add blocks  
- **Right Click [hold]** : Turn on light source  
- **Middle Mouse Button** : Remove blocks  


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


