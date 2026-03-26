// This file computes the top event probability of a fault tree using IS-MCTS algorithm
// OS: Windows

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>
#include <ctype.h>
#include <time.h>
#include "events.h"     // number of unique basic events (num_unique_bes) & number of events (num_events) of the fault tree

#define max_str_size 5 // maximum string size for each gate

// IS-MCTS_TREE
// node struct
struct node {
    unsigned int num_visits;
    double reward;
    struct node * left;  // left: not occur
    struct node * right; // right: occur
};

// Create a new Node
struct node* createNode() {
    struct node * newNode = (struct node *) malloc(sizeof(struct node));
    newNode->num_visits = 1;
    newNode->reward = 0.;
    newNode->left = NULL;
    newNode->right = NULL;

    return newNode;
}

// Insert on the left of the node
struct node * insertLeft(struct node * currentnode) {
    currentnode->left = createNode();
    return currentnode->left;
}

// Insert on the right of the node
struct node * insertRight(struct node * currentnode) {
    currentnode->right = createNode();
    return currentnode->right;
}

// Function to split a line into an array of integers
int* split_line_to_ints(char *line, int *count) {
    int *arr = NULL;
    int num, n = 0;
    char *token = strtok(line, " ");
    
    while (token != NULL) {
        if (sscanf(token, "%d", &num) == 1) {
            arr = realloc(arr, (n + 1) * sizeof(int));
            if (arr == NULL) {
                fprintf(stderr, "Memory allocation failed\n");
                exit(1);
            }
            arr[n++] = num;
        }
        token = strtok(NULL, " ");
    }
    
    *count = n;
    return arr;
}

// Function to check if a string contains only numeric characters (used to detect k/n gates)
int is_numeric_string(const char *str) {
    // Check if the string is not empty
    if (*str == '\0') {
        return 0; // False: empty string
    }

    // Iterate through each character of the string
    while (*str) {
        // Check if the character is not a digit
        if (!isdigit((unsigned char)*str)) {
            return 0; // False: non-digit character found
        }
        str++;
    }

    return 1; // True: all characters are digits
}

// Function to compute the squared value
double square(double x) { 
    return x*x; 
}

// Function to compute the natural log using Taylor series approximation in the domain [1,exp(2)] around point a = 3.8843486492669905 which outputs the lowest MSE
float log_approx(unsigned int x){
    unsigned short i = 0;  // 
    while (x > 7.38905609893065)    // exp(2) = 7.38905609893065
    {
        x /= (float) 7.38905609893065;
        i += 2;
    }
    return -0.03313855720162471*x*x + 0.5148868396191513*x - 0.14304468808287152 + i;
}

