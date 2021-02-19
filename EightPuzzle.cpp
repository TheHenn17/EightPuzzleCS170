//CS170 Project 1: Design a 8-Puzzle solver using A* search
//Name: Thomas Henningson
//SID: 862073029

#include <iostream>
#include <queue>
#include <chrono>
using namespace std;

//node struct gives use the state of the 8-puzzle, as well as its heuristic value and cost value
struct node {
    int** state;
    int hVal;
    int cost;
};

//Comparator function for the standard priority queue
//I had to look up how to do this by looking at an example
//https://www.geeksforgeeks.org/stl-priority-queue-for-structure-or-class/
struct Compare {
    bool operator()(node* a, node* b)
    {
        return (a->hVal + a->cost) > (b->hVal + b->cost);
    }
};

//Lets user input a puzzle, row by row
int** createPuzzle(int size);

//Modified General Search algorithm that only uses A*, but switches the heuristic function
void GSA(int** puzzle, int size, int hNum, char readout);

//Used to create another state in the heap; makes getting child nodes easier
int** cpy(int** puzzle, int size);

//returns heuristic value based on user input
int heuristic(int** puzzle, int hNum, int size);

//helper function to heuristic; given the row, column, and value of the misplaced tile it calculates the manhatten by the horizontal and vertical distance to place it correctly
int getManhatten(int tile, int row, int col, int size);

//Helper function to getManhatten; returns absolute value of a number
int absVal(int num);

//helper function displays the state
void outputPuzzle(int** state, int size);

//tests if given state is the goal state; should work with all puzzle sizes
bool GoalTest(int** state, int size);

//Expands the parent node using the operators up, down, left, right; adds children to the queue
void Expand(priority_queue<node*, vector<node*>, Compare>& q, vector<node*> c, node* n, int size, int hNum);

//checks if a repeated state has been produced
bool isRepeated(vector<node*> c, node* child, int size);

//delete dynamically allocated 2D arrays
void deletePuzzle(int** puzzle, int size);

int main()
{
    //used for user input
    int input;
    char readout;
    
    //double int pointer makes a dynamic 2D array, used to store the states
    int** puzzle = 0;

    //Sets the size of the puzzle by the length of its side
    //Code below is general, so changing this value allows other size puzzles to be solved
    int size = 3;

    //Introduction, gathering input for puzzle data; checking if input is valid
    cout << "Welcome to Thomas Henningson's 8-Puzzle Solver.\n";
    cout << "Type \"1\" to use a default puzzle, or \"2\" to enter your own puzzle\n";
    cin >> input;
    cout << endl;
    while (input != 1 && input != 2) {
        cout << "Invalid input.\n";
        cout << "Type \"1\" to use a default puzzle, or \"2\" to enter your own puzzle\n";
        cin >> input;
        cout << endl;
    }

    //Branch either sets the default puzzle, or calls createPuzzle to let user input their own
    //Exits if input did not take branch (should not happen)
    if (input == 1) {
        puzzle = new int* [size];
        for (int i = 0; i < size; i++) {
            puzzle[i] = new int[size];
        }
        puzzle[0][0] = 1;
        puzzle[0][1] = 2;
        puzzle[0][2] = 3;
        puzzle[1][0] = 4;
        puzzle[1][1] = 0;
        puzzle[1][2] = 6;
        puzzle[2][0] = 7;
        puzzle[2][1] = 5;
        puzzle[2][2] = 8;
        cout << "Default puzzle is:\n";
        outputPuzzle(puzzle, size);
    }
    else if (input == 2) {
        puzzle = createPuzzle(size);
        cout << endl;
    }
    else {
        cout << "User input error. Exiting...\n";
        return 1;
    }
    //If puzzle is not created due to some error, exits
    if (puzzle == 0) {
        return 1;
    }

    //User inputs a choice of algorithm; checking if input is valid
    cout << "Enter your choice of algorithm\n";
    cout << "   1. Uniform Cost Search\n";
    cout << "   2. A* with the Misplaced Tile heuristic\n";
    cout << "   3. A* with the Manhattan Distance heuristic\n";
    cin >> input;
    cout << endl;
    while (input != 1 && input != 2 && input != 3) {
        cout << "Invalid input.\n";
        cout << "Enter your choice of algorithm\n";
        cout << "   1. Uniform Cost Search\n";
        cout << "   2. A* with the Misplaced Tile heuristic\n";
        cout << "   3. A* with the Manhattan Distance heuristic\n";
        cin >> input;
        cout << endl;
    }

    //Asks user if they would like to hide state readout
    cout << "Would you like to hide the state readout to save paper?\n";
    cout << "Press \"y\" to hide readout, or \"n\" to display it\n";
    cin >> readout;
    cout << endl;
    while (readout != 'y' && readout != 'n') {
        cout << "Invalid input.\n";
        cout << "Press \"y\" to hide readout, or \"n\" to display it\n";
        cin >> input;
        cout << endl;
    }

    //Calls the search algorithm/solver
    GSA(puzzle, size, input, readout);

    //Cleanup, make sure no dynamically allocated data remains in the heap
    deletePuzzle(puzzle, size);
    return 0;
}

