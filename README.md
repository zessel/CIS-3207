Zach Essel

09/11/2019

CIS 3207 001

Project 1: Giorgio's Discrete Event Simulator

The goal of this project is to simulate virtualization of the cpu.  This is accomplished by having processes
gain the cpu for some random amount of time before either exiting or moving on to perform i/o where they then
reenter the cpu queue.  The simulator is not quite time driven, but instead driven by a queue of events which occur
chronologically.  Events are created according to a random time interval and also throughout the running of the program.
After creation events are inserted into the event queue sorted.  Events themselves create, move, and free process nodes.

The main program is a loop that pops the top event, outputs and performs its associated actions, frees the event, then
restarts the loop.

The parameters the program runs on are imported from a file "initializers.dat" and the logs are output to a file "eventoutput.dat"
eventoutput.dat will be created if it does not exist. 
