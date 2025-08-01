#include "splitwise.h"
#include <ctype.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#ifdef _WIN32
#include <direct.h>
#define MKDIR(dir) _mkdir(dir)
#else
#include <unistd.h>
#define MKDIR(dir) mkdir(dir, 0755)
#endif

#define GROUPS_FILE "fairshare_groups.txt"
#define TRANSACTIONS_DIR "transactions/"

#define MAX_INPUT_LEN 256
#define MAX_RECEIVERS 10

// Ensure transactions directory exists
void ensure_transactions_dir()
{
#ifdef _WIN32
    _mkdir(TRANSACTIONS_DIR);
#else
    mkdir(TRANSACTIONS_DIR, 0755);
#endif
}

int save_group_edges(Group *group)
{
    char filename[256];
    snprintf(filename, sizeof(filename), "transactions/%s_edges.txt", group->name);

    FILE *file = fopen(filename, "w");
    if (!file)
    {
        fprintf(stderr, "Error opening edges file %s: %s\n",
                filename, strerror(errno));
        return 0;
    }

    // Iterate through all users to find their edges
    GraphNode *current_node = group->users;
    while (current_node)
    {
        // Count edges for this node
        int edge_count = 0;
        EdgeNode *current_edge = current_node->edges;
        while (current_edge)
        {
            edge_count++;
            current_edge = current_edge->next;
        }

        // Write node name and edge count
        fprintf(file, "%s,%d\n", current_node->name, edge_count);

        // Reset and write edges
        current_edge = current_node->edges;
        while (current_edge)
        {
            // Write destination node name and amount
            fprintf(file, "%s,%.2f\n", current_edge->dest->name, current_edge->amount);
            current_edge = current_edge->next;
        }

        current_node = current_node->next;
    }

    fclose(file);
    return 1;
}

// Load edges for a group
void load_group_edges(Group *group)
{
    char filename[256];
    snprintf(filename, sizeof(filename), "transactions/%s_edges.txt", group->name);

    FILE *file = fopen(filename, "r");
    if (!file)
    {
        // No existing edges file, not an error
        return;
    }

    char node_name[MAX_NAME_LENGTH];
    char dest_name[MAX_NAME_LENGTH];
    int edge_count;
    double amount;

    // Read each node's edges
    while (fscanf(file, "%49[^,],%d\n", node_name, &edge_count) == 2)
    {
        // Find the source node
        GraphNode *source_node = find_graph_node(group->users, node_name);
        if (!source_node)
            continue;

        // Read specified number of edges for this node
        for (int i = 0; i < edge_count; i++)
        {
            if (fscanf(file, "%49[^,],%lf\n", dest_name, &amount) == 2)
            {
                // Find destination node
                GraphNode *dest_node = find_graph_node(group->users, dest_name);
                if (dest_node)
                {
                    // Add edge
                    add_edge(source_node, dest_node, amount);
                }
            }
        }
    }

    fclose(file);
}

// Save individual group's transaction history
int save_group_transactions(Group *group)
{
    // Create filename based on group name
    char filename[256];
    snprintf(filename, sizeof(filename), "%s%s_transactions.txt",
             TRANSACTIONS_DIR, group->name);

    FILE *file = fopen(filename, "w");
    if (!file)
    {
        fprintf(stderr, "Error opening transaction file %s: %s\n",
                filename, strerror(errno));
        return 0;
    }

    // Write transaction history
    TransactionNode *current = group->transaction_history;
    while (current)
    {
        fprintf(file, "%s,%s,%.2f\n",
                current->from, current->to, current->amount);
        current = current->next;
    }

    fclose(file);
    return 1;
}

// Load group's transaction history
void load_group_transactions(Group *group)
{
    char filename[256];
    snprintf(filename, sizeof(filename), "%s%s_transactions.txt",
             TRANSACTIONS_DIR, group->name);
    // transactions/arai_transactions.txt
    FILE *file = fopen(filename, "r");
    if (!file)
    {
        // No existing transaction history, not an error
        return;
    }

    char from[MAX_NAME_LENGTH], to[MAX_NAME_LENGTH];
    double amount;

    // Clear existing transaction history
    while (group->transaction_history)
    {
        TransactionNode *temp = group->transaction_history;
        group->transaction_history = temp->next;
        free(temp);
    }

    // Read and reconstruct transaction history
    while (fscanf(file, "%49[^,],%49[^,],%lf\n", from, to, &amount) == 3)
    {
        add_transaction_to_history(&group->transaction_history, from, to, amount);
    }

    fclose(file);
}

