#ifndef SPLITWISE_H
#define SPLITWISE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>

#define MAX_NAME_LENGTH 50
#define MAX_USERS 100
#define MAX_GROUPS 50

// Linked List Node for Transaction History
typedef struct TransactionNode
{
    char from[MAX_NAME_LENGTH];
    char to[MAX_NAME_LENGTH];
    double amount;
    struct TransactionNode *next;
} TransactionNode;

// Graph Node for Tracking Balances
typedef struct GraphNode
{
    char name[MAX_NAME_LENGTH];
    double balance;
    struct GraphNode *next;
    struct EdgeNode *edges;
} GraphNode;

// Edge Node for Graph
typedef struct EdgeNode
{
    GraphNode *dest;
    double amount;
    struct EdgeNode *next;
} EdgeNode;

// Max Heap Node for Settlement
typedef struct
{
    char name[MAX_NAME_LENGTH];
    double amount;
} MaxHeapNode;

// Min Heap Node for Settlement
typedef struct
{
    char name[MAX_NAME_LENGTH];
    double amount;
} MinHeapNode;

// Group Structure
typedef struct
{
    char name[MAX_NAME_LENGTH]; // group Name
    GraphNode *users;
    TransactionNode *transaction_history;
    MaxHeapNode *max_heap;
    MinHeapNode *min_heap;
    int max_heap_size;
    int min_heap_size;
} Group;

// Function Prototypes
// Linked List Functions
TransactionNode *create_transaction_node(const char *from, const char *to, double amount);
void add_transaction_to_history(TransactionNode **head, const char *from, const char *to, double amount);
void print_transaction_history(TransactionNode *head);

// Graph Functions
GraphNode *create_graph_node(const char *name);
void add_graph_node(GraphNode **head, const char *name);
GraphNode *find_graph_node(GraphNode *head, const char *name);
void add_edge(GraphNode *from, GraphNode *to, double amount);
void print_graph(GraphNode *head);

// Heap Functions
void swap_max_heap_nodes(MaxHeapNode *a, MaxHeapNode *b);
void swap_min_heap_nodes(MinHeapNode *a, MinHeapNode *b);
void max_heapify(MaxHeapNode *heap, int size, int index);            // O(log n)
void min_heapify(MinHeapNode *heap, int size, int index);            // O(log n)
void insert_max_heap(Group *group, const char *name, double amount); // O(log n)
void insert_min_heap(Group *group, const char *name, double amount); // O(log n)

// Group Functions
Group *create_group(const char *name);
void add_user_to_group(Group *group, const char *name);                              // O(n)
void add_transaction(Group *group, const char *from, const char *to, double amount); // O(e+n)
void settle_group(Group *group);                                                     // O(nlogn)
void print_group_balance(Group *group);
void free_group(Group *group);

#endif // SPLITWISE_H