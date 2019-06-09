# memofpid

## Description

*memofpid* is a software for calculating the RAM consumption of a PID and its child processes using the RSS value from each PID.

It has two basic forms for showing the result: on the one hand a summary and detailed table and on the other hand the sum of all the processes.

## Use

```
$ ./memofpid <-p PID> [-t] [-u {B,K,M,G}]
```

* -t        Make a deatiled table
* -u UNIT   Selects the units for memory field. Available values are: B,K,M,G.

If you don't pass `-t` as argument, *memofpid* will shown the sum of all processes.

## Compiling

```
$ git clone https://github.com/manuel-alcocer/linux-utils.git
$ cd linux-utils
$ git checkout stable-0.1
$ make
```

