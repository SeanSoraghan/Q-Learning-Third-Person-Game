# Q-Learning-Third-Person-Game
Demo third person maze game made with the Unreal engine that makes use of action-value reward tables for enemy behaviour.
# Dependencies 
### Unreal Engine 4
The Unreal project has been developed on windows and is untested in other platforms.

# Overview
### Gameplay
At the moment the game consists of very simple mechanics. There is a main player character and enemy sphere objects. They both exist within square-grid maze structures. The default camera is a top-down camera. The right mouse button can be used to enter combat mode, which transitions to an over-the-shoulder camera with shooting mechanics. The player can approach doors (white blocks) to generate rooms. This will spawn a new room with a ceiling. As the enemy behaviour is trained for the room, the door will gradually decrease in size (placeholder). Once an enemy policy has been trained for the room, enemies will begin to spawn. In order to stop enemies spawning, the player must reach the spawn point (a box trigger). When the player walks through the spawn point, the enemies will stop spawning and the ceiling will be removed. The presence of enemies within a room will cause the room's health to decrease, indicated by the walls turning from green to red. If the health drops to 0 the room is destroyed. (Everything inside it should also be destroyed -- not implemented yet).
### Repository Structure
The repository includes the Unreal project folder for different versions of the engine (TPGameDemo X.XX). There is also a Blender folder containing all Blender projects and assets.

# Tools Used
### Unreal Engine
https://www.unrealengine.com/what-is-unreal-engine-4
The project is built from scratch, without the use of any templates or starter content. C++ classes are implemented for base functionality, which are then inherited by blueprint classes in the editor.
### Make Human
http://www.makehuman.org/
A great tool for developers without knowledge or skills in artistic 3D modelling. It allows you to create humanoid meshes and rigs using a slider-based interface.
Useful tutorials:  https://www.youtube.com/user/vscorpianc
### Blender
https://www.blender.org/
The characters created in Make Human were imported into Blender in order to create animations, which were then imported into Unreal.
