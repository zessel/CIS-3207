/*  Zach Essel
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
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>

/*  This is the structure for the processes that go between cpu and i/o
    the contain an processid stored as a string, because no mathematical
    operations are done using it.  They also contain a process pointer
    so that they can be queued up
*/

struct process
{
    char processid[8];
    struct process *next;     
};

/*  This is the structure for events which make up the priority queue.
    They have previous and next event pointers, so they are actually more of a linked list.
    eventid is a string that associates the event with a process        TODO: rename this
    poptime is the global time at which the event occurs
    eventtype signifies the action that the process action the event is associated with

    Event types:
    0: End simulation events
    1: Start simulation events
    2: New process in simulation
    3: Enter CPU
    4: Leave CPU
    5: Arrive at Disk 1
    6: Leave Disk 1
    7: Arrive at Disk 2
    8: Leave Disk 2
    9: Exit System
*/
struct event
{
    char eventid[8];
    int poptime;
    int eventtype;
    struct event *prev;
    struct event *next;
};

void initialize_from_file();
int ranged_rand(int max, int min);
struct event* popevent(struct event **event_queue_root);
void print_event(struct event *current_event);
void print_created(struct event *current_event);
struct event* create_event(char* id, int time, int type);
void sorted_event_enqueue(struct event **event_queue_root, struct event *new_event);
struct process* create_process(char* id);
void enqueue(struct process **queue_tail, struct process *new_process);
struct process* dequeue(struct process **queue_head);
void process_arrival(struct process **cpu_queue_tail, char* id);
//void cpu_start(struct process **cpu_queue_head, struct process **disk_tail);  TODO: Flesh out or remove
//void cpu_finish(struct process **cpu_queue_head, struct process **disk_tail);
void printToOutput(FILE *output, struct event *current_event);



int SEED,
    INIT_TIME,
    FIN_TIME,
    ARRIVE_MIN,
    ARRIVE_MAX,
    QUIT_PROB,
    CPU_MIN,
    CPU_MAX,
    DISK1_MIN,
    DISK1_MAX,
    DISK2_MIN,
    DISK2_MAX;

