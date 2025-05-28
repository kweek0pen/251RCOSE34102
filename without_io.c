#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define MAX 10
#define TIME_QUANTUM 2
#define MAX_QUEUE 100

typedef struct {
    int pid;
    int arrival_time;
    int burst_time;
    int remaining_time;
    int priority;
    int waiting_time;
    int turnaround_time;
    int start_time;
    int finish_time;
    int done;
} Process;

void evaluate(const char* name);

Process processes[MAX];
int n = 5; //number of processes
char gantt[2000]; // Gantt Chart buffer
int gidx = 0;

typedef struct{
    int items[MAX_QUEUE];
    int front, rear;
} Queue;

void init_queue(Queue* q) {
    q->front = q->rear = -1;
}

int is_empty(Queue* q){ //check if queue is empty or not
    return q->front == -1;
}

void enqueue(Queue *q, int pid){
    if (q->rear == MAX_QUEUE - 1) return;
    if (q->front == -1) q->front = 0; //처음 추가됨
    q->rear++;
    q->items[q->rear] = pid;
}

int dequeue(Queue* q) {
    if (is_empty(q)) return -1;
    int pid = q->items[q->front]; //큐니까 제일 앞에서 꺼냄
    if (q->front == q->rear) { //only one item in queue
        q->front = -1; //initialize queue
        q->rear = -1;
    } else{
        q->front++;
    }
    return pid;
}

// Reset Gantt buffer
void reset_gantt() {
    gidx = 0;
}

// Append IDLE to Gantt buffer
void log_gantt(const char* label) {
        gidx += sprintf(&gantt[gidx], "|%-5s ", label);  // process being run
}

//Append Process ID to Gantt buffer
void log_gantt_pid(int pid){
    char label[8];
    sprintf(label, "P%d", pid);
    gidx += sprintf(&gantt[gidx], "| %-5s", label);
}

// Print Gantt Chart
void print_gantt_chart(const char* name, int total_time) {
    printf("--------------------------------\n");
    printf("\n[Gantt Chart - %s]\n", name);
    printf("%s|\n", gantt);
    for (int t = 0; t <= total_time; t++) printf("%-7d", t);
    printf("\n");
}

// Random process generator(creates n processes)
void create_processes() {
    srand(time(NULL));
    for (int i = 0; i < n; i++) {
        processes[i].pid = i + 1;
        processes[i].arrival_time = rand() % 5;
        processes[i].burst_time = 6 + rand() % 5;
        processes[i].remaining_time = processes[i].burst_time;
        processes[i].priority = rand() % 10;
        processes[i].waiting_time = 0;
        processes[i].turnaround_time = 0;
        processes[i].start_time = -1;
        processes[i].finish_time = 0;
        processes[i].done = 0;
    }
}

// Print process info
void print_processes() {
    printf("PID\tArrival\tBurst\tPriority\n");
    for (int i = 0; i < n; i++) {
        printf("%d\t%d\t%d\t%d\n",
               processes[i].pid,
               processes[i].arrival_time,
               processes[i].burst_time,
               processes[i].priority);
    }
}

// Reset state before each scheduling
void reset_processes() {
    for (int i = 0; i < n; i++) {
        processes[i].remaining_time = processes[i].burst_time;
        processes[i].waiting_time = 0;
        processes[i].turnaround_time = 0;
        processes[i].start_time = -1;
        processes[i].finish_time = 0;
        processes[i].done = 0;
    }
}

