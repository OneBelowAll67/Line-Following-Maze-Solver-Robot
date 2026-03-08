# Line-Following-Maze-Solver-Robot
Part of Maze Competition Season 3 hosted by University of Nottingham's IEEE Student Chapter

This repository contains code that was used to solve a Line Following Maze. It contains a few different algorithms to solve the maze
It contains: PD Control, Hand-On-Wall Rule, DFS and Static Path Persistence.
Additionally, this README doc will roughly go through each method and some details on its mechanical design. For full details feel free to access the code itself to understand!

# PD Control
A combination of Proportional and Derivative Control feedback control algortihm. It corrects real-time errors to ensure that the robot car is aligned with the center.
Additionally, it also helps to reduce oscillations of the robot when moving.
# Hand On Wall Rule
Main algorithm to map out the entire maze. When mapping, it will try to map out all the possible different routes and determine the shortest path.
It then stores the travelled path in a Volatile Memory System in a form of a bunch of single characters. For example: "LSR" where L for Left Turn, S for Straight and R for Right Turn.
# DFS and Static Path Persistence
The main maze solving algorithm. WIP
# Mechanical Design
WIP