void main ()
{
    initialize_from_file();

    char outputfilename[] = "eventoutput.dat";
    FILE *output;
    output = fopen(outputfilename, "w");

    printf("\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n", SEED,INIT_TIME,FIN_TIME,ARRIVE_MIN,
        ARRIVE_MAX,
        QUIT_PROB,
        CPU_MIN,
        CPU_MAX,
        DISK1_MIN,
        DISK1_MAX,
        DISK2_MIN,
        DISK2_MAX);
    srand((unsigned int)SEED);  // So I don't forget rand() % (MAX - MIN + 1) + MIN 
    int globaltime = INIT_TIME;
    int processcount = 1;

    struct process *cpu_queue_head = NULL;
    struct process *cpu_queue_tail = NULL;
    struct process *disk1_queue_head = NULL;
    struct process *disk1_queue_tail = NULL;
    struct process *disk2_queue_head = NULL;
    struct process *disk2_queue_tail = NULL;
    struct process *current_process = NULL;
    int disk1_queue_count = 0;
    int disk2_queue_count = 0;

    struct event *event_queue_root = NULL;
    struct event *current_event = NULL;
    struct event *new_event = NULL;

    struct event *start_event = (struct event*) malloc(sizeof(struct event));
    strcpy(start_event->eventid, "START");
    start_event->poptime = INIT_TIME;
    start_event->eventtype = 1;
    start_event->prev = NULL;

    struct event *end_event = (struct event*) malloc(sizeof(struct event));
    strcpy(end_event->eventid, "END");
    end_event->poptime = FIN_TIME;
    end_event->eventtype = 0;
    end_event->prev = start_event;
    start_event->next = end_event;

    event_queue_root = start_event;        

    int endhit = 0;
    char id_as_str[8];


    while (endhit != 1)
    {
        current_event = popevent(&event_queue_root);
        printToOutput(output, current_event);
        globaltime = current_event->poptime;
        if (strcmp(current_event->eventid, "END") == 0)
            endhit = 1;
        sprintf(id_as_str, "%d", processcount);
        
        switch(current_event->eventtype)
        {
        case 1: new_event = create_event(id_as_str, globaltime + ranged_rand(ARRIVE_MAX,ARRIVE_MIN), 2);
            sorted_event_enqueue(&event_queue_root, new_event);
            processcount++;
            break;
        case 2: process_arrival(&cpu_queue_tail, current_event->eventid);
            current_process = create_process(current_event->eventid);
            enqueue(&cpu_queue_tail, current_process);
            if (cpu_queue_head == NULL)
            {
                cpu_queue_head = cpu_queue_tail;
                new_event = create_event(current_event->eventid, globaltime, 3);
                sorted_event_enqueue(&event_queue_root, new_event);
            }
            new_event = create_event(id_as_str, globaltime + ranged_rand(ARRIVE_MAX,ARRIVE_MIN), 2);
            sorted_event_enqueue(&event_queue_root, new_event);
            processcount++;
            break;
        case 3: new_event = create_event(current_event->eventid, globaltime + ranged_rand(CPU_MAX, CPU_MIN), 4);
            sorted_event_enqueue(&event_queue_root, new_event);
            break;
        case 4: current_process = dequeue(&cpu_queue_head);
            if (cpu_queue_head != NULL)
            {
                new_event = create_event(cpu_queue_head->processid, globaltime, 3);
                sorted_event_enqueue(&event_queue_root, new_event);
            }
            if (ranged_rand(100,0) < (QUIT_PROB))
            {
                new_event = create_event(cpu_queue_head->processid, globaltime, 9);
                sorted_event_enqueue(&event_queue_root, new_event);
            }
            else
            {
                if (disk1_queue_count >= disk2_queue_count)
                {
                    enqueue(&disk1_queue_tail, current_process);
                    if (disk1_queue_head == NULL)
                    {
                        disk1_queue_head = current_process;                      
                        new_event = create_event(current_process->processid, globaltime, 5);
                        sorted_event_enqueue(&event_queue_root, new_event);
                    }
                    disk1_queue_count++;
                }
                else
                {
                    enqueue(&disk2_queue_tail, current_process);
                    if (disk2_queue_head == NULL)
                    {
                        disk2_queue_head = current_process;
                        new_event = create_event(current_process->processid, globaltime, 7);
                        sorted_event_enqueue(&event_queue_root, new_event);
                    }
                    disk2_queue_count++;
                }
            }
            break;
        case 5: new_event = create_event(disk1_queue_head->processid, globaltime + ranged_rand(DISK1_MAX,DISK1_MIN), 6);
            sorted_event_enqueue(&event_queue_root, new_event);
            break;
        case 6: current_process = dequeue(&disk1_queue_head);
            if (disk1_queue_head != NULL)
            {
                new_event = create_event(disk1_queue_head->processid, globaltime, 5);            
                sorted_event_enqueue(&event_queue_root, new_event);
            }
            enqueue(&cpu_queue_tail, current_process);
            if (cpu_queue_head == NULL)
            {
                cpu_queue_head = cpu_queue_tail;
                new_event = create_event(current_event->eventid, globaltime, 3);
                sorted_event_enqueue(&event_queue_root, new_event);
            }
            break;
        case 7: new_event = create_event(disk2_queue_head->processid, globaltime + ranged_rand(DISK2_MAX,DISK2_MIN), 8);
            sorted_event_enqueue(&event_queue_root, new_event);
            break;
        case 8: current_process = dequeue(&disk2_queue_head);
        if (disk2_queue_head != NULL)
        {
            new_event = create_event(disk2_queue_head->processid, globaltime, 7);
            sorted_event_enqueue(&event_queue_root, new_event);
        }
            enqueue(&cpu_queue_tail, current_process);
            if (cpu_queue_head == NULL)
            {
                cpu_queue_head = cpu_queue_tail;
                new_event = create_event(current_event->eventid, globaltime, 3);
                sorted_event_enqueue(&event_queue_root, new_event);
            }        
            break;
        case 9: free(current_process);
            break;
        default:        printf("\nCASE DEFAULT\n");
        sleep(1);
            break;
        }
        free(current_event);
    }
    fclose(output);
}

