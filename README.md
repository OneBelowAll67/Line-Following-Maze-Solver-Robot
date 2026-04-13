# Line Following Maze Solver Robot with Arduino
Part of Maze Competition Season 3 hosted by University of Nottingham's IEEE Student Chapter

This repository contains code that was used to solve a Line Following Maze. It contains a few different algorithms to solve the maze
It contains: PD Control, Hand-On-Wall Rule, DFS and Static Path Persistence.
Additionally, this README doc will roughly go through each method and some details on its mechanical design. For full details feel free to access the code itself to understand!

Something to note that the following libraries are needed to fully run the code. It is as below:
1. EEPROM.h - able to write EEPROM values in the Arduino. (used for storing the mapped out mazed)
2. LiquidCrystal.h - able to use the functions of the LCD Keypad Shield to access the EEPROM memory and display text.
3. Arduino.h (optional) - it is only needed if you run the code in .cpp as it is a library for C++ to read and run Arduino.

Do feel free to refer to the powerpoint presentation attached alongside in the repository for detailed explaination!

# PD Control
A combination of Proportional and Derivative Control feedback control algortihm. It corrects real-time errors to ensure that the robot car is aligned with the center.
Additionally, it also helps to reduce oscillations of the robot when moving.
# Hand On Wall Rule
Main algorithm to map out the entire maze. When mapping, it will try to map out all the possible different routes and determine the shortest path.
It then stores the travelled path in a Volatile Memory System in a form of a bunch of single characters. For example: "LSR" where L for Left Turn, S for Straight and R for Right Turn.
# DFS and Static Path Persistence
The main maze solving algorithm, a combination of DFS (Depth First Search) and Static Path Persistence ensures the robot is able to solve the maze quickly and efficiently. By using the previous algorithm Hand-On-Wall Rule, it will initially map out all the possible roots in the first run. When initializing the second run, the robot will avoid previously traversed paths to prevent infinite loops or duplicate memory.
# Mechanical Design
WIP 2

