# Running many runs

You can simulate many runs at once like:
```
./parallel --jobs 5 --joblog ./logSim.out < ./input/simFiss.txt
```

where `--jobs` sets the maximum number of jobs to run at once. `--joblog ./fileName` sets the log file which will save the command, runtime, and return/error code. The file piped in contains each command to run (one per line).

To run the simulation is `./input/simFiss.txt`
To run the digitization is `./input/digiFiss.txt`
