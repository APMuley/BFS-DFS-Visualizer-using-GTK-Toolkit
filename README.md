DFS and BFS Visualizer using GTK

This project is a grid-based graph traversal visualizer built in C using the GTK toolkit. Users can select a start and end node on a grid, and the application demonstrates the traversal process of Depth-First Search (DFS) and Breadth-First Search (BFS) in real time. Nodes are highlighted as they are visited, allowing users to clearly see how the algorithms explore the grid to reach the target.

Dependencies:

gtk+3
pkg-config


Command to run the program:

gcc main.c -o main pkg-config gtk+-3.0 --cflags pkg-config gtk+-3.0 --libs

