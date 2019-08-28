#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
Any process that has not exited the system should be in one of the four queues: One of the FIFO queues if 
it is waiting to execute, or the event queue if it is currently executing.
*/

struct node
{
    char eventid[4];
    int finishtime;
    int start_end;
    int cpu_disc; 
    struct node *prev;
    struct node *next;     
};

void main ()
{
    
    struct node *head;
    head = (struct node*) malloc(sizeof(struct node));
    strcpy (head->eventid, "1");
    head->prev = NULL;
    head->next = NULL;
    struct node *tail;
    tail = head;

}

void enqueue (struct node* tail, struct node* popped_node)
{
    tail->next = popped_node;
    tail = tail->next;
}



    struct node *node1, *node2;

    node1 = (struct node*) malloc(sizeof(struct node));
    node2 = (struct node*) malloc(sizeof(struct node));
    
    strcpy (node1->eventid, "One");
    strcpy (node2->eventid, "Two");

    printf("%s\n", node1->eventid);
    printf("%s\n", node2->eventid);

    node1->next = node2;

    struct node *visitor;

    visitor = node1;
    visitor = visitor->next;
    printf("%s\n", visitor->eventid);


    free(node1);
    free(node2);