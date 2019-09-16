Zach Essel

09/11/2019

CIS 3207 001

Project 1: Giorgio's Discrete Event Simulator

The goal of this project is to simulate virtualization of the cpu.  This is accomplished by having processes
gain the cpu for some random amount of time before either exiting or moving on to perform i/o where they then
reenter the cpu queue.  The simulator is not quite time driven, but instead driven by a queue of events which occur
chronologically.  Events are created according to a random time interval and also throughout the running of the program.
After creation events are inserted into the event queue sorted.  Events themselves create, move, and free process nodes.

Events are a structure with the following properties:
an eventid which is a string identifier, because there is no math to be performed on the id itself.
a poptime which is the time an event occurs, since there is no clock in this simulation.
an eventtype, the number used to determine what actions a given event will cause to occur.
Two pointers, one to the previous and one to the next event in the event queue (linked list).

The main bulk of the program is a loop that pops the top event, outputs and performs its associated actions, frees the event, then
restarts the loop.  Inside the loop is a 10 case switch statement that handles the appropriate actions for the current event.
The event types are as follows
0: The end simulation event.  This event is created and enqueued before the loop is launched at the program start.  In the switch statement it flips a flag that will terminate the loop and move the program into the final stages of output.

1: The start simulation event.  This event is created and enqueued before the loop is launched at the program start.  In the switch statement it creates the first process arriva event

2: The process arrival event.  This event is the only way to add new processes into the simulation.  If the cpu queue is empty is will trigger a cpu start event.  After adding the new process to the cpu queue it will add the next process arrival event

3: The cpu start event.  This event will add the event for the cpu exit

4: The cpu exit event.  The event with the most side affects.  This event will dequeue a process from the cpu.  Then it checks whether a new cpu process starts and adds that event if needed.  It also tests whether the current event will exit the simulation and can generate that event.  Finally it adds the dequeued process to one of the disk queues.  Disk 2 will only be enqueued to if it's current queue length is less than that of disk 1.  This balances for different speeds of disk.  Finally it initiates a check for whether those disk queues were empty, and if so initiates a disk entry event.

5: The disk1 entry event.  This event will add the event for the disk1 exit event.

6: The disk1 exit event.  This event will remove the process from the disk1 and enqueue it in the cpu queue.  It also checks if the cpu was empty and if so generates a cpu entry event.

7: The disk2 entry event.  This event will add the event for the disk2 exit event.

8: The disk2 exit event.  This event will remove the process from the disk2 and enqueue it in the cpu queue.  It also checks if the cpu was empty and if so generates a cpu entry event

9: The process exit event.  This event occurs when a process successfully quits after it's cpu cycles.  This event frees the memory that the process was using.

The parameters the program runs on are imported from a file "initializers.dat" and the logs are output to a file "eventoutput.dat"
eventoutput.dat will be created if it does not exist. 

To use the program simply modify the initializers to your liking, then save the file.  Run the EventSim program and it will either create or update the file "eventoutput.dat" in it's directory.  That output file contains all the information on the simulation with the variables you chose.

