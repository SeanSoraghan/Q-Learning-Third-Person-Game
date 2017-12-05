############################################################################
# PyBrain Tutorial "Reinforcement Learning"
#
# Author: Thomas Rueckstiess, ruecksti@in.tum.de
############################################################################

__author__ = 'Thomas Rueckstiess, ruecksti@in.tum.de'

"""
This is a modified version of the pybrain reinforcement learning tutorial.
It can be used to create a number of text files which consist of a grid of 
greedy-optimal actions (0,1,2,3 - north, east, south, west) for the given 
grid structure (the 'structure' array). Each of the text files are named 
according to the location of the goal for that policy (e.g. 1_1.txt is the
policy that should be followed when the goal is at grid position (1, 1)).

"""


"""
A reinforcement learning (RL) task in pybrain always consists of a few
components that interact with each other: Environment, Agent, Task, and
Experiment. In this tutorial we will go through each of them, create
the instances and explain what they do.

But first of all, we need to import some general packages and the RL
components from PyBrain:
"""

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

"""
For later visualization purposes, we also need to initialize the
plotting engine.
"""

# pylab.gray()
# pylab.ion()
# pylab.show()

"""
The Environment is the world, in which the agent acts. It receives input
with the .performAction() method and returns an output with
.getSensors(). All environments in PyBrain are located under
pybrain/rl/environments.

One of these environments is the maze environment, which we will use for
this tutorial. It creates a labyrinth with free fields, walls, and an
goal point. An agent can move over the free fields and needs to find the
goal point. Let's define the maze structure, a simple 2D numpy array, where
1 is a wall and 0 is a free field:
"""

#levelName = 'LevelExample1'

# structure = array([[1, 1, 1, 1, 1, 1, 1, 1, 1, 1],
#                   [1, 0, 1, 1, 0, 0, 0, 0, 1, 1],
#                   [1, 0, 0, 1, 0, 0, 0, 0, 0, 1],
#                   [1, 1, 0, 1, 0, 0, 1, 0, 0, 1],
#                   [1, 1, 0, 1, 0, 1, 1, 0, 0, 1],
#                   [1, 1, 0, 0, 0, 0, 1, 0, 1, 1],
#                   [1, 0, 1, 1, 1, 0, 1, 0, 0, 1],
#                   [1, 0, 0, 0, 0, 0, 0, 0, 0, 1],
#                   [1, 0, 1, 0, 0, 0, 1, 1, 0, 1],
#                   [1, 1, 1, 1, 1, 1, 1, 1, 1, 1]])

#print (structure)
#np.savetxt("TPGameDemo 4.18/Content/Levels/" + levelName + ".txt", structure, fmt='%1.1d', delimiter= ' ')


