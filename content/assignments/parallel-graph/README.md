# Parallel Graph

For this assignment we will implement a generic thread pool, which we will then use to traverse a graph and compute the sum of the elements contained by the nodes.
You will be provided with a serial implementation of the graph traversal and with most of the data structures needed to implement the thread pool.
Your job is to write the thread pool rutines and then use the thread pool to traverse the graph.

## Thread Pool Description

A thread pool contains a given number of active threads that simply wait to be given specific tasks.
The threads are created when the thread pool is created they poll a task queue until a task is available.
Once a tasks are put in the task queue, the threads start running the task.
A thread pool creates N threads when the thread pool is created and does not destroy (join) them through out the life time of the thread pool.
That way, the penalty of creating and destroying threads ad hoc is avoided.
As such, you must implement the following functions (marked with `TODO` in the provided skeleton):

- `task_create` : creates an `os_task_t` that will be put in the task queue - a task consists of a function pointer and an argument.
- `add_task_in_queue` : adds a given task in the thread pool's task queue.
- `get_task` : get a task from the thread pool's task queue.
- `threadpool_create` : allocate and initialize a new thread pool.
- `thread_loop_function` : all the threads in the thread pool will execute this function - they all wait until a task is available in the task queue; once they grab a task they simply invoke the function that was provided to `task_create`.
- `threadpool_stop` : stop all the threads from execution.

Notice that the thread pool is completely independent from any given application.
Any function can be registered in the task queue.

## Graph Traversal

Once you have implemented the thread pool, you need to test it by using it for computing the sum of all the nodesof a graph.
A serial implementation for this algorithm is provided in `skep/serial.c`
To make use of the thread pool, you will need to create tasks that will be put in the task queue.
A task consists of 2 steps:

- adding the current node value to the overall sum.
- creating tasks and adding them to the task queue for the neighbouring nodes.

Since the threads are polling the task queue indefinitely, you need to find a condition for the threads to stop once the graph has been traversed completely.
This condition should be implemented in a function that is passed to `threadpool_stop`.
`threadpool_stop` then needs to wait for the condition to be satisfied and then joins all the threads.

## Synchronization

For synchronization you can use mutexes, semaphores, spinlocks, condition variables - anything that grinds your gear.
However, you are not allowed to use hacks such as `sleep`, `printf` synchronization or adding superflous computation.

## Input Files

Reading the graphs from the input files is being taken care of the functions implemetend in `skel/os_graph.c`.
A graph is represented in input files as follows:

- first line contains 2 integers N and M: N - number of nodes, M - numbed or edges
- second line contains N integer numbers - the values of the nodes
- the next M lines contain each 2 integers that represent the source and the destination of an edge

## Data Structures

### Graph

A graph is represented internally as an `os_graph_t` (see `skel/os_graph.h`).

### List

A list is represented internally as an `os_queue_t` (see `skel/os_list.h`).
You will use this list to implement the task queue.

### Thread pool

A thread pool is represented internally as an `os_threadpool_t` (see `skel/os_threadpool.h`)
The thread pool contains information about the task queue and the threads.

You are not allowed to modify these data structures.
However, you can create other data structures that leverage these ones.

## Infrastructure

### Compilation

To compile both the serial and the parallel version:

```sh
make
```

That will create the `serial` and `parallel` binaries

### Running a test

If you want manually run a single test:

```bash
./parallel tests/test5.in
```

### Checker

To run the checker that will be used to grade your homework, run:

```bash
python checker.py
```

## Grading

The grade that the checker outputs is not the final grade.
Your homework will be manually inspected and may suffer from penalties ranging from 1 to 100 points depending on the severity of the hack, including, but not limited to:

- using a single mutex at the beginning of the traversal
- not using the thread pool to solve the homework
- inefficient usage of synchronization
- incorrect graph traversal

## Deployment

Your implementation needs to be contained in the `skel/os_threadpool.c` and `skel/os_parallel.c` files.
Any other files that you are using will not be taken into account.
Any modifications that you are doing to the other files in the `skel` directory will not be taken into account.
