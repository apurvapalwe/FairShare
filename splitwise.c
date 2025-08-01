#include "splitwise.h"

// Linked List Functions
TransactionNode *create_transaction_node(const char *from, const char *to, double amount)
{
    TransactionNode *new_node = malloc(sizeof(TransactionNode));
    if (!new_node)
    {
        fprintf(stderr, "Memory allocation failed for transaction node\n");
        return NULL;
    }
    strcpy(new_node->from, from);
    strcpy(new_node->to, to);
    new_node->amount = amount;
    new_node->next = NULL;
    return new_node;
}

void add_transaction_to_history(TransactionNode **head, const char *from, const char *to, double amount)
{
    TransactionNode *new_node = create_transaction_node(from, to, amount);
    if (!new_node)
        return;

    new_node->next = *head;
    *head = new_node;
}

void print_transaction_history(TransactionNode *head)
{
    printf("Transaction History:\n");
    printf("+-----------------+-----------------+-----------+\n");
    printf("| %-15s | %-15s | %-9s |\n", "From", "To", "Amount");
    printf("+-----------------+-----------------+-----------+ \n");
    if(!head){
        printf("                  No Transactions yet!\n");
    }
    while (head)
    {
        printf("| %-15s | %-15s | $%8.2f |\n", head->from, head->to, head->amount);
        head = head->next;
    }
    printf("+-----------------+-----------------+-----------+\n");
}

// print transaction of particular person



// Graph Functions
GraphNode *create_graph_node(const char *name)
{
    GraphNode *new_node = malloc(sizeof(GraphNode));
    if (!new_node)
    {
        fprintf(stderr, "Memory allocation failed for graph node\n");
        return NULL;
    }
    strcpy(new_node->name, name);
    new_node->balance = 0.0;
    new_node->next = NULL;
    new_node->edges = NULL;
    return new_node;
}

void add_graph_node(GraphNode **head, const char *name)
{
    // Check if node already exists
    if (find_graph_node(*head, name)){
        printf("user already exists");
        return;
    }
    GraphNode *new_node = create_graph_node(name);
    if (!new_node)
        return;

    new_node->next = *head;
    *head = new_node;
}

GraphNode *find_graph_node(GraphNode *head, const char *name)
{
    while (head)
    {
        if (strcmp(head->name, name) == 0)
            return head;
        head = head->next;
    }
    return NULL;
}

void add_edge(GraphNode *from, GraphNode *to, double amount)
{
    if (!from || !to)
        return;

    // Check if the edge already exists
    EdgeNode *current = from->edges;
    while (current)
    {
        if (current->dest == to) // Edge exists, update the amount
        {
            current->amount += amount;
            from->balance += amount;
            to->balance -= amount;
            return;
        }
        current = current->next;
    }

    // If edge doesn't exist, create a new one
    EdgeNode *new_edge = malloc(sizeof(EdgeNode));
    if (!new_edge)
    {
        fprintf(stderr, "Memory allocation failed for edge\n");
        return;
    }

    new_edge->dest = to;
    new_edge->amount = amount;
    new_edge->next = from->edges;
    from->edges = new_edge;

    // Update balances
    from->balance += amount;
    to->balance -= amount;
}

void print_graph(GraphNode *head)
{
    printf("Graph Balances:\n");
    printf("+-----------------+------------+\n");
    printf("| %-15s | %-10s |\n", "User", "Balance");
    printf("+-----------------+------------+\n");
    while (head)
    {
        printf("| %-15s | $%-9.2f |\n", head->name, head->balance);
        head = head->next;
    }
    printf("+-----------------+------------+\n");
}

// Heap Functions
void swap_max_heap_nodes(MaxHeapNode *a, MaxHeapNode *b)
{
    MaxHeapNode temp = *a;
    *a = *b;
    *b = temp;
}

void swap_min_heap_nodes(MinHeapNode *a, MinHeapNode *b)
{
    MinHeapNode temp = *a;
    *a = *b;
    *b = temp;
}

void max_heapify(MaxHeapNode *heap, int size, int index)
{
    int largest = index;
    int left = 2 * index + 1;
    int right = 2 * index + 2;

    if (left < size && heap[left].amount > heap[largest].amount)
        largest = left;

    if (right < size && heap[right].amount > heap[largest].amount)
        largest = right;

    if (largest != index)
    {
        swap_max_heap_nodes(&heap[index], &heap[largest]);
        max_heapify(heap, size, largest);
    }
}

void min_heapify(MinHeapNode *heap, int size, int index)
{
    int smallest = index;
    int left = 2 * index + 1;
    int right = 2 * index + 2;

    if (left < size && heap[left].amount < heap[smallest].amount)
        smallest = left;

    if (right < size && heap[right].amount < heap[smallest].amount)
        smallest = right;

    if (smallest != index)
    {
        swap_min_heap_nodes(&heap[index], &heap[smallest]);
        min_heapify(heap, size, smallest);
    }
}

