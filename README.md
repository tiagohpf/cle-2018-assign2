# cle-2018-assign2
Development of a simulator of acoustic model. To have the final result, you just need to use two directories: Task1 and MPI.

## Content
- **Task1** - development of a file with data to configurate an acoustic room;
- **Task2** - division of processes in blocks;
- **Task3** - DMW's acoustic modelation in a sequential way;
- **MPI** - DMW's acoustic modelation in a parallel way using MPI;

## How To Run
After the roo'ms configuration file has been created and _main.c_ has been compiled, execute the application with the following command:
```
mpirun -np N --oversubscribe filename fileconfig audiofile X Y Z rev
```
Parameters:
- **N** - número de processos;
- **filename** - name of executable file;
- **fileconfig** - name of file with room's configuration data;
- **audiofile** - name of input file;
- **X** - x's value;
- **Y** - y's value;
- **Z** - z's value;
- **rev** - reverberation's value;

Example:
```
mpirun -np 8 --oversubscribe main room-config.dwm seagull.wav 2 2 2 0
```
