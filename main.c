#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define MAX 10
#define TIME_QUANTUM 2
#define MAX_IO 5
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

    int num_io;
    int io_request_time[MAX_IO];
    int io_burst[MAX_IO];
    int current_cpu_time;
    int io_index;
    int io_remaining;
    int in_io; //flag for when in I/O
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

// Append I/O, IDLE to Gantt buffer
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
        processes[i].io_index = 0;
        processes[i].io_remaining = 0;
        processes[i].current_cpu_time = 0;
        processes[i].in_io = 0;

        processes[i].num_io = rand() % 3 + 1;
        for (int j = 0; j < processes[i].num_io; j++) {
            processes[i].io_request_time[j] = 1 + rand() % (processes[i].burst_time - 1);
            processes[i].io_burst[j] = 1 + rand() % 3;
        }
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
        processes[i].io_index = 0;
        processes[i].io_remaining = 0;
        processes[i].current_cpu_time = 0;
        processes[i].in_io = 0;
    }
}

// FCFS Scheduler
void schedule_fcfs() {
    reset_processes();
    reset_gantt();
    int time = 0;
    int completed = 0;
    Queue ready, wait;
    init_queue(&ready);
    init_queue(&wait);

    while (completed < n) {
        // Check for new arrivals and add current arrival to ready Queue
        for (int i = 0; i < n; i++) {
            if (processes[i].arrival_time == time && !processes[i].done && !processes[i].in_io) {
                enqueue(&ready, i);
            }
        }

        int io_logged = 0;
        // Process I/O wait
        for (int i = 0; i < n; i++) {
            if (processes[i].in_io) {
                if(!io_logged){
                    log_gantt("IO");
                    io_logged = 1;
                }
                processes[i].io_remaining--;
                if (processes[i].io_remaining == 0) {
                    processes[i].in_io = 0;
                    enqueue(&ready, i); //if finished, put process in ready queue
                }
            }
        }

        //dequeue the first arrival in ready queue
        if (!is_empty(&ready)) {
            int idx = dequeue(&ready);
            Process* p = &processes[idx];
            if (p->start_time == -1) p->start_time = time; //record start time

            //run process one time unit
            log_gantt_pid(p->pid);
            p->remaining_time--;
            p->current_cpu_time++;
            time++;

            // Check for I/O request
            if (p->io_index < p->num_io &&
                p->current_cpu_time == p->io_request_time[p->io_index]) {
                p->in_io = 1; //IO로 전환   
                p->io_remaining = p->io_burst[p->io_index];
                p->io_index++;
                continue;
            }

            // If finished
            if (p->remaining_time == 0) {
                p->done = 1;
                p->finish_time = time;
                p->turnaround_time = p->finish_time - p->arrival_time;
                p->waiting_time = p->turnaround_time - p->burst_time;
                completed++;
            } else {
                enqueue(&ready, idx); // Return to ready if not done
            }
        } else if(!io_logged){ // no process to run - idle
            log_gantt("IDLE");
            time++;
        } else{ //IO was ongoing
            time++;
        }
    }

    print_gantt_chart("FCFS with I/O", time);
    evaluate("FCFS with I/O");
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
            if (processes[i].arrival_time == time && !processes[i].done && !processes[i].in_io) {
                enqueue(&ready, i);
            }
        }

        int io_logged = 0;
        // Process I/O wait
        for (int i = 0; i < n; i++) {
            if (processes[i].in_io) {
                if(!io_logged){
                    log_gantt("IO");
                    io_logged = 1;
                }
                processes[i].io_remaining--;
                if (processes[i].io_remaining == 0) {
                    processes[i].in_io = 0;
                    enqueue(&ready, i);
                }
            }
        }

        // Select shortest job from ready queue
        int idx = -1;
        for (int i = ready.front; i <= ready.rear; i++) {
            int pid = ready.items[i];
            if (!processes[pid].done && !processes[pid].in_io) {
                if (idx == -1 || processes[pid].burst_time < processes[idx].burst_time) {
                    idx = pid;
                }
            }
        }

        
        if (idx != -1) {
             // Remove from queue
            for (int i = ready.front; i <= ready.rear; i++) {
                if (ready.items[i] == idx) {
                    for (int j = i; j < ready.rear; j++) {
                        ready.items[j] = ready.items[j + 1];
                    }
                    ready.rear--;
                    break;
                }
            }

            Process* p = &processes[idx];
            if (p->start_time == -1) p->start_time = time;

            //int run = 0;
            while (p->remaining_time > 0) {
                // I/O check
                if (p->io_index < p->num_io &&
                    p->current_cpu_time == p->io_request_time[p->io_index]) {
                    p->in_io = 1;
                    p->io_remaining = p->io_burst[p->io_index];
                    p->io_index++;
                    enqueue(&ready, idx); // return to queue
                    break;
                }

                p->remaining_time--;
                p->current_cpu_time++;
                log_gantt_pid(p->pid);
                time++;
                //run++;

                // Handle new arrivals during execution
                for (int i = 0; i < n; i++) {
                    if (processes[i].arrival_time == time && !processes[i].done && !processes[i].in_io) {
                        enqueue(&ready, i);
                    }
                }

                for (int i = 0; i < n; i++) {
                    if (processes[i].in_io) {
                        processes[i].io_remaining--;
                        if (processes[i].io_remaining == 0) {
                            processes[i].in_io = 0;
                            enqueue(&ready, i);
                        }
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
        } else if(!io_logged){
            log_gantt("IDLE");
            time++;
        } else{
            time++;
        }
    }

    print_gantt_chart("SJF (Non-Preemptive) with I/O", time);
    evaluate("SJF (Non-Preemptive) with I/O");
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
            if (processes[i].arrival_time == time && !processes[i].done && !processes[i].in_io) {
                enqueue(&ready, i);
            }
        }

        int io_logged = 0;
        // Process I/O wait
        for (int i = 0; i < n; i++) {
            if (processes[i].in_io) {
                if(!io_logged){
                    log_gantt("IO");
                    io_logged = 1;
                }
                processes[i].io_remaining--;
                if (processes[i].io_remaining == 0) {
                    processes[i].in_io = 0;
                    enqueue(&ready, i);
                }
            }
        }

        // Choose process with shortest remaining time
        int idx = -1;
        for (int i = ready.front; i <= ready.rear; i++) {
            int pid = ready.items[i];
            if (!processes[pid].done && !processes[pid].in_io) {
                if (idx == -1 || processes[pid].remaining_time < processes[idx].remaining_time) {
                    idx = pid;
                }
            }
        }

        if (idx != -1) {
            Process* p = &processes[idx];
            if (p->start_time == -1) p->start_time = time;

            // Execute for 1 time unit
            p->remaining_time--;
            p->current_cpu_time++;
            log_gantt_pid(p->pid);
            time++;

            // I/O check
            if (p->io_index < p->num_io &&
                p->current_cpu_time == p->io_request_time[p->io_index]) {
                p->in_io = 1;
                p->io_remaining = p->io_burst[p->io_index];
                p->io_index++;
            } else if (p->remaining_time == 0) {
                p->done = 1;
                p->finish_time = time;
                p->turnaround_time = p->finish_time - p->arrival_time;
                p->waiting_time = p->turnaround_time - p->burst_time;
                completed++;
            }
        } else if(!io_logged){
            log_gantt("IDLE");
            time++;
        } else{
            time++;
        }
    }

    print_gantt_chart("SJF (Preemptive) with I/O", time);
    evaluate("SJF (Preemptive) with I/O");
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
            if (processes[i].arrival_time == time && !processes[i].done && !processes[i].in_io) {
                enqueue(&ready, i);
            }
        }

        int io_logged = 0;
        // I/O processing
        for (int i = 0; i < n; i++) {
            if (processes[i].in_io) {
                if(!io_logged){
                    log_gantt("IO");
                    io_logged = 1;
                }
                processes[i].io_remaining--;
                if (processes[i].io_remaining == 0) {
                    processes[i].in_io = 0;
                    enqueue(&ready, i);
                }
            }
        }

        // Select highest priority
        int idx = -1;
        for (int i = ready.front; i <= ready.rear; i++) {
            int pid = ready.items[i];
            if (!processes[pid].done && !processes[pid].in_io) {
                if (idx == -1 || processes[pid].priority < processes[idx].priority) {
                    idx = pid;
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
                    break;
                }
            }

            Process* p = &processes[idx];
            if (p->start_time == -1) p->start_time = time;

            //int run = 0;
            while (p->remaining_time > 0) {
                // I/O request check
                if (p->io_index < p->num_io &&
                    p->current_cpu_time == p->io_request_time[p->io_index]) {
                    p->in_io = 1;
                    p->io_remaining = p->io_burst[p->io_index];
                    p->io_index++;
                    enqueue(&ready, idx);
                    break;
                }

                p->remaining_time--;
                p->current_cpu_time++;
                log_gantt_pid(p->pid);
                time++;
                //run++;

                for (int i = 0; i < n; i++) {
                    if (processes[i].arrival_time == time && !processes[i].done && !processes[i].in_io) {
                        enqueue(&ready, i);
                    }
                    if (processes[i].in_io) {
                        processes[i].io_remaining--;
                        if (processes[i].io_remaining == 0) {
                            processes[i].in_io = 0;
                            enqueue(&ready, i);
                        }
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
        } else if(!io_logged){
            log_gantt("IDLE");
            time++;
        } else{
            time++;
        }
    }

    print_gantt_chart("Priority (Non-Preemptive) with I/O", time);
    evaluate("Priority (Non-Preemptive) with I/O");
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
            if (processes[i].arrival_time == time && !processes[i].done && !processes[i].in_io) {
                enqueue(&ready, i);
            }
        }

        int io_logged = 0;
        // I/O processing
        for (int i = 0; i < n; i++) {
            if (processes[i].in_io) {
                
                processes[i].io_remaining--;
                if (processes[i].io_remaining == 0) {
                    processes[i].in_io = 0;
                    enqueue(&ready, i);
                }
            }
        }

        // Select highest priority from ready
        int idx = -1;
        for (int i = ready.front; i <= ready.rear; i++) {
            int pid = ready.items[i];
            if (!processes[pid].done && !processes[pid].in_io) {
                if (idx == -1 || processes[pid].priority < processes[idx].priority) {
                    idx = pid;
                }
            }
        }

        if (idx != -1) {
            Process* p = &processes[idx];
            if (p->start_time == -1) p->start_time = time;

            p->remaining_time--;
            p->current_cpu_time++;
            log_gantt_pid(p->pid);
            time++;

            // Check for I/O
            if (p->io_index < p->num_io &&
                p->current_cpu_time == p->io_request_time[p->io_index]) {
                p->in_io = 1;
                p->io_remaining = p->io_burst[p->io_index];
                p->io_index++;
            } else if (p->remaining_time == 0) {
                p->done = 1;
                p->finish_time = time;
                p->turnaround_time = p->finish_time - p->arrival_time;
                p->waiting_time = p->turnaround_time - p->burst_time;
                completed++;
            }
        } else if(!io_logged){
            log_gantt("IDLE");
            time++;
        } else{
            time++;
        }
    }

    print_gantt_chart("Priority (Preemptive) with I/O", time);
    evaluate("Priority (Preemptive) with I/O");
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
            if (processes[i].arrival_time == time && !processes[i].done && !processes[i].in_io) {
                enqueue(&ready, i);
            }
        }

        int io_logged = 0;
        for (int i = 0; i < n; i++) {
            if (processes[i].in_io) {
                if(!io_logged){
                    log_gantt("IO");
                    io_logged = 1;
                }
                processes[i].io_remaining--;
                if (processes[i].io_remaining == 0) {
                    processes[i].in_io = 0;
                    enqueue(&ready, i);
                }
            }
        }

        if (!is_empty(&ready)) {
            int idx = dequeue(&ready);
            Process* p = &processes[idx];
            if (p->start_time == -1) p->start_time = time;

            int slice = (p->remaining_time >= TIME_QUANTUM) ? TIME_QUANTUM : p->remaining_time;

            for (int i = 0; i < slice; i++) {
                // I/O interrupt check
                if (p->io_index < p->num_io &&
                    p->current_cpu_time == p->io_request_time[p->io_index]) {
                    p->in_io = 1;
                    p->io_remaining = p->io_burst[p->io_index];
                    p->io_index++;
                    break;
                }

                p->remaining_time--;
                p->current_cpu_time++;
                log_gantt_pid(p->pid);
                time++;

                // new arrivals during execution
                for (int j = 0; j < n; j++) {
                    if (processes[j].arrival_time == time && !processes[j].done && !processes[j].in_io) {
                        enqueue(&ready, j);
                    }
                }

                //IO
                for (int j = 0; j < n; j++) {
                    if (processes[j].in_io) {
                        processes[j].io_remaining--;
                        if (processes[j].io_remaining == 0) {
                            processes[j].in_io = 0;
                            enqueue(&ready, j);
                        }
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
            } else if (!p->in_io) {
                enqueue(&ready, idx);
            }

        } else if(!io_logged){
            log_gantt("IDLE");
            time++;
        } else{
            time++;
        }
    }

    print_gantt_chart("Round Robin with I/O", time);
    evaluate("Round Robin with I/O");
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