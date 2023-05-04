// SPDX-License-Identifier: BSD-3-Clause

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

#include "os_graph.h"
#include "os_threadpool.h"

#define MAX_TASK 100
#define MAX_THREAD 4

pthread_mutex_t sumLock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t *visLocks;

os_threadpool_t *tp;

os_graph_t *graph;
volatile int sum;

// Function to add a task to threadpool, trying again if tasklist is full
void processNode(void *nodeIdxVoid);

void add_node_task(unsigned int nodeIdx)
{
	os_task_t *task = task_create((void *) (long) nodeIdx, processNode);

	add_task_in_queue(tp, task);
}

void processNode(void *nodeIdxVoid)
{
	unsigned int nodeIdx = (unsigned int) (long) nodeIdxVoid;
	os_node_t *node = graph->nodes[nodeIdx];

	pthread_mutex_lock(&sumLock);
	sum += node->nodeInfo;
	pthread_mutex_unlock(&sumLock);

	// Add neighbours to task queue
	for (int i = 0; i < node->cNeighbours; i++) {
		unsigned int n = node->neighbours[i];

		pthread_mutex_lock(&visLocks[n]);
		if (graph->visited[n] == 0) {
			graph->visited[n] = 1;
			add_node_task(n);
		}
		pthread_mutex_unlock(&visLocks[n]);
	}
	// Node process complete
	pthread_mutex_lock(&visLocks[nodeIdx]);
	graph->visited[nodeIdx] = 2;
	pthread_mutex_unlock(&visLocks[nodeIdx]);
}

// Check if entire graph was processsed and re-start traversal when it stops
int is_done(os_threadpool_t *_)
{
	for (int i = 0; i < graph->nCount; ++i) {
		if (graph->visited[i] != 2) {
			// If the graph is not connected, the traversal won't reach all nodes from node 0
			// If traversal ends (tasks becomes NULL), re-start the traversal from the first
			// un-visited node by creating a new task for it
			pthread_mutex_lock(&visLocks[i]);
			if (graph->visited[i] == 0) {
				graph->visited[i] = 1;
				add_node_task(i);
			}
			pthread_mutex_unlock(&visLocks[i]);
			return 0;
		}
	}
	return 1;
}

int main(int argc, char *argv[])
{
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

	// Create vislocks for nodes
	visLocks = malloc(sizeof(pthread_mutex_t) * graph->nCount);
	for (int i = 0; i < graph->nCount; ++i)
		pthread_mutex_init(&visLocks[i], NULL);

	tp = threadpool_create(MAX_TASK, MAX_THREAD);
	// is_done will add the first node to queue
	threadpool_stop(tp, is_done);

	// Destroy vislocks
	for (int i = 0; i < graph->nCount; ++i)
		pthread_mutex_destroy(&visLocks[i]);
	free(visLocks);

	printf("%d", sum);
	return 0;
}