/*  initialize_from_file reads in the globals from the input file.
    I think there is a way to change this into a header so that I
    can keep these all as constants.

    TODO: Try to refactor this into a header file
*/
void initialize_from_file()
{
    char inputfilename[] = "initializers.dat";
    FILE *input;

    input = fopen(inputfilename, "r");
    if (input == NULL)
    {
        printf("The file %s was not found", inputfilename);
    }

        fscanf(input, "%*s %d", &SEED);
    fscanf(input, "%*s %d", &INIT_TIME);
    fscanf(input, "%*s %d", &FIN_TIME);
    fscanf(input, "%*s %d", &ARRIVE_MIN);
    fscanf(input, "%*s %d", &ARRIVE_MAX);
    fscanf(input, "%*s %d", &QUIT_PROB);
    fscanf(input, "%*s %d", &CPU_MIN);
    fscanf(input, "%*s %d", &CPU_MAX);
    fscanf(input, "%*s %d", &DISK1_MIN);
    fscanf(input, "%*s %d", &DISK1_MAX);
    fscanf(input, "%*s %d", &DISK2_MIN);
    fscanf(input, "%*s %d", &DISK2_MAX);
}

/*  ranged_rand exists to slightly clean up the code when a random is called  

    Arguments:
    max - An int representing the maximum value of the range of randoms you want

    min - An int representing the minimum value of the range of randoms you want

    Return:
    An int between the range of min - max
*/
int ranged_rand(int max, int min)
{
    return (rand() % (max - min + 1) + min);
}

/*  popevent removes the event at the top of the event priority queue.
    That event is assumed to be the event with the lowest poptime but
    this method does not check to ensure that.  After popping the event it
    checks if the event queue is empty and if not, sets the next event as the new root.

    TODO: This check shouldn't be needed, because there should never be a single event
    .      It was from earlier unit testing.  Consider removing.

    Arguments:
    event_queue_root - a pointer to a pointer to the root node of the event queue

    Return:
    A pointer to the popped event 
*/
struct event* popevent(struct event **event_queue_root)
{
    struct event *old_root = *event_queue_root;
    if ((*event_queue_root)->next != NULL)
    {
        *event_queue_root = (*event_queue_root)->next;
        old_root->next = NULL;
        (*event_queue_root)->prev = NULL;
    }
    return old_root; 
}

/*  print_event prints an event structures data to the terminal.
    It was used for debugging and testing

    Arguments:
    current_event - a pointer to an event structure whose data will be printed

    Return:
    nothing
*/
void print_event(struct event *current_event)
{
    printf("\nThe event id is %s", current_event->eventid);
    printf("\nThe event time is %d", current_event->poptime);
    printf("\nThe event type is %d", current_event->eventtype);
    printf("\nDONE PRINTING\n");
    sleep(1);
}

/*  print_created prints an event structures data to the terminal.
    It was used for debugging and testing.
    It only differs from print_event in that it prints "*****" before the
    information.  This difference should probably have been an argument not
    a seperate method

    Arguments:
    current_event - a pointer to an event structure whose data will be printed

    Return:
    nothing
*/
void print_created(struct event *current_event)
{
    printf("\n*****The event id is %s", current_event->eventid);
    printf("\n*****The event time is %d", current_event->poptime);
    printf("\n*****The event type is %d", current_event->eventtype);
    printf("\n*****DONE PRINTING\n");
    sleep(1);
}

/*  create_event handles the creation of new events.
    It contains the actual malloc command and the initializing of the
    event variables.

    Arguments:
    id - a string to identify the process associated with the event

    time - the time the event will occur, used for sorting events

    type - the event type.  The various event types are listed previously

    Return:
    a point to the newly created event
*/
struct event* create_event(char* id, int time, int type)
{
    struct event *new_event = (struct event*) malloc(sizeof(struct event));

    strcpy(new_event->eventid, id);
    
    new_event->poptime = time;
    new_event->eventtype = type;
    new_event->prev = NULL;
    new_event->next = NULL;
    printf("Create successful\n");
    return new_event;
}

