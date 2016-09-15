# Q-Learning-Third-Person-Game
Demo third person maze game made with the Unreal engine that makes use of action-value reward tables for enemy behaviour.
# Dependencies 
### Unreal Engine 4
The Unreal project has been developed on windows and is untested in other platforms.
### Python (for enemy training)
In order to use the enemy policy training script you'll need Python and the PyBrain machine learning library installed. 

# Overview
### Gameplay
At the moment the game consists of very simple mechanics. There is a main player character and an enemy sphere object. They both exist within a square-grid maze structure. The main player must avoid getting close to the sphere while trying to shoot and kill it. The sphere will continuously navigate the maze towards the player. If it reaches the player, it will stop and the player will begin to take damage. 
### Repository Structure
The repository includes the Unreal project folder (TPGameDemo) and an accompanying Python script (LevelPolicyTraining.py) that can be used to create levels and train enemy behaviour for those levels. Levels are square-grid mazes in which cells can be blocked (1) or open (0).
### Level Creation & Enemy Training
In order to create a level, edit the LevelPolicyTraining.py script directly. Set the desired level name at line 63:
~~~~
levelName = 'LevelName'
~~~~
And then edit the structure array defined at line 65, using 1 = wall, 0 = free space. For example:
~~~~
structure = array([[1, 1, 1, 1, 1, 1, 1, 1, 1, 1],
                   [1, 0, 1, 1, 0, 0, 0, 0, 0, 1],
                   [1, 0, 0, 1, 0, 0, 1, 0, 0, 1],
                   [1, 0, 0, 1, 0, 0, 1, 0, 0, 1],
                   [1, 0, 0, 1, 0, 1, 1, 0, 0, 1],
                   [1, 0, 0, 0, 0, 0, 1, 0, 1, 1],
                   [1, 0, 1, 1, 1, 0, 1, 0, 0, 1],
                   [1, 0, 0, 0, 0, 0, 0, 0, 0, 1],
                   [1, 0, 0, 0, 1, 0, 0, 1, 0, 1],
                   [1, 1, 1, 1, 1, 1, 1, 1, 1, 1]])
~~~~

Once you've made these changes, you can run the script and it will train enemy behaviour policies for this grid using the Q-Learning algorithm provided in the PyBrain library. Once you have run the script, you can load the level in the Unreal project by editing the EmptyLevel level blueprint. Open the level blueprint and change the SetupLevel node such that the Level Name argument matches the name of your newly created level. Note that the player and enemy are currently not guaranteed to spawn in free spaces (this needs to be fixed).
#Tools Used
###Unreal Engine
https://www.unrealengine.com/what-is-unreal-engine-4
The project is built from scratch, without the use of any templates or starter content. C++ classes are implemented for base functionality, which are then inherited by blueprint classes in the editor.
###PyBrain
http://pybrain.org/
A nice machine learning library for Python. This project makes use of the reinforcement learning example, specifically the Q-Learning algorithm.
Useful tutorial: http://simontechblog.blogspot.co.uk/2010/08/pybrain-reinforcement-learning-tutorial_15.html
###Make Human
http://www.makehuman.org/
A great tool for developers without knowledge or skills in artistic 3D modelling. It allows you to create humanoid meshes and rigs using a slider-based interface.
Useful tutorials:  https://www.youtube.com/user/vscorpianc
###Blender
https://www.blender.org/
The characters created in Make Human were imported into Blender in order to create animations, which were then imported into Unreal.