// Save all groups data
int save_groups(Group **groups, int group_count)
{
    ensure_transactions_dir();

    FILE *file = fopen(GROUPS_FILE, "w");
    if (!file)
    {
        fprintf(stderr, "Error opening groups file: %s\n", strerror(errno));
        return 0;
    }

    // Write group count
    fprintf(file, "%d\n", group_count);

    // Save each group
    for (int i = 0; i < group_count; i++)
    {
        Group *group = groups[i];

        // Write group name
        fprintf(file, "%s\n", group->name);

        // Write users
        GraphNode *user = group->users;
        int user_count = 0;

        // First count users
        while (user)
        {
            user_count++;
            user = user->next;
        }
        fprintf(file, "%d\n", user_count);

        // Now write users
        user = group->users;
        while (user)
        {
            fprintf(file, "%s,%.2f\n", user->name, user->balance);
            user = user->next;
        }

        // Save transactions
        save_group_transactions(group);
        save_group_edges(group);
    }

    fclose(file);
    return 1;
}

// Load groups from file
int load_groups(Group **groups)
{
    FILE *file = fopen(GROUPS_FILE, "r");
    if (!file)
    {
        // No existing groups file, not an error
        return 0;
    }

    int group_count;
    if (fscanf(file, "%d\n", &group_count) != 1)
    {
        fclose(file);
        return 0;
    }

    // Load each group
    for (int i = 0; i < group_count; i++)
    {
        char group_name[MAX_NAME_LENGTH];
        if (fgets(group_name, sizeof(group_name), file) == NULL)
        {
            break;
        }
        group_name[strcspn(group_name, "\n")] = 0; // Remove newline
        groups[i] = create_group(group_name);

        // Read user count
        int user_count;
        if (fscanf(file, "%d\n", &user_count) != 1)
        {
            break;
        }

        // Read and add users
        for (int j = 0; j < user_count; j++)
        {
            char user_line[MAX_INPUT_LEN];
            if (fgets(user_line, sizeof(user_line), file) == NULL)
            {
                break;
            }

            char name[MAX_NAME_LENGTH];
            double balance;
            if (sscanf(user_line, "%49[^,],%lf\n", name, &balance) != 2)
            {
                break;
            }

            // Add user to group
            add_user_to_group(groups[i], name);

            // Find and update balance
            GraphNode *user = find_graph_node(groups[i]->users, name);
            if (user)
            {
                user->balance = 0;
            }
        }

        // Load group's transaction history
        load_group_transactions(groups[i]);
        load_group_edges(groups[i]);
    }

    fclose(file);
    return group_count;
}

