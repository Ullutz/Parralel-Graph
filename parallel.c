// SPDX-License-Identifier: BSD-3-Clause

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <time.h>

#include "os_graph.h"
#include "os_threadpool.h"
#include "log/log.h"
#include "utils.h"

#define NUM_THREADS		4

static int sum;
static os_graph_t *graph;
static os_threadpool_t *tp;

static pthread_mutex_t graph_mutex = PTHREAD_MUTEX_INITIALIZER;

static void process_node(void *idx)
{
	int *value = (int *)idx;

	pthread_mutex_lock(&graph_mutex);
	sum += *value;
	pthread_mutex_unlock(&graph_mutex);
}

static void dfs(unsigned int idx)
{
	pthread_mutex_lock(&graph_mutex);
	graph->visited[idx] = DONE;
	pthread_mutex_unlock(&graph_mutex);

	os_node_t *node = graph->nodes[idx];

	for (unsigned int i = 0; i < node->num_neighbours; i++) {
		if (graph->visited[node->neighbours[i]] == NOT_VISITED)
			dfs(node->neighbours[i]);
	}

	pthread_mutex_lock(&graph_mutex);
	os_task_t *new = create_task(process_node, &node->info, NULL);

	enqueue_task(tp, new);
	pthread_mutex_unlock(&graph_mutex);
}

int main(int argc, char *argv[])
{
	FILE *input_file;

	if (argc != 2) {
		fprintf(stderr, "Usage: %s input_file\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	input_file = fopen(argv[1], "r");
	DIE(input_file == NULL, "fopen");

	graph = create_graph_from_file(input_file);

	if (!graph) {
		printf("error");
		return -1;
	}

	pthread_mutex_lock(&graph_mutex);
	tp = create_threadpool(NUM_THREADS);
	pthread_mutex_unlock(&graph_mutex);

	dfs(0);

	wait_for_completion(tp);

	pthread_mutex_lock(&graph_mutex);
	destroy_threadpool(tp);

	printf("%d", sum);
	pthread_mutex_unlock(&graph_mutex);

	pthread_mutex_destroy(&graph_mutex);

	return 0;
}
