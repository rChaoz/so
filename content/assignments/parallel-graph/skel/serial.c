#include <stdio.h>
#include <stdlib.h>
#include "os_graph.h"

int sum;
os_graph_t *graph;

void processNode(unsigned int nodeIdx)
{
    os_node_t *node = graph->nodes[nodeIdx];
    sum += node->nodeInfo;
    for (int i = 0; i < node->cNeighbours; i++)
        if (graph->visited[node->neighbours[i]] == 0) {
            graph->visited[node->neighbours[i]] = 1;
            processNode(node->neighbours[i]);
        }
}

void traverse_graph()
{
    for (int i = 0; i < graph->nCount; i++)
    {
        if (graph->visited[i] == 0) {
            graph->visited[i] = 1;
            processNode(i);
        }
    }
}

int main(int argc, char *argv[])
{
    if (argc != 2) {
        printf("Usage: ./main input_file\n");
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

    traverse_graph();
    printf("%d", sum);
    return 0;
}