// Cross-platform screen clear function
void clear_screen()
{
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

void clear_input_buffer()
{
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

void print_menu()
{
    printf("\n--- Splitwise CLI ---\n");
    printf("1. Create a new group\n");
    printf("2. Add user to group\n");
    printf("3. Add group transaction\n");
    printf("4. Settle group debt\n");
    printf("5. View group balance\n");
    printf("6. Clear screen\n");
    printf("7. Print User Transactions\n");
    printf("8. Remove User from Group\n");
    printf("9. Remove Group\n");
    printf("0. Exit\n");
    printf("Enter your choice: ");
}

Group *select_or_create_group(Group **groups, int *group_count)
{
    if (*group_count == 0)
    {
        printf("No groups exist. Create a new group first.\n");

        char group_name[MAX_NAME_LENGTH];
        printf("Enter group name: ");
        fgets(group_name, sizeof(group_name), stdin);
        group_name[strcspn(group_name, "\n")] = 0;

        groups[*group_count] = create_group(group_name);
        return groups[(*group_count)++];
    }

    printf("Existing Groups:\n");
    for (int i = 0; i < *group_count; i++)
    {
        printf("%d. %s\n", i + 1, groups[i]->name);
    }
    printf("%d. Create New Group\n", *group_count + 1);

    int choice;
    do
    {
        printf("Select a group (1-%d): ", *group_count + 1);
        if (scanf("%d", &choice) != 1)
        {
            clear_input_buffer();
            printf("Invalid input. Try again.\n");
            continue;
        }
        clear_input_buffer();

        if (choice == *group_count + 1)
        {
            char group_name[MAX_NAME_LENGTH];
            printf("Enter new group name: ");
            fgets(group_name, sizeof(group_name), stdin);
            group_name[strcspn(group_name, "\n")] = 0;

            groups[*group_count] = create_group(group_name);
            return groups[(*group_count)++];
        }

        if (choice < 1 || choice > *group_count)
        {
            printf("Invalid group selection. Try again.\n");
        }
    } while (choice < 1 || choice > *group_count);

    return groups[choice - 1];
}

void print_welcome()
{
    // Slowly print each character of "WELCOME"
    char welcome[] = " WELCOME to the FairShare! ";
    for (int i = 0; welcome[i] != '\0'; i++)
    {
        printf("%c", welcome[i]);
        usleep( 50000);      // Pause for 200 milliseconds for the transition effect
    }
    usleep(100000); // Pause for 200 milliseconds for the transition effect
    printf("\n");
}

void print_user_transactions(GraphNode *head, const char *user_name)
{
    GraphNode *user = find_graph_node(head, user_name);
    if (!user)
    {
        printf("User '%s' not found in the group.\n", user_name);
        return;
    }

    printf("\n=== Transactions for User: %s ===\n", user_name);
    printf("+-----------------+-----------+\n");
    printf("| %-15s | %-9s |\n", "To", "Amount");
    printf("+-----------------+-----------+\n");

    // Outgoing transactions (initiated by the user)
    EdgeNode *edge = user->edges;
    int has_outgoing = 0;
    while (edge)
    {
        printf("| %-15s | $%8.2f |\n", edge->dest->name, edge->amount);
        edge = edge->next;
        has_outgoing = 1;
    }

    if (!has_outgoing)
        printf("No outgoing transactions found.\n");

    printf("+-----------------+-----------+\n");

    // Incoming transactions (received by the user)
    printf("+-----------------+-----------+\n");
    printf("| %-15s | %-9s |\n", "From", "Amount");
    printf("+-----------------+-----------+\n");

    GraphNode *current = head;
    int has_incoming = 0;
    while (current)
    {
        EdgeNode *current_edge = current->edges;
        while (current_edge)
        {
            if (current_edge->dest == user)
            {
                printf("| %-15s | $%8.2f |\n", current->name, current_edge->amount);
                has_incoming = 1;
            }
            current_edge = current_edge->next;
        }
        current = current->next;
    }

    if (!has_incoming)
        printf("No incoming transactions found.\n");

    printf("+-----------------+-----------+\n");
}

// Function to remove a user from a group
int remove_user_from_group(Group *group, const char *user_name)
{
    // Find the user node to remove
    GraphNode *prev = NULL;
    GraphNode *current = group->users;

    // Traverse the linked list to find the user
    while (current)
    {
        if (strcmp(current->name, user_name) == 0)
        {
            // Remove the node from the users list
            if (prev == NULL)
            {
                // User is the first node
                group->users = current->next;
            }
            else
            {
                prev->next = current->next;
            }

            // Remove all edges involving this user
            GraphNode *node = group->users;
            while (node)
            {
                // Remove edges from other users to this user
                EdgeNode *edge_prev = NULL;
                EdgeNode *edge = node->edges;
                while (edge)
                {
                    if (edge->dest == current)
                    {
                        // Remove this edge
                        if (edge_prev == NULL)
                        {
                            node->edges = edge->next;
                        }
                        else
                        {
                            edge_prev->next = edge->next;
                        }

                        // Free the edge
                        EdgeNode *temp = edge;
                        edge = edge->next;
                        free(temp);
                        continue;
                    }
                    edge_prev = edge;
                    edge = edge->next;
                }

                // Remove this user's edges to other nodes
                edge_prev = NULL;
                edge = current->edges;
                while (edge)
                {
                    EdgeNode *temp = edge;
                    edge = edge->next;
                    free(temp);
                }

                node = node->next;
            }

            // Free the current user node
            free(current);
            return 1; // User successfully removed
        }

        prev = current;
        current = current->next;
    }

    return 0; // User not found
}

// Function to remove a group from the groups array
int remove_group(Group **groups, int *group_count, const char *group_name)
{
    // Validate input
    if (groups == NULL || *group_count == 0)
    {
        printf("No groups exist to remove.\n");
        return 0;
    }

    // Find the group to remove
    for (int i = 0; i < *group_count; i++)
    {
        if (strcmp(groups[i]->name, group_name) == 0)
        {
            // Free the group's memory
            free_group(groups[i]);

            // Shift remaining groups to fill the gap
            for (int j = i; j < *group_count - 1; j++)
            {
                groups[j] = groups[j + 1];
            }

            // Decrease group count
            (*group_count)--;

            // Optional: Remove group-specific transaction and edges files
            char filename[256];

            // Remove transactions file
            snprintf(filename, sizeof(filename), "%s%s_transactions.txt",
                     TRANSACTIONS_DIR, group_name);
            remove(filename);

            // Remove edges file
            snprintf(filename, sizeof(filename), "%s%s_edges.txt",
                     TRANSACTIONS_DIR, group_name);
            remove(filename);

            printf("Group '%s' successfully removed.\n", group_name);
            return 1;
        }
    }

    printf("Group '%s' not found.\n", group_name);
    return 0;
}

int main()
{
    Group *groups[MAX_GROUPS];
    int group_count = 0;
    clear_screen();
    print_welcome();

    group_count = load_groups(groups);

    while (1)
    {
        print_menu();

        int choice;
        if (scanf("%d", &choice) != 1)
        {
            clear_input_buffer();
            printf("Invalid input. Please enter a number.\n");
            continue;
        }
        clear_input_buffer();

        switch (choice)
        {
        case 1:
        {
            char group_name[MAX_NAME_LENGTH];
            printf("Enter group name: ");
            fgets(group_name, sizeof(group_name), stdin);
            group_name[strcspn(group_name, "\n")] = 0;

            if (group_count >= MAX_GROUPS)
            {
                printf("Maximum number of groups reached.\n");
                break;
            }

            groups[group_count] = create_group(group_name);
            printf("Group '%s' created successfully!\n", group_name);
            group_count++;
            break;
        }

        case 2:
        {
            if (group_count == 0)
            {
                printf("No groups exist. Create a group first.\n");
                break;
            }

            Group *current_group = select_or_create_group(groups, &group_count);

            printf("Enter users to add (space or comma-separated, type 'done' to finish):\n");

            char input[MAX_INPUT_LEN];
            fgets(input, sizeof(input), stdin);
            input[strcspn(input, "\n")] = 0;

            // Replace commas with spaces for consistent parsing
            for (int i = 0; input[i]; i++)
            {
                if (input[i] == ',')
                    input[i] = ' ';
            }

            // Tokenize the input
            char *token = strtok(input, " ");
            int user_count = 0;

            while (token != NULL)
            {
                // Skip 'done' keyword
                if (strcmp(token, "done") == 0)
                    break;

                // Trim whitespace
                while (isspace(*token))
                    token++;
                if (*token == '\0')
                {
                    token = strtok(NULL, " ");
                    continue;
                }

                // Check if user already exists in the group
                if (find_graph_node(current_group->users, token))
                {
                    printf("User '%s' already exists in the group. Skipping.\n", token);
                }
                else
                {
                    // Add user to group
                    add_user_to_group(current_group, token);
                    printf("Added user: %s\n", token);
                    user_count++;
                }

                // Get next token
                token = strtok(NULL, " ");
            }

            if (user_count == 0)
            {
                printf("No users were added.\n");
            }
            else
            {
                printf("Total users added: %d\n", user_count);
            }
            break;
        }

        case 3:
        {
            if (group_count == 0)
            {
                printf("No groups exist. Create a group first.\n");
                break;
            }

            Group *current_group = select_or_create_group(groups, &group_count);

            char payer[MAX_NAME_LENGTH];
            char receivers[MAX_RECEIVERS][MAX_NAME_LENGTH];
            double total_amount, split_method;
            int receiver_count = 0;

            // Get payer
            printf("Enter payer's name: ");
            fgets(payer, sizeof(payer), stdin);
            payer[strcspn(payer, "\n")] = 0;

            // Get total amount
            printf("Enter total transaction amount: $");
            if (scanf("%lf", &total_amount) != 1 || total_amount <= 0)
            {
                clear_input_buffer();
                printf("Invalid amount. Transaction cancelled.\n");
                break;
            }
            clear_input_buffer();

            // Split method
            printf("Select split method:\n");
            printf("1. Equal Split\n");
            printf("2. Custom Split\n");
            if (scanf("%lf", &split_method) != 1 || (split_method != 1 && split_method != 2))
            {
                clear_input_buffer();
                printf("Invalid split method. Transaction cancelled.\n");
                break;
            }
            clear_input_buffer();

            // Get receivers
            while (1)
            {
                if (receiver_count >= MAX_RECEIVERS)
                {
                    printf("Maximum receivers reached.\n");
                    break;
                }

                char receiver[MAX_NAME_LENGTH];
                printf("Enter receiver's name (or 'done' to finish): ");
                fgets(receiver, sizeof(receiver), stdin);
                receiver[strcspn(receiver, "\n")] = 0;

                if (strcmp(receiver, "done") == 0)
                    break;

                // Prevent duplicate entries
                int is_duplicate = 0;
                for (int i = 0; i < receiver_count; i++)
                {
                    if (strcmp(receivers[i], receiver) == 0)
                    {
                        is_duplicate = 1;
                        break;
                    }
                }

                if (is_duplicate)
                {
                    printf("User already added. Skipping.\n");
                    continue;
                }

                strcpy(receivers[receiver_count], receiver);
                receiver_count++;
            }

            if (receiver_count == 0)
            {
                printf("No receivers added. Transaction cancelled.\n");
                break;
            }

            // Split and add transactions
            double split_amount = (split_method == 1) ? (total_amount / (receiver_count + 1)) : 0;

            // For equal split
            if (split_method == 1)
            {
                for (int i = 0; i < receiver_count; i++)
                {
                    add_transaction(current_group, payer, receivers[i], split_amount);
                    printf("%s paid %s: $%.2f\n", payer, receivers[i], split_amount);
                }
            }
            // For custom split
            else
            {
                for (int i = 0; i < receiver_count; i++)
                {
                    double custom_amount;
                    printf("Enter amount for %s: $", receivers[i]);
                    if (scanf("%lf", &custom_amount) != 1 || custom_amount < 0)
                    {
                        clear_input_buffer();
                        printf("Invalid amount. Skipping this receiver.\n");
                        continue;
                    }
                    clear_input_buffer();

                    add_transaction(current_group, payer, receivers[i], custom_amount);
                    printf("%s paid %s: $%.2f\n", payer, receivers[i], custom_amount);
                }
            }

            break;
        }

        case 4:
        {
            if (group_count == 0)
            {
                printf("No groups exist. Create a group first.\n");
                break;
            }

            Group *current_group = select_or_create_group(groups, &group_count);
            settle_group(current_group);
            break;
        }

        case 5:
        {
            if (group_count == 0)
            {
                printf("No groups exist. Create a group first.\n");
                break;
            }

            Group *current_group = select_or_create_group(groups, &group_count);
            print_group_balance(current_group);
            break;
        }

        case 6:
        {
            clear_screen();
            break;
        }
        case 7:
        {
            if (group_count == 0)
            {
                printf("No groups exist. Create a group first.\n");
                break;
            }

            // Select a group
            Group *current_group = select_or_create_group(groups, &group_count);

            // Get the user's name
            char user_name[MAX_NAME_LENGTH];
            printf("Enter user's name: ");
            fgets(user_name, sizeof(user_name), stdin);
            user_name[strcspn(user_name, "\n")] = 0;
            print_user_transactions(current_group->users, user_name);
            break;
        }

        case 8:
        {
            if (group_count == 0)
            {
                printf("No groups exist. Create a group first.\n");
                break;
            }

            Group *current_group = select_or_create_group(groups, &group_count);

            char user_name[MAX_NAME_LENGTH];
            printf("Enter user name to remove: ");
            fgets(user_name, sizeof(user_name), stdin);
            user_name[strcspn(user_name, "\n")] = 0;

            if (remove_user_from_group(current_group, user_name))
            {
                printf("User '%s' successfully removed from the group.\n", user_name);
            }
            else
            {
                printf("User '%s' not found in the group.\n", user_name);
            }
            break;
        }
        case 9: // Remove Group
        {
            if (group_count == 0)
            {
                printf("No groups exist to remove.\n");
                break;
            }

            char group_name[MAX_NAME_LENGTH];
            printf("Enter group name to remove: ");
            fgets(group_name, sizeof(group_name), stdin);
            group_name[strcspn(group_name, "\n")] = 0;

            remove_group(groups, &group_count, group_name);
            break;
        }
        case 0:
        {
            // Save groups before exiting
            if (save_groups(groups, group_count))
            {
                printf("Groups and transactions saved successfully.\n");
            }
            else
            {
                printf("Failed to save groups and transactions.\n");
            }

            // Free all groups before exiting
            for (int i = 0; i < group_count; i++)
            {
                free_group(groups[i]);
            }
            printf("Goodbye!\n");
            return 0;
        }

        default:
            printf("Invalid choice. Please try again.\n");
        }
    }

    return 0;
}