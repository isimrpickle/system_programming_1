# Job Commander and Server

## Overview
This project consists of two main components: `jobCommander` and `Server`. The `jobCommander` acts as a client sending job requests to the `Server`, which manages job execution and concurrency control. Communication is handled via FIFOs and synchronized using signal handling and shared memory mechanisms.

## Compilation & Execution

To compile the project, simply run:
```sh
make
```
To clean up executable files, FIFOs, and temporary files:
```sh
make clean
```

To execute the programs:
```sh
./jobCommander <command>
./Server
```

## JobCommander

1. Starts by computing the size of the command line arguments.
2. Creates the first FIFO if it does not exist and checks for the required text file.
3. If the server is not running, it forks and executes it.
4. Allocates necessary memory to store and send the command-line arguments as a string.
5. Opens the FIFO and sends the message size and content to the server.
6. If the command is not `setConcurrency`, it reads the response from the second FIFO.
7. Cleans up and frees allocated memory before terminating.

## Server

The `Server` is designed to handle job execution and command processing through a structured queuing mechanism. It consists of:

- **Signal Handlers:** Two handlers, one for SIGUSR1 signals (sent by `jobCommander`) and one for SIGCHLD (to prevent zombie processes).
- **Global Variables:** Includes flags for synchronization, concurrency control, and process tracking.
- **Three Queues:**
  - `queue`: Stores jobs in an executable format.
  - `queuedProcesses`: Stores job metadata (formatted triplets `<jobID, job, queuePosition>`).
  - `activeProcesses`: Stores running jobs.

### Key Features

1. **IssueJob**
   - Parses and stores job requests.
   - Adds the job to the execution queue and metadata queue.
   - Sends formatted job information back to `jobCommander`.

2. **SetConcurrency**
   - Sets the maximum number of concurrent processes.

3. **Stop**
   - Searches for a job in `activeProcesses`.
   - If found, sends `SIGTERM` to terminate it.
   - If not found, searches `queuedProcesses` and removes it.

4. **Poll**
   - Iterates through both queues and retrieves job status.

5. **Exit**
   - Terminates the server while allowing running processes to complete.

### Execution Flow
- `Server` reads messages from `jobCommander` via FIFO.
- Parses commands and arguments.
- Executes jobs while maintaining concurrency constraints.
- Uses `fork` and `execvp` for job execution, tracking PIDs.
- Handles completed jobs using the SIGCHLD signal handler.

## Implementation Details
- **FIFO Communication:** Two FIFOs are used for bidirectional messaging.
- **Memory Management:** Proper allocation and deallocation to prevent leaks.
- **Synchronization:** Signal-based synchronization ensure proper communication.
- **Linked List Queues:** Used for job tracking and management.

## Assumptions
- When `exit` is called, running jobs continue until completion.
- Queue implementation is based on linked lists.


