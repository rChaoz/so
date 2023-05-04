#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

#include "os_graph.h"
#include "os_threadpool.h"

#define MAX_TASK 100
#define MAX_THREAD 4

pthread_mutex_t sumLock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t visLock = PTHREAD_MUTEX_INITIALIZER;

os_threadpool_t *tp;

os_graph_t *graph;
volatile int sum = 0;

// Function to add a task to threadpool, trying again if tasklist is full
void processNode(void *nodeIdxVoid);
void add_node_task(unsigned int nodeIdx) {
    os_task_t *task = task_create((void*) (long) nodeIdx, processNode);
    add_task_in_queue(tp, task);
}

void processNode(void *nodeIdxVoid) {
    unsigned int nodeIdx = (unsigned int) (long) nodeIdxVoid;
    os_node_t *node = graph->nodes[nodeIdx];

    pthread_mutex_lock(&sumLock);
    sum += node->nodeInfo;
    pthread_mutex_unlock(&sumLock);

    // Add neighbours to task queue
    for (int i = 0; i < node->cNeighbours; i++) {
        pthread_mutex_lock(&visLock);
        if (graph->visited[node->neighbours[i]] == 0) {
            graph->visited[node->neighbours[i]] = 1;
            add_node_task(node->neighbours[i]);
        }
        pthread_mutex_unlock(&visLock);
    }
    // Node process complete
    graph->visited[nodeIdx] = 2;
}

// Check if entire graph was processsed
int is_done() {
    for (int i = 0; i < graph->nCount; ++i) {
        // If the graph is not connected, add the first unprocessed node to the task queue
        // when all tasks are complete
        if (graph->visited[i] == 0) {
            if (tp->tasks == NULL) {
                pthread_mutex_lock(&visLock);
                if (graph->visited[i] == 0) {
                    graph->visited[i] = 1;
                    add_node_task(i);
                }
                pthread_mutex_unlock(&visLock);
            }
            return 0;
        } else if (graph->visited[i] != 2) return 0;
    }
    return 1;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s input_file\n", argv[0]);
        exit(1);
    }

    FILE *input_file = fopen(argv[1], "r");

    if (input_file == NULL) {
        printf("[Error] Can't open file\n");
        return -1;
    }

    graph = create_graph_from_file(input_file);
    if (graph == NULL) {
        printf("[Error] Can't read the graph from file\n");
        return -1;
    }

    tp = threadpool_create(MAX_TASK, MAX_THREAD);
    // is_done will add the first node to queue
    threadpool_stop(tp, is_done);

    printf("%d", sum);
    return 0;
}
