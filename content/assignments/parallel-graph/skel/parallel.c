#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <time.h>

#include "os_graph.h"
#include "os_threadpool.h"

#define MAX_TASK 100
#define MAX_THREAD 4

pthread_mutex_t sumLock = PTHREAD_MUTEX_INITIALIZER;
os_threadpool_t *tp;
int sum = 0;
os_graph_t *graph;

// Function to add a task to threadpool, trying again if tasklist is full
struct timespec milli = {0, 1000000L};
void processNode(void *nodeIdxVoid);
void add_node_task(unsigned int nodeIdx) {
    os_task_t *task = task_create((void*) (long long) nodeIdx, processNode);
    while (!add_task_in_queue(tp, task)) nanosleep(&milli, NULL);
}

void processNode(void *nodeIdxVoid) {
    unsigned int nodeIdx = (unsigned int) (long long) nodeIdxVoid;
    // It is possible to add a node twice ocasionally, still, don't process it twice
    // We prefer to do this instead of use a lock because a lock would be significantly slower
    if (graph->visited[nodeIdx]) return;

    os_node_t *node = graph->nodes[nodeIdx];

    pthread_mutex_lock(&sumLock);
    sum += node->nodeInfo;
    pthread_mutex_unlock(&sumLock);

    for (int i = 0; i < node->cNeighbours; i++)
        if (graph->visited[node->neighbours[i]] == 0) {
            graph->visited[node->neighbours[i]] = 1;
            add_node_task(node->neighbours[i]);
        }
}

void traverse_graph() {
    for (int i = 0; i < graph->nCount; i++) {
        if (graph->visited[i] == 0) {
            graph->visited[i] = 1;
            add_node_task(i);
        }
    }
}

// Check if entire graph was processsed
int is_done() {
    for (int i = 0; i < graph->nCount; ++i) if (!graph->visited[i]) return 0;
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
    traverse_graph();
    threadpool_stop(tp, is_done);

    printf("%d", sum);
    return 0;
}
