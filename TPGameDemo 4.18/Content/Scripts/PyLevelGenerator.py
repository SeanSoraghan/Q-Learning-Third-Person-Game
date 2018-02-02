import unreal_engine as ue

from scipy import * #@UnusedWildImport
import numpy as np
import pylab
import os
from pybrain.rl.environments.mazes import Maze, MDPMazeTask
from pybrain.rl.learners.valuebased import ActionValueTable
from pybrain.rl.agents import LearningAgent
from pybrain.rl.learners import Q, SARSA #@UnusedImport
from pybrain.rl.experiments import Experiment
from numpy.random import random_integers as rand
import matplotlib.pyplot as pyplot



ue.log('Hello i am a Python module')


full_path = os.path.realpath(__file__)
print(os.path.dirname(full_path) + "/../Levels/GeneratedRooms")

np.savetxt(os.path.dirname(full_path) + "/../Levels/GeneratedRooms" + "/1.txt", [1], fmt='%1.1d', delimiter= ' ')

class Hero:
    def begin_play(self):
        self.isPendingKill = False
        self.structure = []

    def tick(self, delta_time):
        if (not self.isPendingKill):
            if (self.uobject.get_property('MazeGenerationRequired')):
                self.generate()
                self.uobject.set_property('MazeGenerationRequired', False)
            if (self.uobject.get_property('LevelBuilt')):
                self.isPendingKill = True;
                self.uobject.actor_destroy()

    def maze(self, width=51, height=51, complexity=.75, density=.75):
        # Only odd shapes
        shape = ((height // 2) * 2 + 1, (width // 2) * 2 + 1)
        # Adjust complexity and density relative to maze size
        complexity = int(complexity * (5 * (shape[0] + shape[1])))
        density    = int(density * ((shape[0] // 2) * (shape[1] // 2)))
        # Build actual maze
        Z = np.zeros(shape, dtype=bool)
        # Fill borders
        Z[0, :] = Z[-1, :] = 1
        Z[:, 0] = Z[:, -1] = 1
        #print(Z)
        # Make aisles
        for i in range(density):
            x, y = rand(0, shape[1] // 2) * 2, rand(0, shape[0] // 2) * 2
            #Z[y, x] = 1
            Z[x, y] = 1 
            #print("Y: " + str(y) + " | X: " + str(x))
            #print(Z)
            for j in range(complexity):
                neighbours = []
                if x > 1:             neighbours.append((y, x - 2))
                if x < shape[1] - 2:  neighbours.append((y, x + 2))
                if y > 1:             neighbours.append((y - 2, x))
                if y < shape[0] - 2:  neighbours.append((y + 2, x))
                if len(neighbours):
                    y_,x_ = neighbours[rand(0, len(neighbours) - 1)]
                    #if Z[y_, x_] == 0:
                    if Z[x_, y_] == 0: 
                        #Z[y_, x_] = 1
                        Z[x_, y_] = 1 
                        #Z[y_ + (y - y_) // 2, x_ + (x - x_) // 2] = 1
                        Z[x_ + (x - x_) // 2, y_ + (y - y_) // 2] = 1 
                        #print(neighbours)
                        #print("Y_n: " + str(y_) + " | X_n: " + str(x_))
                        #print("Y_n + ...: " + str(y_ + (y - y_) // 2) + " | X_n + ...: " + str(x_ + (x - x_) // 2))
                        #print(Z)
                        x, y = x_, y_
        return Z

    def generate(self):
        w = 15
        h = 15
        self.structure = self.maze(w, h, 0.9, 0.9)
        roomsDir = os.path.dirname(full_path) + "/../Levels/GeneratedRooms/"
        levelName = str(self.uobject.get_property('GeneratedRoomCoordinates').x) + '_' + str(self.uobject.get_property('GeneratedRoomCoordinates').y)
        np.savetxt(roomsDir + levelName + ".txt", self.structure, fmt='%1.1d', delimiter= ' ')
        ue.log('Generated Room NAME::: --- ' + roomsDir + levelName + ".txt")
        self.uobject.set_property('RoomGenerated', True)
