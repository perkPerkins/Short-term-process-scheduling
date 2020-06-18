#include <iostream>
#include <vector>
#include <unistd.h>

using namespace std;

struct process_control_block{
    char *name, status, priority;
    int32_t id, burst, base_reg;
    long long limit_reg;
};

long get_total_mem(const vector<process_control_block>& processes) {
    long mem_count = 0;
    for(int i = 0; i < processes.size(); i++) {
        mem_count += (processes[i].limit_reg - processes[i].base_reg);
    }
    return mem_count;
}

vector<process_control_block> read_binary(FILE * file_pointer) {
    char * buffer;
    int size;
    vector<process_control_block> PCB;

    fseek(file_pointer, 0L, SEEK_END);
    size = (int)ftell(file_pointer);
    fseek(file_pointer, 0L, SEEK_SET);
    buffer = new char[4];
    int counter = 0;

    while(ftell(file_pointer) != size) {
        PCB.push_back(process_control_block());

        fread(buffer, 16, 1, file_pointer);
        PCB[counter].name = buffer;
        printf("Process name: %s\n", PCB[counter].name);

        fread(&PCB[counter].id, sizeof(PCB[counter].id), 1, file_pointer);
        printf("Process ID: %d\n", PCB[counter].id);

        fread(&PCB[counter].status, sizeof(PCB[counter].status), 1, file_pointer);
        printf("Process status: %d\n", PCB[counter].status);

        fread(&PCB[counter].burst, sizeof(PCB[counter].burst), 1, file_pointer);
        printf("Process burst time: %d\n", PCB[counter].burst);

        fread(&PCB[counter].base_reg, sizeof(PCB[counter].base_reg), 1, file_pointer);
        printf("Process base register: %d\n", PCB[counter].base_reg);

        fread(&PCB[counter].limit_reg, sizeof(PCB[counter].limit_reg), 1, file_pointer);
        printf("Process limit register: %lld\n", PCB[counter].limit_reg);

        fread(&PCB[counter].priority, sizeof(PCB[counter].priority), 1, file_pointer);
        printf("Process priority: %d\n", PCB[counter].priority);

        counter++;
        cout << endl;
    }
    fclose(file_pointer);
    cout << "Processes available: " << counter <<  endl;
    printf("Total memory allocated by the processes: %ld\n\n", get_total_mem(PCB));
    return PCB;
}

void read_process(int counter, vector<process_control_block> PCB, FILE * file_pointer) {
    cout << "\nUpdated process after 1 quantum burst:\n";
//    FILE * file_pointer;
    file_pointer = fopen("processes.bin", "rb+");

    fseek(file_pointer, 38 * counter, SEEK_SET);
    char * buffer;
    buffer = new char[4];

    fread(buffer, 16, 1, file_pointer);
    PCB[counter].name = buffer;
    printf("Process name: %s\n", PCB[counter].name);

    fread(&PCB[counter].id, sizeof(PCB[counter].id), 1, file_pointer);
    printf("Process ID: %d\n", PCB[counter].id);

    fread(&PCB[counter].status, sizeof(PCB[counter].status), 1, file_pointer);
    printf("Process status: %d\n", PCB[counter].status);

    fread(&PCB[counter].burst, sizeof(PCB[counter].burst), 1, file_pointer);
    printf("Process burst time: %d\n", PCB[counter].burst);

    fread(&PCB[counter].base_reg, sizeof(PCB[counter].base_reg), 1, file_pointer);
    printf("Process base register: %d\n", PCB[counter].base_reg);

    fread(&PCB[counter].limit_reg, sizeof(PCB[counter].limit_reg), 1, file_pointer);
    printf("Process limit register: %lld\n", PCB[counter].limit_reg);

    fread(&PCB[counter].priority, sizeof(PCB[counter].priority), 1, file_pointer);
    printf("Process priority: %d\n", PCB[counter].priority);
}

void write_to_file(int counter, vector<process_control_block> PCB, FILE * write_pointer) {
//    FILE * write_pointer;
    write_pointer = fopen("processes.bin", "rb+");

    fseek(write_pointer, (38 * counter) + 20, SEEK_SET); // Each process takes 38 bytes of the file. Multiply that by index of current process
    fwrite(&PCB[counter].status, sizeof(PCB[counter].status), 1, write_pointer);

    fwrite(&PCB[counter].burst, sizeof(PCB[counter].burst), 1, write_pointer);
    rewind(write_pointer);

    fseek(write_pointer, (38 * counter) + 37, SEEK_SET);
    fwrite(&PCB[counter].priority, sizeof(PCB[counter].priority), 1, write_pointer);
    fclose(write_pointer);
}