/*  sorted_event_enqueue inserts events into the event queue according to their poptime
    If the time matches the lowest time of the system, that event becomes the new root.
    This means that newer events occur prior to older events of the same poptime.  Therefore
    the END event created at the start actually occurs last amongst all events at the end
    time.

    TODO: check if that's play with a TA

    Arguments:
    event_queue_root - a pointer to a pointer to the root node of the event queue

    new_event - a pointer to the event node to be inserted into the queue

    Return:
    Nothing
*/
void sorted_event_enqueue(struct event **event_queue_root, struct event *new_event)
{
    struct event *traverser = (*event_queue_root);

    if (new_event->poptime <= traverser->poptime)
    {
        (*event_queue_root)->prev = new_event;
        new_event->next = (*event_queue_root);
        (*event_queue_root) = new_event;
    }
    else 
    {        
        while ((new_event->poptime > traverser->poptime) && (traverser->next != NULL))
        {
            traverser = traverser->next;
        }
        if ((new_event->poptime > traverser->poptime) && (traverser->next == NULL))
        {
            traverser->next = new_event;
            new_event->prev = traverser;
        }
        else
        {
            new_event->next = traverser;
            traverser->prev->next = new_event;
            new_event->prev = traverser->prev;
            traverser->prev = new_event;
        }
    } 
}

/*  create_process handles the creation of new process nodes
    Contains the actual malloc command then initializes the id

    Arguments:
    id - a string containing the id of the new process

    Return:
    a pointer the the newly created process node
*/
struct process* create_process(char* id)
{
    struct process *new_process = (struct process*) malloc(sizeof(struct process));
    strcpy(new_process->processid, id); 
    return new_process;
}

/*  enqueue adds a process node to the end of a given queue

    Arguments:
    queue_tail - a pointer to a pointer to the tail node in a queue

    new_process - a pointer to the process node being enqueued

    Return:
    nothing
*/
void enqueue(struct process **queue_tail, struct process *new_process)
{
    if ((*queue_tail) != NULL)
    {
        (*queue_tail)->next = new_process;
        (*queue_tail) = new_process;
        new_process->next = NULL;
    }
    else
    {
        (*queue_tail = new_process);
        new_process->next = NULL;
    }
    
}

/*  dequeue removes the first node in a queue.
    Then moves the root to the next node back

    Arguments:
    queue_head - pointer to a pointer to the root process node of a queue

    Return:
    pointer to the removed process node
*/
struct process* dequeue(struct process **queue_head)
{
    struct process *popped_process = (*queue_head);
    (*queue_head) = (*queue_head)->next;
    popped_process->next = NULL;
    return popped_process;
}

/*  process_arrival handles the actions to be carried out for an arrival event

    TODO: Started this abstraction to clean up main, never finished.  FINISH

    Arguments:
    cpu_queue_tail - a pointer to a pointer to the tail node of the cpu queue

    id - string containing the process id

    Return:
    nothing
*/
void process_arrival(struct process **cpu_queue_tail, char* id)
{
    struct process *new_process = create_process(id);
    enqueue(cpu_queue_tail, new_process);
}

/*  TODO: Flesh out or remove
void cpu_is_empty(struct process **cpu_queue_head, struct process **disk_tail)
{

}

void cpu_finish(struct process **cpu_queue_head, struct process **disk_tail)
{
    struct process *finished_process = dequeue(cpu_queue_head);
    if (ranged_rand(0,100) < (QUIT_PROB))
        free(finished_process);
    else
    {
        enqueue(disk_tail, finished_process);
    }
}
*/

/*  printToOutput handles the log file output
    Initializes a string depending on the eventtype.
    Uses that string in unchanged fprintf

    Arguments:
    output - pointer to the output file object

    curent_event - pointer to the event to be printed

    Return:
    nothing
*/
void printToOutput(FILE *output, struct event *current_event)
{
    char printType[22];
    switch (current_event->eventtype)
    {
    case 0: strcpy(printType, "ENDING SIMULATION");
        break;
    case 1: strcpy(printType, "STARTING SIMULATION");
        break;
    case 2: strcpy(printType, "arriving at CPU queue");
        break;
    case 3: strcpy(printType, "entering CPU");
        break;
    case 4: strcpy(printType, "leaving CPU");
        break;
    case 5: strcpy(printType, "entering DISK1");
        break;
    case 6: strcpy(printType, "leaving DISK1");
        break;
    case 7: strcpy(printType, "entering DISK2");
        break;
    case 8: strcpy(printType, "leaving DISK2");
        break;
    case 9: strcpy(printType, "leaving simulation");
        break;
    }
    fprintf(output, "at time %d process %s is %s\n\n", current_event->poptime, current_event->eventid, printType);
}