//Lets user input a puzzle, row by row
int** createPuzzle(int size) {
    //Does not set puzzle if size is redundant; exits in main
    if (size <= 1) {
        cout << "Invalid Size. Exiting...\n";
        return 0;
    }
    int** puzzle = new int* [size];
    for (int i = 0; i < size; i++) {
        puzzle[i] = new int[size];
    }
    cout << "Enter your puzzle, use a zero to represent the blank\n";
    for (int i = 0; i < size; i++) {
        cout << "Enter row " << i+1 << ", use space or tabs between numbers   ";
        for (int j = 0; j < size; j++) {
            cin >> puzzle[i][j];
        }
    }
    return puzzle;
}

//Modified General Search algorithm that only uses A*, but switches the heuristic function
void GSA(int** puzzle, int size, int hNum, char readout) {
    //priority queue will always pop the node we want to expand
    //this is thanks to the Compare function we defined above
    //pops the smallest value of heuristic + cost, exactly how A* wants
    //https://www.geeksforgeeks.org/stl-priority-queue-for-structure-or-class/
    priority_queue<node*, vector<node*>, Compare> q;

    //Holds the states already looked at so we do not repeat
    vector<node*> c;

    //holds the total number of expansions the algorithm has and will perform
    int numExpansions = 0;

    //holds the maximum amount of nodes that were in the queue at one time
    int max = 0;

    //used for holding the node values in the loop, and sets the initial node to the users puzzle using cpy function
    node* n = new node;
    n->state = cpy(puzzle, size);

    //calculates intial states heuristic; sets cost to 0; adds to the queue
    n->hVal = heuristic(n->state, hNum, size);
    n->cost = 0;
    q.push(n);

    //Lets user know the algorithm has started; outputPuzzle shows user what the algorithm is working on
    //Alternatively; hides this information
    if (readout == 'n') {
        cout << "Expanding state\n";
        outputPuzzle(n->state, size);
    }
    else {
        cout << "Calculating...\n\n";
    }

    //loops while queue is not empty; should be infinte since we repeat states, but if we did not it would be finite
    while (!q.empty()) {
        //set new maximum size of queue, if applicable
        if (q.size() > max) { max = q.size(); }

        //pops queue with node that has smallest heuristic value + cost
        n = q.top(); q.pop();

        //Tests if node is the goal
        //if yes, output collected data and return to exit program
        //if not, expand the node and add the new states to the queue
        //Cleans up the rest of the queue
        if (GoalTest(n->state, size)) {
            cout << "Goal!\n\n";
            cout << "To solve this problem the search algorithm expanded a total of " << numExpansions << " nodes.\n";
            cout << "The maximum number of nodes in the queue at any one time was " << max << ".\n";
            cout << "The depth of the goal node was " << n->cost << ".\n\n";
            cout << "Cleaning up... ";
            deletePuzzle(n->state, size);
            while (!q.empty()) {
                n = q.top(); q.pop();
                deletePuzzle(n->state, size);
                delete n;
                n = 0;
            }
            while (!c.empty()) {
                n = c.back(); c.pop_back();
                deletePuzzle(n->state, size);
                delete n;
                n = 0;
            }
            cout << "Done.\n";
            return;
        }
        else { Expand(q, c, n, size, hNum); numExpansions++; }

        c.push_back(n);

        //Lets user know the algorithm has expanded; outputPuzzle shows user what the algorithm is working on
        //Alternatively; hides this information
        n = q.top();
        if (readout == 'n') {
            cout << "The best state to expand to with a g(n) = " << n->cost << " and h(n) = " << n->hVal << " is...\n";
            outputPuzzle(n->state, size);
        }
    }

    //out of the loop means no goal state was found; the puzzle is unsolvable
    cout << "No Solution.\n";
}

//Used to create another state in the heap; makes getting child nodes easier
int** cpy(int** puzzle, int size) {
    int** cpy = new int* [size];
    for (int i = 0; i < size; i++) {
        cpy[i] = new int[size];
    }
    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {
            cpy[i][j] = puzzle[i][j];
        }
    }
    return cpy;
}

//returns heuristic value based on user input
int heuristic(int** puzzle, int hNum, int size) {
    int num = 1;
    int val = 0;

    //harcode 0 heuristic means alogrithm turns into uniform cost search
    if (hNum == 1) {
        return 0;
    }

    //Misplaced tile heuristic: if any tile but the blank is in the wrong place; value of heuristic increments
    if (hNum == 2) {
        for (int i = 0; i < size; i++) {
            for (int j = 0; j < size; j++) {
                if (puzzle[i][j] != 0 && puzzle[i][j] != num++) {
                    val++;
                }
            }
        }
        return val;
    }

    //Manhatten Distance heuristic: for tiles out of place, value of heuristic increases with how many moves would put it in place
    if (hNum == 3) {
        for (int i = 0; i < size; i++) {
            for (int j = 0; j < size; j++) {
                if (puzzle[i][j] != 0 && puzzle[i][j] != num) {
                    //piece is out of place; call getManhatten to calculate moves to put it back
                    val += getManhatten(puzzle[i][j], i, j, size);
                }
                num++;
            }
        }
        return val;
    }
}

