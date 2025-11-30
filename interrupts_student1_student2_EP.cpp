/**
 * @file interrupts.cpp
 * @author Sasisekhar Govind
 * @author Sarah Andrew, 101303468
 * @brief template main.cpp file for Assignment 3 Part 1 of SYSC4001
 * 
 */
#include "interrupts_student1_student2.hpp"

// This sorts by the high PID comes before the lower PID.
void EP(std::vector<PCB> &ready_queue) {
    std::sort(ready_queue.begin(), ready_queue.end(),
        [](const PCB &first, const PCB &second) {
            return (first.PID > second.PID);
        }
    );
}

// Keeps track of the IO events by storing the process and completion time.
struct IOEvent {
    PCB process;
    unsigned int io_completion;
};

std::tuple<std::string /* add std::string for bonus mark */ >
run_simulation(std::vector<PCB> list_processes) {

    std::vector<PCB> ready_queue;   //The ready queue of processes
    std::vector<IOEvent>  wait_queue;    //The wait queue of processes
    std::vector<PCB> job_list;      //A list to keep track of all the processes. This is similar
                                    //to the "Process, Arrival time, Burst time" table that you
                                    //see in questions. You don't need to use it, I put it here
                                    //to make the code easier :).

    unsigned int current_time = 0;
    PCB running;
    unsigned int last_io = 0;

    //Initialize an empty running process
    idle_CPU(running);

    std::string execution_status;

    //make the output table (the header row)
    execution_status = print_exec_header();


    //Loop while till there are no ready or waiting processes.
    //This is the main reason I have job_list, you don't have to use it.
    while(!all_process_terminated(job_list) || job_list.empty()) {

        //Inside this loop, there are three things you must do:
        // 1) Populate the ready queue with processes as they arrive
        // 2) Manage the wait queue
        // 3) Schedule processes from the ready queue

        //Population of ready queue is given to you as an example.
        //Go through the list of proceeses
        for(auto &process : list_processes) {
            if(process.arrival_time == current_time) {//check if the AT = current time
                //if so, assign memory and put the process into the ready queue
                assign_memory(process);

                process.state = READY;  //Set the process state to READY
                ready_queue.push_back(process); //Add the process to the ready queue
                job_list.push_back(process); //Add it to the list of processes

                execution_status += print_exec_status(current_time, process.PID, NEW, READY);
            }
        }

        ///////////////////////MANAGE WAIT QUEUE/////////////////////////
        //This mainly involves keeping track of how long a process must remain in the wait queue
        for (int i = 0; i < (int)wait_queue.size(); ) {
            if (wait_queue[i].io_completion == current_time) {
                execution_status += print_exec_status(current_time, wait_queue[i].process.PID, WAITING,
                                                    READY);
                
                // Updayes the process and places into the ready queue and then removes it from the wait queue.
                wait_queue[i].process.state = READY;
                ready_queue.push_back(wait_queue[i].process);
                wait_queue.erase(wait_queue.begin() + i);
            }
            else {
                i++;
            }
        }
        /////////////////////////////////////////////////////////////////

        //////////////////////////SCHEDULER//////////////////////////////
        // Checks to see if its running. When its running, it takes a part of the CPU,
        // sees the last I/O event and updates the job list
        if (running.state == RUNNING) {

            running.remaining_time--;
            last_io++;
            sync_queue(job_list, running);

            // Process terminating
            if (running.remaining_time <= 0) {

                execution_status += print_exec_status(current_time, running.PID,
                    RUNNING, TERMINATED);

                terminate_process(running, job_list);
                idle_CPU(running);
            }

            // IO occuring in the process, which creates a new IO event
            else if (running.io_freq > 0 && last_io >= running.io_freq) {

                execution_status += print_exec_status(current_time, running.PID,
                    RUNNING, WAITING);

                running.state = WAITING;
                sync_queue(job_list, running);

                IOEvent entry_io;
                entry_io.process = running;
                entry_io.io_completion = current_time + running.io_duration;

                wait_queue.push_back(entry_io);
                last_io = 0;
                idle_CPU(running);
            }
        }

        // Picks the next availiable processs
        if (running.state != RUNNING && !ready_queue.empty()) {

            EP(ready_queue);
            run_process(running, job_list, ready_queue, current_time);
            last_io = 0;

            // Displays status
            execution_status += print_exec_status(current_time, running.PID, READY,
                RUNNING);
        }
        // add the time
        current_time++;
    }
    //Close the output table
    execution_status += print_exec_footer();
    return std::make_tuple(execution_status);
}
int main(int argc, char** argv) {

    //Get the input file from the user
    if(argc != 2) {
        std::cout << "ERROR!\nExpected 1 argument, received " << argc - 1 << std::endl;
        std::cout << "To run the program, do: ./interrutps <your_input_file.txt>" << std::endl;
        return -1;
    }

    //Open the input file
    auto file_name = argv[1];
    std::ifstream input_file;
    input_file.open(file_name);

    //Ensure that the file actually opens
    if (!input_file.is_open()) {
        std::cerr << "Error: Unable to open file: " << file_name << std::endl;
        return -1;
    }

    //Parse the entire input file and populate a vector of PCBs.
    //To do so, the add_process() helper function is used (see include file).
    std::string line;
    std::vector<PCB> list_process;
    while(std::getline(input_file, line)) {
        auto input_tokens = split_delim(line, ", ");
        auto new_process = add_process(input_tokens);
        list_process.push_back(new_process);
    }
    input_file.close();

    //With the list of processes, run the simulation
    auto [exec] = run_simulation(list_process);

    write_output(exec, "execution.txt");

    return 0;
}
