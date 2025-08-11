# OpenGL-Space-Simulation
Flying Spaceship Simulation 
This project is an OpenGL-based simulation of a spaceship flying in a 3D space, 
navigating through planets and interacting with a stationary space station.
The program follows shaderbased rendering using OpenGL 3.2+ and implements perspective viewing. 

Features Implemented
- Spaceship Rendering and Movement
  - The spaceship is constructed using two tori and a tetrahedron to indicate its front.
  - It moves in the x-y plane at a constant speed, with adjustable velocity.
  - The user can control speed and direction using keyboard inputs.
  
- Planets Rendering
  - Planets are represented as spheres with distinct colors.
  - They are placed at fixed coordinates in space.
  
- Space Station
  - The station is a large graysphere with a distinguishable front.
  - It rotates around the z-axis with adjustable angular speed.
  
- Ground Rendering
  - The ground is a large white square on the x-y plane.
  
- Camera Views
  - Four different viewing modes:
    1. **Cockpit view (`c`)**: Positioned at the spaceship s control desk, looking in its movement direction.
    2. **Station front view (`s`)**: Positioned at the front of the space station, aligned with its rotation.
    3. **Third-person spaceship view (`t`)**: Positioned behind and above the spaceship.
    4. **Overhead view (`w`)**: Positioned high above the scene to view all objects.

- Controls and Interaction
  - `a` / `d`: Increase or decrease spaceship speed.
  - Arrow keys: Turn spaceship left or right.
  - `j` / `k`: Adjust space station s rotational speed.
  - `p`: Pause/resume the simulation.

System Requirements
- Operating System: Windows (Tested on Windows 10)
- Libraries:
  - OpenGL
  - GLEW
  - FreeGLUT
  - `Angel's mat.h and vec.h` 

Source Files
- main.cpp => Contains the main logic with shaders embedded as string literals.


Ayse Sadioglu-191101077