//helper function to heuristic; given the row, column, and value of the misplaced tile it calculates the manhatten by the horizontal and vertical distance to place it correctly
int getManhatten(int tile, int row, int col, int size) {
    if ((tile >= size * size) || (row >= size) || (col >= size)) {
        return -1;
    }
    int num = 1;
    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {
            if (num == tile) {
                return (absVal(i - row) + absVal(j - col));
            }
            num++;
        }
    }
}

//Helper function to getManhatten; returns absolute value of a number
int absVal(int num) {
    if (num < 0) { return -1 * num; }
    return num;
}

//helper function displays the state
void outputPuzzle(int** state, int size) {
    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {
            cout << state[i][j] << " ";
        }
        cout << endl;
    }
    cout << endl;
}

//tests if given state is the goal state; should work with all puzzle sizes
bool GoalTest(int** state, int size) {
    int num = 1;
    if (state[size - 1][size - 1] != 0) {
        return false;
    }
    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {
            if (i == size - 1 && j == size - 1) {}
            else {
                if (state[i][j] != num++) { return false; }
            }
        }
    }
    return true;
}

//Expands the parent node using the operators up, down, left, right; adds children to the queue
void Expand(priority_queue<node*, vector<node*>, Compare>& q, vector<node*> c, node* parent, int size, int hNum) {
    node* child = 0;
    int row = -1;
    int col = -1;

    //Finds position of the blank tile
    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {
            if (parent->state[i][j] == 0) {
                row = i;
                col = j;
            }
        }
    }

    //operator one: blank goes up
    //Not applicable to blanks in the top row
    if (row != 0) {
        child = new node;
        child->state = cpy(parent->state, size);
        child->state[row][col] = child->state[row-1][col];
        child->state[row-1][col] = 0;
        child->hVal = heuristic(child->state, hNum, size);
        child->cost = parent->cost + 1;
        if (!isRepeated(c, child, size)) {
            q.push(child);
        }
        else {
            deletePuzzle(child->state, size);
            delete child;
            child = 0;
        }
    }

    //operator two: blank goes down
    //Not applicable to blanks in the last row
    if (row != size-1) {
        child = new node;
        child->state = cpy(parent->state, size);
        child->state[row][col] = child->state[row + 1][col];
        child->state[row + 1][col] = 0;
        child->hVal = heuristic(child->state, hNum, size);
        child->cost = parent->cost + 1;
        if (!isRepeated(c, child, size)) {
            q.push(child);
        }
        else {
            deletePuzzle(child->state, size);
            delete child;
            child = 0;
        }
    }
    
    //operator three: blank goes left
    //Not applicable to blanks in the first column
    if (col != 0) {
        child = new node;
        child->state = cpy(parent->state, size);
        child->state[row][col] = child->state[row][col - 1];
        child->state[row][col - 1] = 0;
        child->hVal = heuristic(child->state, hNum, size);
        child->cost = parent->cost + 1;
        if (!isRepeated(c, child, size)) {
            q.push(child);
        }
        else {
            deletePuzzle(child->state, size);
            delete child;
            child = 0;
        }
    }

    //operator four: blank goes right
    //Not applicable to blanks in the last column
    if (col != size-1) {
        child = new node;
        child->state = cpy(parent->state, size);
        child->state[row][col] = child->state[row][col + 1];
        child->state[row][col + 1] = 0;
        child->hVal = heuristic(child->state, hNum, size);
        child->cost = parent->cost + 1;
        if (!isRepeated(c, child, size)) {
            q.push(child);
        }
        else {
            deletePuzzle(child->state, size);
            delete child;
            child = 0;
        }
    }
}

//Helper to Expand; checks if a repeated state has been produced
bool isRepeated(vector<node*> c, node* child, int size) {
    bool isSame = true;
    if (!c.empty()) {
        for (int i = 0; i < c.size(); i++) {
            for (int j = 0; j < size; j++) {
                for (int k = 0; k < size; k++) {
                    if (c.at(i)->state[j][k] != child->state[j][k]) {
                        isSame = false;
                    }
                }
            }
            if (isSame) {
                return true;
            }
            isSame = true;
        }
    }
    return false;
}

//Deallocates puzzles in memory
void deletePuzzle(int** puzzle, int size) {
    for (int i = 0; i < size; i++) {
        delete[] puzzle[i];
    }
    delete[] puzzle;
}