void insert_max_heap(Group *group, const char *name, double amount)
{
    if (group->max_heap_size >= MAX_USERS)
    {
        fprintf(stderr, "Max heap is full\n");
        return;
    }

    strcpy(group->max_heap[group->max_heap_size].name, name);
    group->max_heap[group->max_heap_size].amount = amount;

    int current = group->max_heap_size;
    group->max_heap_size++;

    while (current > 0 &&
           group->max_heap[(current - 1) / 2].amount < group->max_heap[current].amount)
    {
        swap_max_heap_nodes(&group->max_heap[current], &group->max_heap[(current - 1) / 2]);
        current = (current - 1) / 2;
    }
}

void insert_min_heap(Group *group, const char *name, double amount)
{
    if (group->min_heap_size >= MAX_USERS)
    {
        fprintf(stderr, "Min heap is full\n");
        return;
    }

    strcpy(group->min_heap[group->min_heap_size].name, name);
    group->min_heap[group->min_heap_size].amount = amount;

    int current = group->min_heap_size;
    group->min_heap_size++;

    while (current > 0 &&
           group->min_heap[(current - 1) / 2].amount > group->min_heap[current].amount)
    {
        swap_min_heap_nodes(&group->min_heap[current], &group->min_heap[(current - 1) / 2]);
        current = (current - 1) / 2;
    }
}

// Group Functions
Group *create_group(const char *name)
{
    Group *group = malloc(sizeof(Group));
    if (!group)
    {
        fprintf(stderr, "Memory allocation failed for group\n");
        return NULL;
    }

    strcpy(group->name, name);
    group->users = NULL;
    group->transaction_history = NULL;

    group->max_heap = malloc(MAX_USERS * sizeof(MaxHeapNode));
    group->min_heap = malloc(MAX_USERS * sizeof(MinHeapNode));
    group->max_heap_size = 0;
    group->min_heap_size = 0;

    return group;
}

void add_user_to_group(Group *group, const char *name)
{
    add_graph_node(&group->users, name);
}

void add_transaction(Group *group, const char *from, const char *to, double amount)
{
    GraphNode *from_node = find_graph_node(group->users, from);
    GraphNode *to_node = find_graph_node(group->users, to);

    if (!from_node || !to_node)
    {
        fprintf(stderr, "Users not found in the group\n");
        return;
    }

    // Add transaction to graph
    add_edge(from_node, to_node, amount);

    // Add to transaction history
    add_transaction_to_history(&group->transaction_history, from, to, amount);
}

//minimize Cash flow
void settle_group(Group *group)
{
    GraphNode *current = group->users;

    // Clear previous heaps
    group->max_heap_size = 0;
    group->min_heap_size = 0;

    // Populate heaps based on balances
    while (current)
    {
        if (current->balance > 0)
        {
            insert_max_heap(group, current->name, current->balance);
        }
        else if (current->balance < 0)
        {
            insert_min_heap(group, current->name, fabs(current->balance));
        }
        current = current->next;
    }

    printf("Settlement Transactions:\n");
    while (group->max_heap_size > 0 && group->min_heap_size > 0)
    {
        MaxHeapNode creditor = group->max_heap[0];
        MinHeapNode debtor = group->min_heap[0];

        double settle_amount = fmin(creditor.amount, debtor.amount);
        printf("| %-15s | %-15s | %8.2f |\n", debtor.name, creditor.name, settle_amount);

        // Update heaps
        group->max_heap[0].amount -= settle_amount;
        group->min_heap[0].amount -= settle_amount;

        // Re-heapify
        max_heapify(group->max_heap, group->max_heap_size, 0);
        min_heapify(group->min_heap, group->min_heap_size, 0);

        // Remove zero balance nodes
        if (group->max_heap[0].amount == 0)
        {
            group->max_heap_size--;
            group->max_heap[0] = group->max_heap[group->max_heap_size];
            max_heapify(group->max_heap, group->max_heap_size, 0);
        }
        if (group->min_heap[0].amount == 0)
        {
            group->min_heap_size--;
            group->min_heap[0] = group->min_heap[group->min_heap_size];
            min_heapify(group->min_heap, group->min_heap_size, 0);
        }
    }
}

void print_group_balance(Group *group)
{
    printf("\n=== Group Balance Report: %s ===\n", group->name);
    print_graph(group->users);
    print_transaction_history(group->transaction_history);
}

void free_group(Group *group)
{
    // Free graph nodes
    GraphNode *current_user = group->users;
    while (current_user)
    {
        GraphNode *temp_user = current_user;

        // Free edges
        EdgeNode *current_edge = current_user->edges;
        while (current_edge)
        {
            EdgeNode *temp_edge = current_edge;
            current_edge = current_edge->next;
            free(temp_edge);
        }

        current_user = current_user->next;
        free(temp_user);
    }

    // Free transaction history
    TransactionNode *current_trans = group->transaction_history;
    while (current_trans)
    {
        TransactionNode *temp_trans = current_trans;
        current_trans = current_trans->next;
        free(temp_trans);
    }

    // Free heaps
    free(group->max_heap);
    free(group->min_heap);

    // Free group
    free(group);
}