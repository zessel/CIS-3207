#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int PRIORITY_QUEUE_SIZE = 100;
int CPU_QUEUE_SIZE = 25;
int DISK1_QUEUE_SIZE = 25;
int DISK2_QUEUE_SIZE = 25;

struct process
{
    char processid[8];
    struct process *next;     
};

struct event
{
    char eventid[8];
    int finishtime;
    struct event *prev;
    struct event *next;
};

struct event* popevent(struct event *event_queue_root);
void print_event(struct event *current_event);
struct event* create_event();
void sorted_event_enqueue(struct event *event_queue_root, struct event *new_event);



void main ()
{
    char inputfilename[] = "initializers.dat";
    char outputfilename[] = "eventoutput.dat";
    FILE *input;
    FILE *output;
    input = fopen(inputfilename, "r");
    if (input == NULL)
    {
        printf("The file %s was not found", inputfilename);
    }
    output = fopen(outputfilename, "w");

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

    char emptystring[20];
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

    printf("\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n", SEED,INIT_TIME,FIN_TIME,ARRIVE_MIN,
        ARRIVE_MAX,
        QUIT_PROB,
        CPU_MIN,
        CPU_MAX,
        DISK1_MIN,
        DISK1_MAX,
        DISK2_MIN,
        DISK2_MAX);

    int time = INIT_TIME;
    int eventcount = 1;

    struct process *cpu_queue_head = NULL;
    struct process *cpu_queue_tail = NULL;
    struct process *disc1_queue_head = NULL;
    struct process *disc1_queue_tail = NULL;
    struct process *disc2_queue_head = NULL;
    struct process *disc2_queue_tail = NULL;

    struct event *event_queue_root = NULL;
    struct event *current_event = NULL;

    struct event *start_event = (struct event*) malloc(sizeof(struct event));
    strcpy(start_event->eventid, "START");
    start_event->finishtime = INIT_TIME;
    start_event->prev = NULL;

    struct event *end_event = (struct event*) malloc(sizeof(struct event));
    strcpy(end_event->eventid, "END");
    end_event->finishtime = FIN_TIME;
    end_event->prev = start_event;
    start_event->next = end_event;

    event_queue_root = start_event;

    while (event_queue_root->next != NULL)
    {
        current_event = popevent(event_queue_root);
        printf("\n\nTHIS SHOULD SAY END: ~%s~\n\n", event_queue_root->eventid);
        printf("Pop successful");
        print_event(current_event);
        print_event(event_queue_root);
        printf("Print successful\n");
        if (eventcount == 1)
        {
            sorted_event_enqueue(event_queue_root, create_event());
            printf("Enqueue successful\n");
            }
        ++eventcount;
        free(current_event);
        printf("Free successful");
    }
    
}

struct event* popevent(struct event *event_queue_root)
{
    struct event *old_root = event_queue_root;
    event_queue_root = event_queue_root->next;
    old_root->next = NULL;
    event_queue_root->prev = NULL;
    printf("\n\nTHIS SHOULD SAY END: ~%s~\n\n", event_queue_root->eventid);
    return old_root; 
}

void print_event(struct event *current_event)
{
    printf("\nThe event id is %s", current_event->eventid);
    printf("\nThe Event time is %d\n", current_event->finishtime);
}

struct event* create_event()
{
    struct event *new_event = (struct event*) malloc(sizeof(struct event));
    strcpy(new_event->eventid, "one");
    new_event->finishtime = 40;
    new_event->prev = NULL;
    new_event->next = NULL;
    printf("Create successful\n");
    return new_event;
}

void sorted_event_enqueue(struct event *event_queue_root, struct event *new_event)
{/*
printf("launched");
    struct event *traverser = event_queue_root;
printf ("~~~here~~~~");
    if (new_event->finishtime < traverser->finishtime)
    {
        printf("1111\n");
        event_queue_root->prev = new_event;
        new_event->next = event_queue_root;
        event_queue_root = new_event;
    }
    else 
    {        
        printf("22222");
        while ((new_event->finishtime > traverser->finishtime) && (traverser->next != NULL))
        {
            traverser = traverser->next;
        }
        if ((new_event->finishtime > traverser->finishtime) && (traverser->next != NULL))
        {
            traverser->next = new_event;
            new_event->prev = traverser;
        }
        else
        {
        printf("3333"); 
            new_event->next = traverser;
            traverser->prev->next = new_event;
            new_event->prev = traverser->prev;
            traverser->prev = new_event;
        }
    }*/ 
}