// FCFS Scheduler
void schedule_fcfs() {
    reset_processes();
    reset_gantt();
    int time = 0;
    int completed = 0;
    Queue ready;
    init_queue(&ready);

    // Keep track of the actual process index (0 to n-1)
    int current_process_idx = -1; 

    while (completed < n) {
        // Add new arrivals to the ready queue
        for (int i = 0; i < n; i++) {
            if (processes[i].arrival_time == time && !processes[i].done) {
                enqueue(&ready, i);
            }
        }

        // If no process is currently running, pick one from the ready queue
        if (current_process_idx == -1 && !is_empty(&ready)) {
            current_process_idx = dequeue(&ready);
            // If the process is picked for the first time, record its start time
            if (processes[current_process_idx].start_time == -1) {
                processes[current_process_idx].start_time = time;
            }
        }

        // Execute the current process for one time unit
        if (current_process_idx != -1) {
            Process* p = &processes[current_process_idx];
            log_gantt_pid(p->pid);
            p->remaining_time--;
            time++;

            // If the current process finishes
            if (p->remaining_time == 0) {
                p->done = 1;
                p->finish_time = time;
                p->turnaround_time = p->finish_time - p->arrival_time;
                p->waiting_time = p->turnaround_time - p->burst_time;
                completed++;
                current_process_idx = -1; // No process running, pick new one in next cycle
            }
        } else { // No process in ready queue, CPU is idle
            log_gantt("IDLE");
            time++;
        }
    }

    print_gantt_chart("FCFS", time);
    evaluate("FCFS");
}


// Non-preemptive SJF
void schedule_sjf_np() {
    reset_processes();
    reset_gantt();
    int time = 0;
    int completed = 0;
    Queue ready;
    init_queue(&ready);

    while (completed < n) {
        // Add new arrivals to ready queue
        for (int i = 0; i < n; i++) {
            if (processes[i].arrival_time == time && !processes[i].done) {
                enqueue(&ready, i);
            }
        }

        // Select shortest job from ready queue
        int idx = -1;
        int shortest_burst = -1;
        for (int i = ready.front; i <= ready.rear; i++) {
            int current_pid_in_queue = ready.items[i];
            if (!processes[current_pid_in_queue].done) {
                if (idx == -1 || processes[current_pid_in_queue].burst_time < shortest_burst) {
                    shortest_burst = processes[current_pid_in_queue].burst_time;
                    idx = current_pid_in_queue;
                }
            }
        }

        if (idx != -1) {
            // Remove from queue (this is an inefficient way to remove, but matches existing pattern)
            for (int i = ready.front; i <= ready.rear; i++) {
                if (ready.items[i] == idx) {
                    for (int j = i; j < ready.rear; j++) {
                        ready.items[j] = ready.items[j + 1];
                    }
                    ready.rear--;
                    if (ready.front > ready.rear) { // If queue becomes empty
                        ready.front = -1;
                    }
                    break;
                }
            }

            Process* p = &processes[idx];
            if (p->start_time == -1) p->start_time = time;

            // Execute the entire burst time (non-preemptive)
            while (p->remaining_time > 0) {
                p->remaining_time--;
                log_gantt_pid(p->pid);
                time++;

                // Add new arrivals during execution
                for (int i = 0; i < n; i++) {
                    if (processes[i].arrival_time == time && !processes[i].done) {
                        enqueue(&ready, i);
                    }
                }
            }

            if (p->remaining_time == 0) {
                p->done = 1;
                p->finish_time = time;
                p->turnaround_time = p->finish_time - p->arrival_time;
                p->waiting_time = p->turnaround_time - p->burst_time;
                completed++;
            }
        } else { // no process to run - idle
            log_gantt("IDLE");
            time++;
        }
    }

    print_gantt_chart("SJF (Non-Preemptive)", time);
    evaluate("SJF (Non-Preemptive)");
}

