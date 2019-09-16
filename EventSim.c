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
#include <unistd.h>

/*  This is the structure for the processes that go between cpu and i/o
    the contain an processid stored as a string, because no mathematical
    operations are done using it.  They also contain a process pointer
    so that they can be queued up
*/

struct process
{
    char processid[8];
    int arrival_time;
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

int upped = 0;
int downed = 0;

void initialize_from_file();
int ranged_rand(int max, int min);
struct event* popevent(struct event **event_queue_root);
void print_event(struct event *current_event);
void print_created(struct event *current_event);
struct event* create_event(char* id, int time, int type);
void sorted_event_enqueue(struct event **event_queue_root, struct event *new_event);
struct process* create_process(char* id);
void enqueue(struct process **queue_tail, struct process *new_process);
struct process* dequeue(struct process **queue_head, struct process **queue_tail);
void process_arrival(struct process **cpu_queue_tail, char* id);
int is_empty(struct process *queue_head);
void printToOutput(FILE *output, struct event *current_event);
void calculate_idle(struct process *cpu_head, struct process *disk1_head, struct process *disk2_head, int time, int printout, FILE *output);
void statistics(int cpu_length, int d1_length, int d2_length, int event_length, int printbool, FILE *output);
void response_times (struct event *current_event, int arrival_time, int printbool, FILE *outputfile);
void throughput (int cpu, int d1, int d2, FILE *output);





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

    fprintf(output, "The simulation ran with the following values\n"
        "\nSeed: %30d\nInitial Time: %22d\nFinish Time: %23d\nArrival Window Minimum: %12d"
        "\nArrival Window Maximum: %12d\nQuit Probability (out of 100): %5d\nCPU Processing Minimum: %12d"
        "\nCPU Processing Maximum: %12d\nDISK1 Time Minimum: %16d\nDISK1 Time Maximum: %16d"
        "\nDISK2 Time Minimum: %16d\nDISK2 Time Maximum: %16d\n\n\n", SEED,INIT_TIME,FIN_TIME,ARRIVE_MIN,
        ARRIVE_MAX, QUIT_PROB, CPU_MIN, CPU_MAX, DISK1_MIN, DISK1_MAX, DISK2_MIN, DISK2_MAX);

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

    int cpu_length = 0;
    int d1_length = 0;
    int d2_length = 0;
    int event_length = 0;
    int completed_cpu = 0;
    int completed_d1 = 0;
    int completed_d2 = 0;
    int total_exited = 0;

    struct event *event_queue_root = NULL;
    struct event *current_event = NULL;
    struct event *new_event = NULL;



    struct event *start_event = (struct event*) malloc(sizeof(struct event));
    strcpy(start_event->eventid, "START");
    start_event->poptime = INIT_TIME;
    start_event->eventtype = 1;
    start_event->prev = NULL;
    event_length++;

    struct event *end_event = (struct event*) malloc(sizeof(struct event));
    strcpy(end_event->eventid, "END");
    end_event->poptime = FIN_TIME;
    end_event->eventtype = 0;
    end_event->prev = start_event;
    start_event->next = end_event;
    event_length++;

    event_queue_root = start_event;        

    int endhit = 0;
    char id_as_str[8];
    int d1_ran = 0;
    int d2_ran = 0;

