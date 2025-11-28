/**
 * @file interrupts.cpp
 * @author Sasisekhar Govind
 * @brief template main.cpp file for Assignment 3 Part 1 of SYSC4001
 * 
 */

#include "interrupts_student1_student2.hpp"
static const unsigned int quantum = 100;

void FCFS(std::vector<PCB> &ready_queue) {
    std::sort( 
                ready_queue.begin(),
                ready_queue.end(),
                []( const PCB &first, const PCB &second ){
                    return (first.arrival_time > second.arrival_time); 
                } 
            );
}

struct IO {
    PCB process;
    unsigned int io;
};

std::tuple<std::string /* add std::string for bonus mark */ > 
run_simulation(std::vector<PCB> list_processes) {

    std::vector<PCB> ready_queue;   //The ready queue of processes
    std::vector<IO>  wait_queue;    //The wait queue of processes
    std::vector<PCB> job_list;      //A list to keep track of all the processes. This is similar
                                    //to the "Process, Arrival time, Burst time" table that you
                                    //see in questions. You don't need to use it, I put it here
                                    //to make the code easier :).*/

    unsigned int current_time = 0;
    PCB running;

    unsigned int time_slice_used = 0;

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
        for(auto &process : list_processes) {
            if(process.arrival_time == current_time) {
                assign_memory(process);

                process.state = READY;  
                ready_queue.push_back(process);
                job_list.push_back(process);

                execution_status += print_exec_status(current_time, process.PID, NEW, READY);
            }
        }

        ///////////////////////MANAGE WAIT QUEUE/////////////////////////
        //This mainly involves keeping track of how long a process must remain in the wait queue
        for (auto it = wait_queue.begin(); it != wait_queue.end();) {

            it->io--;

            if (it->io == 0) {
                execution_status += print_exec_status(
                    current_time,
                    it->process.PID,
                    WAITING,
                    READY
                );

                it->process.state = READY;
                ready_queue.push_back(it->process);

                it = wait_queue.erase(it);
            }
            else {
                ++it;
            }
        }
        /////////////////////////////////////////////////////////////////

        //////////////////////////SCHEDULER//////////////////////////////
        if (running.state != RUNNING && !ready_queue.empty()) {

            FCFS(ready_queue);

            run_process(running, job_list, ready_queue, current_time);

            time_slice_used = 0;

            execution_status += print_exec_status(
                current_time,
                running.PID,
                READY,
                RUNNING
            );
        }
    
        if (running.state == RUNNING) {

            running.remaining_time--;
            time_slice_used++;

            sync_queue(job_list, running);

            unsigned executed = running.processing_time - running.remaining_time;
            bool needs_io = false;

            if (running.io_freq > 0 && executed > 0) {
                if (executed % running.io_freq == 0)
                    needs_io = true;
            }

            if (running.remaining_time == 0) {

                execution_status += print_exec_status(
                    current_time + 1,
                    running.PID,
                    RUNNING,
                    TERMINATED
                );

                terminate_process(running, job_list);
                idle_CPU(running);
            }
            else if (needs_io) {

                execution_status += print_exec_status(
                    current_time + 1,
                    running.PID,
                    RUNNING,
                    WAITING
                );

                running.state = WAITING;
                sync_queue(job_list, running);

                IO entry;
                entry.process = running;
                entry.io = running.io_duration;

                wait_queue.push_back(entry);

                idle_CPU(running);
                time_slice_used = 0;
            }
            else if (time_slice_used >= quantum) {

                execution_status += print_exec_status(
                    current_time + 1,
                    running.PID,
                    RUNNING,
                    READY
                );

                running.state = READY;
                ready_queue.push_back(running);

                idle_CPU(running);
                time_slice_used = 0;
            }
        }

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

    //Open input file
    auto file_name = argv[1];
    std::ifstream input_file;
    input_file.open(file_name);

    if (!input_file.is_open()) {
        std::cerr << "Error: Unable to open file: " << file_name << std::endl;
        return -1;
    }

    //Parse processes
    std::string line;
    std::vector<PCB> list_process;
    while(std::getline(input_file, line)) {
        auto input_tokens = split_delim(line, ", ");
        auto new_process = add_process(input_tokens);
        list_process.push_back(new_process);
    }
    input_file.close();

    auto [exec] = run_simulation(list_process);

    write_output(exec, "execution.txt");

    return 0;
}