// Preemptive SJF
void schedule_sjf_p() {
    reset_processes();
    reset_gantt();
    int time = 0;
    int completed = 0;
    Queue ready;
    init_queue(&ready);

    while (completed < n) {
        // New arrivals
        for (int i = 0; i < n; i++) {
            if (processes[i].arrival_time == time && !processes[i].done) {
                enqueue(&ready, i);
            }
        }

        // Choose process with shortest remaining time
        int idx = -1;
        int shortest_remaining_time = -1;
        for (int i = ready.front; i <= ready.rear; i++) {
            int current_pid_in_queue = ready.items[i];
            if (!processes[current_pid_in_queue].done) {
                if (idx == -1 || processes[current_pid_in_queue].remaining_time < shortest_remaining_time) {
                    shortest_remaining_time = processes[current_pid_in_queue].remaining_time;
                    idx = current_pid_in_queue;
                }
            }
        }

        if (idx != -1) {
            Process* p = &processes[idx];
            if (p->start_time == -1) p->start_time = time;

            // Execute for 1 time unit
            p->remaining_time--;
            log_gantt_pid(p->pid);
            time++;

            if (p->remaining_time == 0) {
                p->done = 1;
                p->finish_time = time;
                p->turnaround_time = p->finish_time - p->arrival_time;
                p->waiting_time = p->turnaround_time - p->burst_time;
                completed++;
            }
            // If not done, it remains implicitly in the "ready" consideration for the next time unit
            // as it's a preemptive algorithm. No explicit re-enqueue needed unless it was removed.
            // Given the way `idx` is selected each cycle, this works.
        } else { // no process to run - idle
            log_gantt("IDLE");
            time++;
        }
    }

    print_gantt_chart("SJF (Preemptive)", time);
    evaluate("SJF (Preemptive)");
}

// Non-preemptive Priority
void schedule_priority_np() {
    reset_processes();
    reset_gantt();
    int time = 0;
    int completed = 0;
    Queue ready;
    init_queue(&ready);

    while (completed < n) {
        // New arrivals
        for (int i = 0; i < n; i++) {
            if (processes[i].arrival_time == time && !processes[i].done) {
                enqueue(&ready, i);
            }
        }

        // Select highest priority (lowest priority value)
        int idx = -1;
        int highest_priority = -1;
        for (int i = ready.front; i <= ready.rear; i++) {
            int current_pid_in_queue = ready.items[i];
            if (!processes[current_pid_in_queue].done) {
                if (idx == -1 || processes[current_pid_in_queue].priority < highest_priority) {
                    highest_priority = processes[current_pid_in_queue].priority;
                    idx = current_pid_in_queue;
                }
            }
        }

        // Remove from ready queue
        if (idx != -1) {
            for (int i = ready.front; i <= ready.rear; i++) {
                if (ready.items[i] == idx) {
                    for (int j = i; j < ready.rear; j++) {
                        ready.items[j] = ready.items[j + 1];
                    }
                    ready.rear--;
                    if (ready.front > ready.rear) { // If queue becomes empty
                        ready.front = -1;
                    }
                    break;
                }
            }

            Process* p = &processes[idx];
            if (p->start_time == -1) p->start_time = time;

            // Execute the entire burst time (non-preemptive)
            while (p->remaining_time > 0) {
                p->remaining_time--;
                log_gantt_pid(p->pid);
                time++;

                for (int i = 0; i < n; i++) {
                    if (processes[i].arrival_time == time && !processes[i].done) {
                        enqueue(&ready, i);
                    }
                }
            }

            if (p->remaining_time == 0) {
                p->done = 1;
                p->finish_time = time;
                p->turnaround_time = p->finish_time - p->arrival_time;
                p->waiting_time = p->turnaround_time - p->burst_time;
                completed++;
            }
        } else { // no process to run - idle
            log_gantt("IDLE");
            time++;
        }
    }

    print_gantt_chart("Priority (Non-Preemptive)", time);
    evaluate("Priority (Non-Preemptive)");
}

