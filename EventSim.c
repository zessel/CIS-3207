#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

struct process
{
    char processid[8];
    struct process *next;     
};

/*  Event types:
    0: Start simulation events
    1: New process in simulation
    2: Arrive at CPU queue
    3: Leave CPU
    4: Arrive at Disk 1
    5: Leave Disk 1
    6: Arrive at Disk 2
    7: Leave Disk 2
    9: End simulation events
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
struct event* create_event(char* id, int time, int type);
void sorted_event_enqueue(struct event **event_queue_root, struct event *new_event);
void enqueue(struct process **queue_tail, struct process *new_process);
struct process* dequeue(struct process **queue_head);
void process_arrival(struct process **cpu_queue_tail, char* id);
void cpu_finish(struct process **cpu_queue_head, struct process **disk_tail);



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

    struct event *start_event = (struct event*) malloc(sizeof(struct event));
    strcpy(start_event->eventid, "START");
    start_event->poptime = INIT_TIME;
    start_event->eventtype = 0;
    start_event->prev = NULL;

    struct event *end_event = (struct event*) malloc(sizeof(struct event));
    strcpy(end_event->eventid, "END");
    end_event->poptime = FIN_TIME;
    end_event->eventtype = 9;
    end_event->prev = start_event;
    start_event->next = end_event;

    event_queue_root = start_event;        

    int endhit = 0;
    char id_as_str[8];

    while (endhit != 1)
    {
        current_event = popevent(&event_queue_root);
        globaltime = current_event->poptime;
        if (strcmp(current_event->eventid, "END") == 0)
            endhit = 1;
        printf("Pop successful");

        switch(current_event->eventtype)
        {
        case 0: create_event(processcount, globaltime + ranged_rand(ARRIVE_MAX,ARRIVE_MIN), 1);
            processcount++;
            break;
        case 1: process_arrival(&cpu_queue_tail, current_event->eventid);
            if (cpu_queue_head == NULL)
            {
                cpu_queue_head = cpu_queue_tail;
                create_event(processcount, globaltime, 2);
            }
            break;
        case 2: create_event(cpu_queue_head->processid, globaltime + ranged_rand(CPU_MAX,CPU_MIN), 3);
            break;
        case 3: current_process = dequeue(&cpu_queue_head);
            create_event(cpu_queue_head->processid, globaltime, 2);
            if (ranged_rand(0,100) < (QUIT_PROB))
                free(current_process);
            else
            {
                if (disk1_queue_count >= disk2_queue_count)
                {
                    enqueue(&disk1_queue_tail, current_process);
                    if (disk1_queue_head == NULL)
                    {
                        disk1_queue_head = current_process;
                        create_event(current_process->processid, globaltime, 4);
                    }
                    disk1_queue_count++;
                }
                else
                {
                    enqueue(&disk2_queue_tail, current_process);
                    if (disk2_queue_head == NULL)
                    {
                        disk2_queue_head = current_process;
                        create_event(current_process->processid, globaltime, 6);
                    }
                    disk2_queue_count++;
                }
            }
            break;
        case 4: create_event(disk1_queue_head->processid, globaltime + ranged_rand(DISK1_MAX,DISK1_MIN), 5);
            break;
        case 5: current_process = dequeue(&disk1_queue_tail);
            create_event(disk1_queue_head->processid, globaltime, 4);
            enqueue(&cpu_queue_tail, current_process);
            if (cpu_queue_head == NULL)
            {
                cpu_queue_head = cpu_queue_tail;
                create_event(current_process->processid, globaltime, 2);
            }
        case 6: create_event(disk2_queue_head->processid, globaltime + ranged_rand(DISK2_MAX,DISK2_MIN), 7);
            break;
        case 7: current_process = dequeue(&disk2_queue_tail);
            create_event(disk2_queue_head->processid, globaltime, 6);
            enqueue(&cpu_queue_tail, current_process);
            if (cpu_queue_head == NULL)
            {
                cpu_queue_head = cpu_queue_tail;
                create_event(current_process->processid, globaltime, 2);
            }        
        default:
            break;
        }
        print_event(current_event);
        printf("Print successful\n");
 /*       sprintf(str, "%d", processcount);
        sorted_event_enqueue(&event_queue_root, create_event(str, 
            globaltime + ranged_rand(ARRIVE_MAX,ARRIVE_MIN), 1)); */
        free(current_event);
        printf("Free successful\n");

    }
    
}

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

/*  Exists to slightly clean up the code when a random is called  */

int ranged_rand(int max, int min)
{
    return (rand() % (max - min + 1) + min);
}

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

void print_event(struct event *current_event)
{
    printf("\nThe event id is %s", current_event->eventid);
    printf("\nThe Event time is %d\n", current_event->poptime);
}

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

void sorted_event_enqueue(struct event **event_queue_root, struct event *new_event)
{
    struct event *traverser = *event_queue_root;

    if (new_event->poptime < traverser->poptime)
    {
        (*event_queue_root)->prev = new_event;
        new_event->next = *event_queue_root;
        *event_queue_root = new_event;
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

struct process* create_process(char* id)
{
    struct process *new_process = (struct process*) malloc(sizeof(struct process));
    return new_process;
}

void enqueue(struct process **queue_tail, struct process *new_process)
{
    (*queue_tail)->next = new_process;
}

struct process* dequeue(struct process **queue_head)
{
    struct process *popped_process = (*queue_head);
    (*queue_head) = (*queue_head)->next;
    popped_process->next = NULL;
    return popped_process;
}

void process_arrival(struct process **cpu_queue_tail, char* id)
{
    struct process *new_process = create_process(id);
    enqueue(cpu_queue_tail, new_process);
}

void cpu_start(struct process **cpu_queue_head, struct process **disk_tail)
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

 