int main()
{
    unsigned int i, j, k, l;
    float computation_time;
    double te_prob_hist;
    double explore_weight = 1.0;
    unsigned int count_top = 0;

    // READ FILES

    FILE *stream;

//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////

    /*read unique_be_probs*/
    char filename_unique_be_probs[] = "converted_unique_be_probs.txt"; // Name of the text file
    double unique_be_probs[num_unique_bes]; // Array to store basic events probabilities
    unsigned int be_prob_index = 0; // Number of basic events read
    double be_prob_temp; // Temporary variable to store each basic event probability

    // Open the file
    stream = fopen(filename_unique_be_probs, "r");
    if (stream == NULL) {
        fprintf(stderr, "Could not open file %s\n", filename_unique_be_probs);
        return 1;
    }

    // Read basic events probabilities from the file and store them in basic events probabilities array
    while (fscanf(stream, "%lf", &be_prob_temp) == 1 && be_prob_index < num_unique_bes) {
        unique_be_probs[be_prob_index] = be_prob_temp;
        be_prob_index++;
    }

    // Close the file
    fclose(stream);

//////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

    /*read events_list_ordered*/
    char filename_events_list_ordered[] = "converted_events_list_ordered.txt";
    int events[num_events]; // Array to store event numbers
    unsigned int event_index = 0; // Number of events read
    unsigned int event_temp; // Temporary int variable to store each event

    // Open the file
    stream = fopen(filename_events_list_ordered, "r");
    if (stream == NULL) {
        fprintf(stderr, "Could not open file %s\n", filename_events_list_ordered);
        return 1;
    }

    // Read event numbers from the file and store them in the events array
    while (fscanf(stream, "%d", &event_temp) == 1) {
        events[event_index] = event_temp;
        event_index++;
    }

    // Close the file
    fclose(stream);

//////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

    /*read gates_list_ordered*/
    char filename_gates_list_ordered[] = "converted_gates_list_ordered.txt";
    char gates_str[num_events][max_str_size]; // Array to store gate type as a string
    short gates[num_events]; // Array to store gate type as an integer
    unsigned int gate_index = 0; // Number of gates read

    // Open the file
    stream = fopen(filename_gates_list_ordered, "r");
    if (stream == NULL) {
        fprintf(stderr, "Could not open file %s\n", filename_gates_list_ordered);
        return 1;
    }

    // Read strings from the file and store them in the predefined array
    while (gate_index < num_events && fscanf(stream, "%255s", gates_str[gate_index]) == 1) {
        gate_index++;
    }

    // Close the file
    fclose(stream);

    // Map the gates from string to integers
    for (int z = 0; z < num_events; z++) {
        if (strcmp(gates_str[z], "b") == 0) {           // basic event (no gate associated with this event)
            gates[z] = 0;
        } else if (strcmp(gates_str[z], "+") == 0) {
            gates[z] = -1;
        } else if (strcmp(gates_str[z], "*") == 0) {    // AND gate (-2)
            gates[z] = -2;
        } else if (strcmp(gates_str[z], "n") == 0) {    // negation gate (-3)
            gates[z] = -3;
        } else if (is_numeric_string(gates_str[z])) {	// k/n gate (> 0)
            gates[z] = atoi(gates_str[z]);  // convert string to integer using atoi
        } else {
            fprintf(stderr, "unexpected gate type: %s\n", gates_str[z]);
            return 1;
        }
    }

//////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

    /*read child_events_list_order*/
    char filename_child_events_list_ordered[] = "converted_child_events_list_ordered.txt";
    char line[1024]; // Buffer to store each line
    int *child_events[num_events]; // Array of pointers to store events_temp arrays
    unsigned int nums_child_events[num_events]; // Array to store the number of events in each line
    unsigned int line_index = 0; // Current line index
    int *events_temp; // Temporary array to store events of a single line
    unsigned int num_events_temp; // Temporary variable to store the number of events in a single line

    stream = fopen(filename_child_events_list_ordered, "r");
    if (stream == NULL) {
        fprintf(stderr, "Could not open file %s\n", filename_child_events_list_ordered);
        return 1;
    }

    while (fgets(line, sizeof(line), stream) != NULL && line_index < num_events) {
        // Remove newline character if present
        line[strcspn(line, "\n")] = 0;
        
        events_temp = split_line_to_ints(line, &num_events_temp);
        
        child_events[line_index] = events_temp;
        nums_child_events[line_index] = num_events_temp;
        line_index++;
    }

    fclose(stream);

//////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

    // RUN IS-MCTS
    clock_t start_simulation = clock();
    clock_t end_simulaton;
    bool unique_be_comb[num_unique_bes];
    double weight;
    // initialize mc_tree
    struct node * mc_tree = createNode();
    struct node * currentnode;
    // initialize proposal probs
    double prop_prob_left;
    double prop_prob_right;
    // random number
    float randnum;
    // states of events
    bool states[num_events];
    // termination condition
    double sum_sq = 0;  // sum of squares (used to compute the termination condition)
    double variance; // variance (used to compute the termination condition)
    unsigned int i_min;      // minimum number of simulation for convergence (used to compute the termination condition)
    float a = 0.1;     // confidence interval parameter (used to compute the termination condition)
    unsigned int max_sim = 1750000;    // max number of simulations
    bool condition = 1;

    unsigned int seed = 981;    // Seed value
    srand(seed);    // Set the seed

    while (condition == 1 && i < max_sim){   // while loop will stop when termination condition becomes 0 & number of simulations is below the max value
        // initialize weight to be 1.
        weight = 1.;
        // initialize currentnode
        currentnode = mc_tree;
        // for all unique basic events
        for (j = 0; j < num_unique_bes; j++){
            // increase the num_visits of the currentnode
            currentnode->num_visits += 1;
            //  for the left branch (basic event does not occur)
            if (currentnode->left == NULL){
                prop_prob_left = (double) (1. - unique_be_probs[j]) * explore_weight * sqrt(log_approx(currentnode->num_visits));
            }
            else {
                prop_prob_left = (double) (1. - unique_be_probs[j]) * (currentnode->left->reward / currentnode->left->num_visits + explore_weight * sqrt(log_approx(currentnode->num_visits) / currentnode->left->num_visits));
            }
            //  for the right branch (basic event does occur)
            if (currentnode->right == NULL){
                prop_prob_right = (double) unique_be_probs[j] * explore_weight * sqrt(log_approx(currentnode->num_visits));
            }
            else {
                prop_prob_right = (double) unique_be_probs[j] * (currentnode->right->reward / currentnode->right->num_visits + explore_weight * sqrt(log_approx(currentnode->num_visits) / currentnode->right->num_visits));
            }
            //  normalize the distribution
            prop_prob_left /= (prop_prob_left + prop_prob_right);
            prop_prob_right = 1 - prop_prob_left;
            //  determine if unique basic event j occurs or not
            randnum = (float) rand() / RAND_MAX;
            //  if left, i.e., unique basic event j does not occur
            if (randnum < prop_prob_left){
                // update weight
                weight *= (1 - unique_be_probs[j]) / prop_prob_left;
                //  append the combination array with this unique basic event state
                unique_be_comb[j] = 0;
                if (currentnode->left == NULL){
                    // not explored yet
                    currentnode = insertLeft(currentnode);
                }
                else {
                    currentnode = currentnode->left;
                }
            }
            else {
                // update weight
                weight *= unique_be_probs[j] / prop_prob_right;
                // if right, i.e., unique basic event j occurs
                unique_be_comb[j] = 1;
                if (currentnode->right == NULL){
                    // not explored yet
                    currentnode = insertRight(currentnode);
                }
                else {
                    currentnode = currentnode->right;
                }
            }
        }
        //  check if the combination cause the top event to occur
        for (k = 0; k < num_events; k++){
            // assign the states of the unique basic events
            if (k < num_unique_bes){
                states[k] = unique_be_comb[k];
            }
            else {
                //  if the node connects to its child nodes via an AND gate (-2)
                if (gates[k] == -2){
                    states[k] = 1;
                    for (l = 0; l < nums_child_events[k]; l++){
                        if (states[child_events[k][l]] == 0){
                            states[k] = 0;
                            break;
                        }
                    }
                }
                //  if the node connects to its child nodes via an OR gate (-1)
                else if (gates[k] == -1){
                    states[k] = 0;
                    for (l = 0; l < nums_child_events[k]; l++){
                        if (states[child_events[k][l]] == 1){
                            states[k] = 1;
                            break;
                        }
                    }
                }
                //  if the node connects its child nodes via a negation gate (-3)
                else if (gates[k] == -3) {
                    states[k] = !states[child_events[k][0]];
                }
                //  if the node connects its child nodes via a k/n gate
                else if (gates[k] > 0) {
                    if (gates[k] > nums_child_events[k]) {
                        perror("k > n");
                        exit(EXIT_FAILURE);
                    }
                    states[k] = 0;
                    int count_kn = 0;
                    for (l = 0; l < nums_child_events[k]; l++) {
                        if (states[child_events[k][l]] == 1) {
                            count_kn++;
                            if (count_kn >= gates[k]) {
                                states[k] = 1;
                                break;
                            }
                        }
                    }
                }
                //  if gate type is not included
                else {
                    perror("gate type not included.");
                    exit(EXIT_FAILURE);
                }
            }
        }
        //  backpropagate the reward
        if (states[num_events - 1]) {
            currentnode = mc_tree;
            count_top += 1;     // number of simulations that caused the top event state to be 1
            for (j = 0; j < num_unique_bes; j++) {
                if (unique_be_comb[j] == 0) {
                    currentnode->left->reward += 1;
                    currentnode = currentnode->left;
                }
                else {
                    currentnode->right->reward += 1;
                    currentnode = currentnode->right;
                }
            }
        }
        //  record the te probability estimate
        if (i == 0) {
            te_prob_hist = (double) weight * states[num_events - 1];
        }
        else {
            te_prob_hist = (double) (te_prob_hist * i + weight * states[num_events - 1]) / (i + 1);
        }

        // termination criterion
        sum_sq += square(weight * states[num_events - 1]);
        variance = (sum_sq - (i + 1) * square(te_prob_hist)) / i;
        i_min = ceil(variance*square(5.16/(a*te_prob_hist)));

        if (i > i_min && te_prob_hist > 0 && te_prob_hist < 1)
        {
            condition = 0;
        }

        i++;

        // computation time
        end_simulaton = clock();
        computation_time = ((float) (end_simulaton - start_simulation)) / CLOCKS_PER_SEC;

    }

    // Save top event probability and run time in a text file
    stream = fopen("Results.txt", "w");
    if (stream == NULL) {
        // If there is an error opening the file, print an error message and return
        printf("Error opening file!\n");
        return 1;
    }

    // Write the numbers to the file, each on a new line
    fprintf(stream, "%u\n", i);      // total number of simulations
    fprintf(stream, "%u\n", count_top);      // number of simulations that caused the top event state to be 1
    fprintf(stream, "%.20lf\n", te_prob_hist);    // top event probability
    fprintf(stream, "%f\n", computation_time);      // run time
    fprintf(stream, "%u\n", i_min);      // minimum number of simulations for convergence

    // Close the file
    fclose(stream);

    // Free allocated memory
    for (int g = 0; g < num_events; g++) {
        free(child_events[g]);
    }

    return 0;
}