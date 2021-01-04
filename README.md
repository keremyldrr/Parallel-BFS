# Parallel_BFS

Compile with -fopenmp for timing properties.
execute with filepath as second argument. 

Read the file into an adjacency list and convert it to a Compressed Row 
Format.
10x speedup compared to using adjacency list
Preprocessing takes longer time than BFS

0 indexed files dont work properly

Given benchmark reached with -O3 flag for all graphs except 
europe_osm.mtx
