#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <omp.h>
#include <time.h>

#define ROWS 512
#define COLS 512

typedef struct Node {
    int x, y;
    int f, g, h;
    struct Node *parent;
    bool isWall; // Nova propriedade para verificar se o nó é uma parede
} Node;

Node *grid[ROWS][COLS];

Node *createNode(int x, int y, bool isWall) {
    Node *node = (Node *)malloc(sizeof(Node));
    node->x = x;
    node->y = y;
    node->f = 0;
    node->g = 0;
    node->h = 0;
    node->parent = NULL;
    node->isWall = isWall;
    return node;
}

int heuristic(Node *current, Node *goal) {
    return abs(current->x - goal->x) + abs(current->y - goal->y);
}

bool isValid(int x, int y) {
    return x >= 0 && x < ROWS && y >= 0 && y < COLS && !grid[x][y]->isWall;
}

bool isSameNode(Node *node1, Node *node2) {
    return node1->x == node2->x && node1->y == node2->y;
}

void printGrid(Node *start, Node *goal) {
    // Declara a variável `output`
    char output[ROWS][COLS];

    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLS; j++) {
            if (grid[i][j]->isWall) {
                output[i][j] = '|';  // Marca as paredes com '#'
            } else {
                output[i][j] = '.';  // Marca os caminhos com '.'
            }
        }
    }

    Node *path = goal;
    while (path != NULL) {
        output[path->x][path->y] = '*'; // Marca o caminho com '*'
        path = path->parent;
    }

    output[start->x][start->y] = 'S';
    output[goal->x][goal->y] = 'D'; // Marca o destino com 'D'

    // Imprime a matriz `output`
    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLS; j++) {
            printf("%c ", output[i][j]);
        }
        printf("\n");
    }
}

bool closedListContains(Node *closedList[], int count, Node *node) {
    for (int i = 0; i < count; i++) {
        if (isSameNode(closedList[i], node)) {
            return true;
        }
    }
    return false;
}

bool openListContains(Node *openList[], int count, Node *node) {
    for (int i = 0; i < count; i++) {
        if (isSameNode(openList[i], node)) {
            return true;
        }
    }
    return false;
}

void AStar(Node *start, Node *goal) {
    Node *openList[ROWS * COLS];
    Node *closedList[ROWS * COLS];
    int openCount = 0;
    int closedCount = 0;

    openList[openCount++] = start;

    while (openCount > 0) {
        int currentIndex = 0;
        int minF = openList[0]->f;
        for (int i = 1; i < openCount; i++) {
            if (openList[i]->f < minF) {
                minF = openList[i]->f;
                currentIndex = i;
            }
        }

        Node *current = openList[currentIndex];

        for (int i = currentIndex; i < openCount - 1; i++) {
            openList[i] = openList[i + 1];
        }
        openCount--;

        closedList[closedCount++] = current;

        if (isSameNode(current, goal)) {
            printf("Caminho encontrado:\n");
            Node *path = current;
            while (path != NULL) {
                printf("(%d, %d) -> ", path->x, path->y);
                path = path->parent;
            }
            printf("\n");

            printf("Representacao visual do grid com o caminho:\n");
            printGrid(start, goal);
            return;
        }

        int dx[] = {-1, 1, 0, 0};
        int dy[] = {0, 0, -1, 1};

        #pragma omp parallel for num_threads(4)
        for (int i = 0; i < 4; i++) {
            int newX = current->x + dx[i];
            int newY = current->y + dy[i];

            if (isValid(newX, newY)) {
                Node *neighbor = grid[newX][newY];

                if (!closedListContains(closedList, closedCount, neighbor)) {
                    int tentativeG = current->g + 1;

                    if (!openListContains(openList, openCount, neighbor) || tentativeG <= neighbor->g) {
                        #pragma omp critical
                        {
                           neighbor->parent = current;
                           neighbor->g = tentativeG;
                           neighbor->h = heuristic(neighbor, goal);
                           neighbor->f = neighbor->g + neighbor->h;
                        }
                        if (!openListContains(openList, openCount, neighbor)) {
                            #pragma omp critical
                            {
                                openList[openCount++] = neighbor;
                            }
                        }
                    }
                }
            }
        }
    }

    printf("Caminho não encontrado.\n");
}

int main() {
    
    clock_t start_time, end_time;
    double cpu_time_used;
    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLS; j++) {
            if (grid[i][j] == NULL) {
                bool isWall = (rand() % 100) < 20;
                grid[i][j] = createNode(i, j, isWall);
            }
        }
    }

    Node *start = grid[0][0];
    Node *goal = grid[511][511];

    printf("Nó de início: (%d, %d)\n", start->x, start->y);
    printf("Nó de destino: (%d, %d)\n", goal->x, goal->y);

    start_time = clock();
    AStar(start, goal);
    end_time = clock();

    cpu_time_used = ((double) (end_time - start_time)) / CLOCKS_PER_SEC;
    printf("Tempo de execucao: %f segundos\n", cpu_time_used);

    return 0;
}
