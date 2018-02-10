import unreal_engine as ue

from scipy import * #@UnusedWildImport
import _thread
from threading import Thread
import time
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

class Hero:
    def begin_play(self):
        self.isPendingKill = False
        self.isTrained = False;
        self.structure = []

    def tick(self, delta_time):
        if (not self.isPendingKill):
            if (self.uobject.get_property('MazeGenerationRequired')):
                self.generate()
                self.uobject.set_property('MazeGenerationRequired', False)
            #if (self.uobject.get_property('LevelBuilt') and self.isTrained):
            #    self.isPendingKill = True;
            #    self.uobject.actor_destroy()

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
        self.w = 15
        self.h = 15
        self.structure = self.maze(self.w, self.h, 0.9, 0.9)
        roomsDir = os.path.dirname(full_path) + "/../Levels/GeneratedRooms/"
        levelName = str(self.uobject.get_property('GeneratedRoomCoordinates').x) + '_' + str(self.uobject.get_property('GeneratedRoomCoordinates').y)
        np.savetxt(roomsDir + levelName + ".txt", self.structure, fmt='%1.1d', delimiter= ' ')
        ue.log('Generated Room NAME::: --- ' + roomsDir + levelName + ".txt")
        self.uobject.set_property('RoomGenerated', True)
        #self.generateLevelPolicy()

    def generateLevelPolicy(self):
        self.updateEnemyPolicyDirectory();
        #self.train();
        #_thread.start_new_thread(self.train(), "trainer_thread", 1)
        Thread(target=self.train(), args=('trainer_thread',1)).start()

    def updateEnemyPolicyDirectory(self):
        roomsDir = os.path.dirname(full_path) + "/../Levels/GeneratedRooms/"
        levelName = str(self.uobject.get_property('GeneratedRoomCoordinates').x) + '_' + str(self.uobject.get_property('GeneratedRoomCoordinates').y)
        self.enemyPolicyDirectory = roomsDir + levelName + "/"
        if not os.path.exists(self.enemyPolicyDirectory):
            os.makedirs(self.enemyPolicyDirectory)

    def train_for_goal_position(self, x, y):
        if self.structure[x, y] == 0:
            environment = Maze(self.structure, (x, y))
            controller = ActionValueTable(self.w*self.h, 4)
            controller.initialize(1.)
            learner = Q()
            agent = LearningAgent(controller, learner)
            task = MDPMazeTask(environment)
            experiment = Experiment(task, agent)
            print ("beginning trainging iteration...")
            print(" Row: " + str(x) + " | Col: " + str(y))
            for i in range(700):#700
                experiment.doInteractions(500)#500
                agent.learn()
                agent.reset()
            np.savetxt(self.enemyPolicyDirectory + str(x) + '_' + str(y) + ".txt",
                       controller.params.reshape(self.w*self.h, 4).argmax(axis=1).reshape(self.w, self.h), fmt='%1.1d', delimiter=' ')


    def train(self):
        xindex = 1
        yindex = 1
        for x in range(self.w - 2):
            xindex = x+1
            print(xindex)
            for y in range(self.h - 2):
                yindex = y + 1
                self.train_for_goal_position(xindex, yindex)
        self.isTrained = true