def maze(width=51, height=51, complexity=.75, density=.75):
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
    print(Z)
    # Make aisles
    for i in range(density):
        x, y = rand(0, shape[1] // 2) * 2, rand(0, shape[0] // 2) * 2
        #Z[y, x] = 1
        Z[x, y] = 1 
        print("Y: " + str(y) + " | X: " + str(x))
        print(Z)
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
                    print(neighbours)
                    print("Y_n: " + str(y_) + " | X_n: " + str(x_))
                    print("Y_n + ...: " + str(y_ + (y - y_) // 2) + " | X_n + ...: " + str(x_ + (x - x_) // 2))
                    print(Z)
                    x, y = x_, y_
    return Z

w = 15
h = 15
structure = maze(w, h, 0.9, 0.9)
levelName = 'WikipediaExample'
np.savetxt("TPGameDemo 4.18/Content/Levels/" + levelName + ".txt", structure, fmt='%1.1d', delimiter= ' ')

enemyPolicyDirectory = "TPGameDemo 4.18/Content/Characters/Enemies/LevelPolicies/" + levelName + "/"
if not os.path.exists(enemyPolicyDirectory):
    os.makedirs(enemyPolicyDirectory)

"""
Then we create the environment with the structure as first parameter
and the goal field tuple as second parameter:
"""

""" environment = Maze(structure, (7, 7)) """

"""
Next, we need an agent. The agent is where the learning happens. It can
interact with the environment with its .getAction() and
.integrateObservation() methods.

The agent itself consists of a controller, which maps states to actions,
a learner, which updates the controller parameters according to the
interaction it had with the world, and an explorer, which adds some
explorative behaviour to the actions. All standard agents already have a
default explorer, so we don't need to take care of that in this
tutorial.

The controller in PyBrain is a module, that takes states as inputs and
transforms them into actions. For value-based methods, like the
Q-Learning algorithm we will use here, we need a module that implements
the ActionValueInterface. There are currently two modules in PyBrain
that do this: The ActionValueTable for discrete actions and the
ActionValueNetwork for continuous actions. Our maze uses discrete
actions, so we need a table:
"""

"""
controller = ActionValueTable(81, 4)
controller.initialize(1.)
"""

"""
The table needs the number of states and actions as parameters. The standard
maze environment comes with the following 4 actions: north, east, south, west.

Then, we initialize the table with 1 everywhere. This is not always necessary
but will help converge faster, because unvisited state-action pairs have a
promising positive value and will be preferred over visited ones that didn't
lead to the goal.

Each agent also has a learner component. Several classes of RL learners
are currently implemented in PyBrain: black box optimizers, direct
search methods, and value-based learners. The classical Reinforcement
Learning mostly consists of value-based learning, in which of the most
well-known algorithms is the Q-Learning algorithm. Let's now create
the agent and give it the controller and learner as parameters.
"""

"""
learner = Q()
agent = LearningAgent(controller, learner)
"""

"""
So far, there is no connection between the agent and the environment. In fact,
in PyBrain, there is a special component that connects environment and agent: the
task. A task also specifies what the goal is in an environment and how the
agent is rewarded for its actions. For episodic experiments, the Task also
decides when an episode is over. Environments usually bring along their own
tasks. The Maze environment for example has a MDPMazeTask, that we will use.
MDP stands for "markov decision process" and means here, that the agent knows
its exact location in the maze. The task receives the environment as parameter.
"""

"""
task = MDPMazeTask(environment)
"""

"""
Finally, in order to learn something, we create an experiment, tell it both
task and agent (it knows the environment through the task) and let it run
for some number of steps or infinitely, like here:
"""

"""
experiment = Experiment(task, agent)
"""


def train_for_goal_position(x, y):
    if structure[x, y] == 0:
        environment = Maze(structure, (x, y))
        controller = ActionValueTable(w*h, 4)
        controller.initialize(1.)
        learner = Q()
        agent = LearningAgent(controller, learner)
        task = MDPMazeTask(environment)
        experiment = Experiment(task, agent)
        print ("beginning trainging iteration...")
        print(" Row: " + str(x) + " | Col: " + str(y))
        for i in range(700):
            # if i % 100 == 0:
            #     print (i)
            experiment.doInteractions(500)
            agent.learn()
            agent.reset()
            """
            if i % 100 == 0:
              print(controller.params.reshape(w*h,4).argmax(axis=1).reshape(w,h))
            """
            """
            pylab.pcolor(controller.params.reshape(w*h, 4).max(1).reshape(w,h))
            pylab.draw()
            """
        np.savetxt(enemyPolicyDirectory + str(x) + '_' + str(y) + ".txt",
                   controller.params.reshape(w*h, 4).argmax(axis=1).reshape(w, h), fmt='%1.1d', delimiter=' ')


def train():
    xindex = 1
    yindex = 1
    for x in range(w - 2):
        xindex = x+1
        print(xindex)
        for y in range(h - 2):
            yindex = y + 1
            train_for_goal_position(xindex, yindex)

train()


"""
print(controller.params.reshape(100,4).argmax(axis=1).reshape(10,10))
"""

"""
Above, the experiment executes 100 interactions between agent and
environment, or, to be exact, between the agent and the task. The task
will process the agent's actions, possibly scale it and hand it over to
the environment. The environment responds, returns the new state back to
the task which decides what information should be given to the agent.
The task also gives a reward value for each step to the agent.

After 100 steps, we call the agent's .learn() method and then reset it.
This will make the agent forget the previously executed steps but of
course it won't undo the changes it learned.

Then the loop is repeated, until a desired behaviour is learned.

In order to observe the learning progress, we visualize the controller
with the last two code lines in the loop. The ActionValueTable consists
of a scalar value for each state/action pair, in this case 81x4 values.
A nice way to visualize learning is to only consider the maximum value
over all actions for each state. This value is called the state-value V
and is defined as V(s) = max_a Q(s, a).

We plot the new table after learning and resetting the agent, INSIDE the
while loop. Running this code, you should see the shape of the maze and
a change of colors for the free fields. During learning, colors may jump
and change back and forth, but eventually the learning should converge
to the true state values, having higher scores (brigher fields) the
closer they are to the goal.
"""


pyplot.figure(figsize=(10, 5))
pyplot.imshow(structure, cmap=pyplot.cm.binary, interpolation='nearest')
pyplot.xticks([]), pyplot.yticks([])
pyplot.show()