    while (endhit != 1)
    {
        current_event = popevent(&event_queue_root);
        event_length--;
        statistics(cpu_length, d1_length, d2_length, event_length, 0, output);
        printToOutput(output, current_event);
        globaltime = current_event->poptime;
        sprintf(id_as_str, "%d", processcount);
        
        switch(current_event->eventtype)
        {
        case 1: new_event = create_event(id_as_str, globaltime + ranged_rand(ARRIVE_MAX,ARRIVE_MIN), 2);
            sorted_event_enqueue(&event_queue_root, new_event);
            event_length++;
            processcount++;
            break;
        case 2: current_process = create_process(current_event->eventid);
            current_process->arrival_time = globaltime;
            enqueue(&cpu_queue_tail, current_process);
            cpu_length++;
            if (is_empty(cpu_queue_head))
            {
                cpu_queue_head = cpu_queue_tail;
                new_event = create_event(current_event->eventid, globaltime, 3);
                sorted_event_enqueue(&event_queue_root, new_event);
                event_length++;
            }
            new_event = create_event(id_as_str, globaltime + ranged_rand(ARRIVE_MAX,ARRIVE_MIN), 2);
            sorted_event_enqueue(&event_queue_root, new_event);
            event_length++;
            processcount++;
            break;
        case 3: new_event = create_event(current_event->eventid, globaltime + ranged_rand(CPU_MAX, CPU_MIN), 4);
            sorted_event_enqueue(&event_queue_root, new_event);
            event_length++;
            break;
        case 4: current_process = dequeue(&cpu_queue_head, &cpu_queue_tail);
            cpu_length--;
            completed_cpu++;
            response_times (current_event, current_process->arrival_time, 0, output);

            if (!is_empty(cpu_queue_head))
            {
                new_event = create_event(cpu_queue_head->processid, globaltime, 3);
                sorted_event_enqueue(&event_queue_root, new_event);
                event_length++;
            }
            if (ranged_rand(100,0) < (QUIT_PROB))
            {
                new_event = create_event(current_process->processid, globaltime, 9);
                sorted_event_enqueue(&event_queue_root, new_event);
                event_length++;
            }
            else
            {
                if (d1_length <= d2_length)
                {
                    current_process->arrival_time = globaltime;
                    enqueue(&disk1_queue_tail, current_process);

                    if (is_empty(disk1_queue_head))
                    {
                        disk1_queue_head = disk1_queue_tail;                      
                        new_event = create_event(current_process->processid, globaltime, 5);
                        sorted_event_enqueue(&event_queue_root, new_event);
                        event_length++;
                    }
                    d1_length++;
                }
                else
                {
                    current_process->arrival_time = globaltime;
                    enqueue(&disk2_queue_tail, current_process);
                    if (is_empty(disk2_queue_head))
                    {
                        disk2_queue_head = disk2_queue_tail;
                        new_event = create_event(current_process->processid, globaltime, 7);
                        sorted_event_enqueue(&event_queue_root, new_event);
                        event_length++;
                    }
                    d2_length++;
                    upped++;
                }
            }
            break;
        case 5: new_event = create_event(disk1_queue_head->processid, globaltime + ranged_rand(DISK1_MAX,DISK1_MIN), 6);
            sorted_event_enqueue(&event_queue_root, new_event);
            event_length++;
            break;
        case 6: current_process = dequeue(&disk1_queue_head, &disk1_queue_tail);
            d1_length--;
            completed_d1++;
            response_times (current_event, current_process->arrival_time, 0, output);
            
            if (!is_empty(disk1_queue_head))
            {
                new_event = create_event(disk1_queue_head->processid, globaltime, 5);
                sorted_event_enqueue(&event_queue_root, new_event);
                event_length++;
            }
            current_process->arrival_time = globaltime;
            enqueue(&cpu_queue_tail, current_process);
            cpu_length++;

            if (is_empty(cpu_queue_head))
            {
                cpu_queue_head = cpu_queue_tail;
                new_event = create_event(current_event->eventid, globaltime, 3);
                sorted_event_enqueue(&event_queue_root, new_event);
                event_length++;
            }
            break;
        case 7: new_event = create_event(disk2_queue_head->processid, globaltime + ranged_rand(DISK2_MAX,DISK2_MIN), 8);
            sorted_event_enqueue(&event_queue_root, new_event);
            event_length++;
            break;
        case 8: current_process = dequeue(&disk2_queue_head, &disk2_queue_tail);
            d2_length--;
            completed_d2++;
            response_times (current_event, current_process->arrival_time, 0, output);
            if (!is_empty(disk2_queue_head))
            {
                new_event = create_event(disk2_queue_head->processid, globaltime, 7);
                sorted_event_enqueue(&event_queue_root, new_event);
                event_length++;
            }
            current_process->arrival_time = globaltime;
            enqueue(&cpu_queue_tail, current_process);
            cpu_length++;

            if (is_empty(cpu_queue_head))
            {
                cpu_queue_head = cpu_queue_tail;
                new_event = create_event(current_event->eventid, globaltime, 3);
                sorted_event_enqueue(&event_queue_root, new_event);
                event_length++;
            }
            break;
        case 9: free(current_process);
            total_exited++;
            break;
        case 0: endhit = 1;       
            break;
        }

        calculate_idle(cpu_queue_head, disk1_queue_head, disk2_queue_head, globaltime, 0, output);
        free(current_event);
    }
    calculate_idle(cpu_queue_head, disk1_queue_head, disk2_queue_head, globaltime, 1, output);
    response_times (current_event, 0, 1, output);
    statistics(cpu_length, d1_length, d2_length, event_length, 1, output);
    throughput (completed_cpu, completed_d1, completed_d2, output);
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
    new_process->arrival_time = 0; 
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
struct process* dequeue(struct process **queue_head, struct process **queue_tail)
{
    struct process *popped_process = (*queue_head);
    (*queue_head) = (*queue_head)->next;
    popped_process->next = NULL;
    if ((*queue_head) == NULL)
        (*queue_tail) = NULL;
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

/*  is_empty checks if a queue_head pointer is empty returns 1 if true

    Arguments:
    queue_head - a pointer to the head of a process queue

    Return:
    int of 0 (false) or 1 (true)
*/
int is_empty(struct process *queue_head)
{
    int empty = 0;
    if (queue_head == NULL)
        empty = 1;
    return empty;
}
/*  TODO: Flesh out or remove
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

/*  calculate_idle contains the variables and actions for determining utilization
    the function has an argument parameter to print the results, but unfortunately
    needs an output ile whether or not printing is taking place.
    Uses statics to keep track of the "time" that any given queue was empty,
    then it calculates how much time passed since then.

    Prints both the number of cycles of inactivity for any given queue
    along with the percentage of utilization out of the total run time

    Arguments:
    cpu_head - pointer to a process struc containing the head of the cpu queue

    disk1_head - pointer to a process struc containing the head of the disk1 queue

    disk2_head - pointer to a process struc containing the head of the disk2 queue

    printout - bool for whether the function prints to the output file

    output - the output file for printing

    Returns:
    Nothing 
*/
void calculate_idle(struct process *cpu_head, struct process *disk1_head, struct process *disk2_head, int time, int printout, FILE *output)
{
    static int lastFilledCPU = 0;
    static int lastFilledD1 = 0;
    static int lastFilledD2 = 0;

    static int totalCPU = 0;
    static int totalD1 = 0;
    static int totalD2 = 0;

    static int CPUWasEmpty = 0;
    static int D1WasEmpty = 0;
    static int D2WasEmpty = 0;
    int runtime = FIN_TIME-INIT_TIME;

    if ((is_empty(cpu_head)) && (lastFilledCPU <= time))
    {
        lastFilledCPU = time;
        CPUWasEmpty = 1;
    }
    else
    {
        if (CPUWasEmpty)
            totalCPU = totalCPU + time - lastFilledCPU;
        lastFilledCPU = time;
        CPUWasEmpty = 0;
    } 
    if ((is_empty(disk1_head)) && (lastFilledD1 <= time))
    {
        lastFilledD1 = time;
        D1WasEmpty = 1;
    }
    else
    {
        if (D1WasEmpty)
            totalD1 = totalD1 + time - lastFilledD1;
        lastFilledD1 = time;
        D1WasEmpty = 0;
    }
    if ((is_empty(disk2_head)) && (lastFilledD2 <= time))
    {
        lastFilledD2 = time;
        D2WasEmpty = 1;
    }
    else
    {
        if (D2WasEmpty)
            totalD2 = totalD2 + time - lastFilledD2;
        lastFilledD2 = time;
        D2WasEmpty = 0;
    }
    if (printout)
        fprintf(output, "\nUnused cycles:\n\tCPU:   %d\n\tDISK1: %d\n\tDISK2: %d\n"
                        "\nUtilization:\n\tCPU:   %f%%\n\tDISK1: %f%%\n\tDISK2: %f%%\n",
                        totalCPU, totalD1, totalD2,
                        (float)(runtime-totalCPU)*100/runtime,
                        (float)(runtime-totalD1)*100/runtime,
                        (float)(runtime-totalD2)*100/runtime);

}

/*  statistics provides the data and calculations pertaining to queue lengths
    The function runs every event so the average is the length of the queues / the total number
    of events.  Remember that multiple events can run at the same time slice.

    The average is only calculated after you decide to print, in other cases it simply tallies
    the average prior to dividing by n runs.

    Arguments:
    cpu_length - the current length of the cpu queue
    
    d1_length - the current length of the disk1 queue

    d2_length - the current length of the disk2 queue

    event_length - the current length of the event queue

    printbool - boolean for whether output file printing is needed

    output - output file

    Returns:
    Nothing
*/
void statistics(int cpu_length, int d1_length, int d2_length, int event_length, int printbool, FILE *output)
{
    static int cpu_queue_max = 0;
    static int d1_queue_max = 0;
    static int d2_queue_max = 0;
    static int event_queue_max = 0;

    static long cpu_average = 0;
    static long d1_average =0;
    static long d2_average = 0;
    static long event_average =0;

    static int stat_calls = 1;

    if (printbool)
    {
        fprintf(output, "\nMaximum Queue Lengths:\n\tCPU:   %d\n\tDISK1: %d\n\tDISK2: %d\n\tEvent: %d\n",
            cpu_queue_max,
            d1_queue_max,
            d2_queue_max,
            event_queue_max);
        fprintf(output, "\nAverage Queue Lengths:\n\tCPU:   %f\n\tDISK1: %f\n\tDISK2: %f\n\tEvent: %f\n",
            (float) cpu_average / stat_calls,
            (float) d1_average / stat_calls,
            (float) d2_average / stat_calls,
            (float) event_average / stat_calls);
    }
    else
    {    
        if (cpu_length > cpu_queue_max)
            cpu_queue_max = cpu_length;
        if (d1_length > d1_queue_max)
            d1_queue_max = d1_length;
        if (d2_length > d2_queue_max)
            d2_queue_max = d2_length;
        if (event_length > event_queue_max)
            event_queue_max = event_length;
            
        cpu_average += cpu_length;
        d1_average += d1_length;
        d2_average += d2_length;
        event_average += event_length;

        stat_calls++;
    }
}

/*  response_times contains the data and math for determining average and maximum response time
    Calculates the response times for the three queue systems.

    Arguments:
    current_event - a pointer to am event struct containing the event driving this loop

    arrival_time - the time value of the process the current_event is dequeueing

    printbool - a boolean for determining whether to print to outputfile

    outputfile - the file to print output to

    Returns:
    Nothing
*/
void response_times (struct event *current_event, int arrival_time, int printbool, FILE *outputfile)
{
    static long avg_cpu_response = 0;
    static long avg_d1_response = 0;
    static long avg_d2_response = 0;
    static int cpu_total = 0;
    static int d1_total = 0;
    static int d2_total = 0;
    static int largest_cpu = 0;
    static int largest_d1 = 0;
    static int largest_d2 = 0;

    if (printbool)
    {
        fprintf(outputfile, "\nAverage Reponse Times:\n\tCPU:   %f\n\tDISK1: %f\n\tDISK2: %f\n",
            (float)avg_cpu_response/cpu_total,
            (float)avg_d1_response/d1_total,
            (float)avg_d2_response/d2_total);
        fprintf(outputfile, "\nLongest Reponse Times:\n\tCPU:   %d\n\tDISK1: %d\n\tDISK2: %d\n",
            largest_cpu,
            largest_d1,
            largest_d2);
    }
    else
    {
        if ((current_event->eventtype) == 4)
        {
            if ((current_event->poptime - arrival_time) > largest_cpu)
                largest_cpu = current_event->poptime - arrival_time;
            avg_cpu_response += (current_event->poptime - arrival_time);
            cpu_total++;
        }
        if ((current_event->eventtype) == 6)
        {
            if ((current_event->poptime - arrival_time) > largest_d1)
                largest_d1 = current_event->poptime - arrival_time;
            avg_d1_response += (current_event->poptime - arrival_time);
            d1_total++;
        }
        if ((current_event->eventtype) == 8)
        {
            if ((current_event->poptime - arrival_time) > largest_d2)
                largest_d2 = current_event->poptime - arrival_time;
            avg_d2_response += (current_event->poptime - arrival_time);
            d2_total++;
        }
    }
}

/*  throughput prints the jobs completed for each system / total simulation time

    Arguments:
    cpu - the total number of processes that exited cpu queue

    d1 - the total number of processes that exited disk 1 queue

    d2 - the total number of processese that exited disk 2 queue

    Returns:
    Nothing
*/
void throughput (int cpu, int d1, int d2, FILE *output)
{
    fprintf(output, "\nThroughput of System:\n\tCPU:   %f\n\tDISK1: %f\n\tDISK2: %f\n",
        (float)cpu / (FIN_TIME - INIT_TIME),
        (float)d1 / (FIN_TIME - INIT_TIME),
        (float)d2 / (FIN_TIME - INIT_TIME));
}