// Preemptive Priority
void schedule_priority_p() {
    reset_processes();
    reset_gantt();
    int time = 0;
    int completed = 0;
    Queue ready;
    init_queue(&ready);

    while (completed < n) {
        // New arrivals
        for (int i = 0; i < n; i++) {
            if (processes[i].arrival_time == time && !processes[i].done) {
                enqueue(&ready, i);
            }
        }

        // Select highest priority from ready (lowest priority value)
        int idx = -1;
        int highest_priority = -1;
        for (int i = ready.front; i <= ready.rear; i++) {
            int current_pid_in_queue = ready.items[i];
            if (!processes[current_pid_in_queue].done) {
                if (idx == -1 || processes[current_pid_in_queue].priority < highest_priority) {
                    highest_priority = processes[current_pid_in_queue].priority;
                    idx = current_pid_in_queue;
                }
            }
        }

        if (idx != -1) {
            Process* p = &processes[idx];
            if (p->start_time == -1) p->start_time = time;

            p->remaining_time--;
            log_gantt_pid(p->pid);
            time++;

            if (p->remaining_time == 0) {
                p->done = 1;
                p->finish_time = time;
                p->turnaround_time = p->finish_time - p->arrival_time;
                p->waiting_time = p->turnaround_time - p->burst_time;
                completed++;
            }
        } else { // no process to run - idle
            log_gantt("IDLE");
            time++;
        }
    }

    print_gantt_chart("Priority (Preemptive)", time);
    evaluate("Priority (Preemptive)");
}

// Round Robin
void schedule_rr() {
    reset_processes();
    reset_gantt();
    int time = 0;
    int completed = 0;
    Queue ready;
    init_queue(&ready);

    while (completed < n) {
        // Add new arrivals to ready queue
        for (int i = 0; i < n; i++) {
            if (processes[i].arrival_time == time && !processes[i].done) {
                enqueue(&ready, i);
            }
        }

        if (!is_empty(&ready)) {
            int idx = dequeue(&ready);
            Process* p = &processes[idx];
            if (p->start_time == -1) p->start_time = time;

            int slice = (p->remaining_time >= TIME_QUANTUM) ? TIME_QUANTUM : p->remaining_time;

            for (int i = 0; i < slice; i++) {
                p->remaining_time--;
                log_gantt_pid(p->pid);
                time++;

                // new arrivals during execution
                for (int j = 0; j < n; j++) {
                    if (processes[j].arrival_time == time && !processes[j].done) {
                        enqueue(&ready, j);
                    }
                }
                if (p->remaining_time == 0) break;
            }

            if (p->remaining_time == 0) {
                p->done = 1;
                p->finish_time = time;
                p->turnaround_time = p->finish_time - p->arrival_time;
                p->waiting_time = p->turnaround_time - p->burst_time;
                completed++;
            } else {
                enqueue(&ready, idx); // Return to ready if not done
            }
        } else { // no process to run - idle
            log_gantt("IDLE");
            time++;
        }
    }

    print_gantt_chart("Round Robin", time);
    evaluate("Round Robin");
}

// Print results and averages
void evaluate(const char* name) {
    printf("--------------------------------\n");
    printf("\n[%s Evaluation]\n", name);
    printf("PID\tWT\tTAT\n");
    
    float wt = 0, tat = 0;
    for (int i = 0; i < n; i++) {
        printf("%d\t%d\t%d\n", processes[i].pid,
               processes[i].waiting_time, processes[i].turnaround_time);
        wt += processes[i].waiting_time;
        tat += processes[i].turnaround_time;
    }

    printf("Average WT: %.2f\n", wt / n);
    printf("Average TAT: %.2f\n", tat / n);
}

int main() {
    int choice;
    do {
        create_processes();
        print_processes();

        printf("\nSelect Scheduling Algorithm:\n");
        printf("1. FCFS\n");
        printf("2. SJF (Non-Preemptive)\n");
        printf("3. SJF (Preemptive)\n");
        printf("4. Priority (Non-Preemptive)\n");
        printf("5. Priority (Preemptive)\n");
        printf("6. Round Robin\n");
        printf("Other number to exit.\n> ");
        scanf("%d", &choice);

        switch (choice) {
            case 1: schedule_fcfs(); break;
            case 2: schedule_sjf_np(); break;
            case 3: schedule_sjf_p(); break;
            case 4: schedule_priority_np(); break;
            case 5: schedule_priority_p(); break;
            case 6: schedule_rr(); break;
            default: printf("Exiting...\n"); return 0;
        }

        printf("\n===============================\n");

    } while (choice >= 1 && choice <= 6);

    return 0;
}