bool check_if_finished(vector<process_control_block> PCB) {
    for(int i = 0; i < PCB.size(); i++) {
        if(int(PCB[i].burst) != 0) {
            return false;
        }
    }
    return true;
}

vector<process_control_block> sort_processes(vector<process_control_block> PCB) {
    int i, j;
    struct process_control_block key;
    for(i = 1; i < PCB.size(); i++) {
        key = PCB[i];
        j = i - 1;
        while(j >= 0 and PCB[j].priority > key.priority) {
            PCB[j + 1] = PCB[j];
            j--;
        }
        PCB[j + 1] = key;

    }
    return PCB;
}

int find_vector_index(int proc_id, vector<process_control_block> PCB) {
    for(int i = 0; i < PCB.size(); i++) {
        if(PCB[i].id == proc_id) {
            return i;
        }
    }
    return -1;
}

vector<process_control_block> apply_aging(vector<process_control_block> sorted_processes) {
    for(int i = 0; i < sorted_processes.size(); i++) {
        if(sorted_processes[i].priority > 0) {
            sorted_processes[i].priority--;
        }
    }
    sorted_processes = sort_processes(sorted_processes);
    return sorted_processes;
}

void execute_processes(vector<process_control_block> PCB, FILE * rp, FILE * wp) {
    bool all_proc_finished = false, priority_execution = false;
    int seconds_passed, vector_index = 0;
    cout << "System is currently using round robin scheduling...\n\n";
    while(!all_proc_finished) {
        if(!priority_execution) { //round robin
            seconds_passed = 0;
            while(seconds_passed < 30) {
                for (int i = 0; i < PCB.size(); i++) {
                    if (PCB[i].burst == 0) {
                        PCB[i].status = 0;
                        continue;
                    } else {
                        PCB[i].burst--;
                        write_to_file(i, PCB, wp);
                        read_process(i, PCB, rp);
                        seconds_passed++;
                        //sleep(1);
                    }
                }
                all_proc_finished = check_if_finished(PCB);
                if(check_if_finished(PCB)) {
                    cout << "all processes finished" << endl;
                    seconds_passed = 30; //this is only done to prevent an infinite loop when all the processes finish before the
                    //30 second time quantum is over.
                }
            }
            cout << "Switching to priority scheduling...\n\n";
            priority_execution = true;
        }
        else{ // priority scheduling
            seconds_passed = 0;
            vector<process_control_block> sorted_processes = sort_processes(PCB);
            while(seconds_passed < 30) {
                for(int i = 0; i < sorted_processes.size(); i++) {
                    if(sorted_processes[i].burst == 0) {
                        sorted_processes[i].status = 0;
                        continue;
                    }
                    else {
                        sorted_processes[i].burst--;
                        vector_index = find_vector_index(sorted_processes[i].id, PCB);
                        PCB[vector_index] = sorted_processes[i];// this could be the problem?
                        write_to_file(vector_index, PCB, wp);
                        read_process(vector_index, PCB, rp);
                        seconds_passed++;

                        if(i % 2 == 1) {
                            sorted_processes = apply_aging(sorted_processes);
                        }
                        //sleep(1);
                    }
                }
                all_proc_finished = check_if_finished(PCB);
                if(check_if_finished(PCB)) {
                    cout << "all processes finished" << endl;
                    seconds_passed = 30;
                }
            }
            cout << "Switching to round robin scheduling...\n\n";
            priority_execution = false;
        }
    }
    for(int i = 0; i < PCB.size(); i++) {
        read_process(i, PCB, rp);
        cout << endl;
    }
    cout << "Processes available: 0" <<  endl; //writing .status to file never worked for whatever reason
    //otherwise I would just make a for loop and count how many processes have 1 in PCB.available (which is obviously 0)
    printf("Total memory allocated by the processes: %ld\n\n", get_total_mem(PCB));
}

int main(int argc, const char *argv[]) {
    if(argc == 1) {
        cout << "Please provide a bin file.\n";
        exit(1);
    }
    else if(argc > 2) {
        cout << "Too many arguments\n";
        exit(1);
    }

    FILE * rp;
    //rp = fopen("processes.bin", "rb");
    rp = fopen(argv[1], "rb+");
    FILE * wp;
    wp = fopen(argv[1], "rb+");

    if(rp == nullptr) {
        perror("Please at input file to program directory, or provide the full path to the file. Error");
        exit(1);
    }
    vector<process_control_block> PCB;
    cout << "Processes at the beginning of the program: \n";
    PCB = read_binary(rp);
    execute_processes(PCB, rp, wp);

    return 0;
}
