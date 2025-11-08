/**
 *
 * @file interrupts.cpp
 * @author Sasisekhar Govind
 *
 * @author Maxim Creanga 101298069
 */

#include <interrupts.hpp>
#include <string.h>
#include <algorithm> // Needed to delete a PCB from a vector, to clear the wait queue

using namespace std;

std::tuple<std::string, std::string, int> simulate_trace(std::vector<std::string> trace_file, int time, std::vector<std::string> vectors, std::vector<int> delays, std::vector<external_file> external_files, PCB current, std::vector<PCB> &wait_queue) {

    std::string trace;      //!< string to store single line of trace file
    std::string execution = "";  //!< string to accumulate the execution output
    std::string system_status = "";  //!< string to accumulate the system status output
    int current_time = time;

	// Utility function to avoid having to manually add a line to the execution and set the time
    // This appends the current timestamp, duration of the operation, and output to the execution string and it increments the global duration time
	auto print = [&](string output, int duration) {
		execution += std::to_string(current_time) + ", " + std::to_string(duration) + ", " + output + "\n";
		current_time += duration;
	};


    // Utility function to print out the current PCB and PCB wait queue to the system status
	auto printSystemStatus = [&](PCB &pcb, std::string &trace) {
		system_status += "Time: " + to_string(current_time) + "; Current Trace: " + trace + "\n";
		system_status += print_PCB(pcb, wait_queue) + "\n";
	};

    //parse each line of the input trace file. 'for' loop to keep track of indices.
    for(size_t i = 0; i < trace_file.size(); i++) {
        auto trace = trace_file[i];

        auto [activity, duration_intr, program_name] = parse_trace(trace);

        if(activity == "CPU") { //As per Assignment 1
            execution += std::to_string(current_time) + ", " + std::to_string(duration_intr) + ", CPU Burst\n";
            current_time += duration_intr;
        } else if(activity == "SYSCALL") { //As per Assignment 1
            auto [intr, time] = intr_boilerplate(current_time, duration_intr, 10, vectors);
            execution += intr;
            current_time = time;

            execution += std::to_string(current_time) + ", " + std::to_string(delays[duration_intr]) + ", SYSCALL ISR (ADD STEPS HERE)\n";
            current_time += delays[duration_intr];

            execution +=  std::to_string(current_time) + ", 1, IRET\n";
            current_time += 1;
        } else if(activity == "END_IO") {
            auto [intr, time] = intr_boilerplate(current_time, duration_intr, 10, vectors);
            current_time = time;
            execution += intr;

            execution += std::to_string(current_time) + ", " + std::to_string(delays[duration_intr]) + ", ENDIO ISR(ADD STEPS HERE)\n";
            current_time += delays[duration_intr];

            execution +=  std::to_string(current_time) + ", 1, IRET\n";
            current_time += 1;
        } else if(activity == "FORK") {
            auto [intr, time] = intr_boilerplate(current_time, 2, 10, vectors);
            execution += intr;
            current_time = time;

            ///////////////////////////////////////////////////////////////////////////////////////////
            //Add your FORK output here

            print("cloning the PCB", duration_intr);
            // Clone the PCB:
            // Make the PID + 1
            // The Parent PID should the be the current PID
            // The partition number is -1 as it will be allocated by the allocate_memory function
    		PCB cloned_PCB(current.PID + 1, current.PID, current.program_name, current.size, -1);
            if (!allocate_memory(&cloned_PCB)) {
                print("ERROR: Could not allocate memory partition for current program '" + current.program_name + "' with size " + to_string(current.size) + "MB. Aborting FORK instruction...", 0);
                continue;
            }

            print("scheduler called", 0);
			print("IERT", 1);

            ///////////////////////////////////////////////////////////////////////////////////////////

            //The following loop helps you do 2 things:
            // * Collect the trace of the child (and only the child, skip parent)
            // * Get the index of where the parent is supposed to start executing from
            std::vector<std::string> child_trace;
            bool skip = true;
            bool exec_flag = false;
            int parent_index = 0;

            for(size_t j = i; j < trace_file.size(); j++) {
                auto [_activity, _duration, _pn] = parse_trace(trace_file[j]);
                if(skip && _activity == "IF_CHILD") {
                    skip = false;
                    continue;
                } else if(_activity == "IF_PARENT"){
                    skip = true;
                    parent_index = j;
                    if(exec_flag) {
                        break;
                    }
                } else if(skip && _activity == "ENDIF") {
                    skip = false;
                    continue;
                } else if(!skip && _activity == "EXEC") {
                    skip = true;
                    child_trace.push_back(trace_file[j]);
                    exec_flag = true;
                }

                if(!skip) {
                    child_trace.push_back(trace_file[j]);
                }
            }
            i = parent_index;

            ///////////////////////////////////////////////////////////////////////////////////////////
            //With the child's trace, run the child (HINT: think recursion)


            // Add parent to the WAIT queue, as the CHILD has priority over the parent.
            wait_queue.push_back(current);

            printSystemStatus(cloned_PCB, trace);

            // Recursively call simulate_trace with the forked pcb.
            // The child will run first, running only the child trace,
            // And after it is done, the parent will execute as well.
            auto [child_execution, child_system_status, child_time] = simulate_trace(child_trace, current_time, vectors, delays, external_files, cloned_PCB, wait_queue);
            execution += child_execution;            
            current_time = child_time;
            system_status += child_system_status;

            // Free memory for child PCB after it finishes runing
            free_memory(&cloned_PCB);

            ///////////////////////////////////////////////////////////////////////////////////////////

        } else if(activity == "EXEC") {
            auto [intr, time] = intr_boilerplate(current_time, 3, 10, vectors);
            current_time = time;
            execution += intr;

            ///////////////////////////////////////////////////////////////////////////////////////////
            //Add your EXEC output here

            // Find the corresponding external file 
			// In order to determine the EXEC file.size
			external_file file;
			for (external_file external_file : external_files) {
				if (external_file.program_name == program_name) {
					file = external_file;
				}
			}

            print("Program '" + program_name +  "' is " + to_string(file.size) + " MB large", duration_intr);
            // The time is calculated as follows: Size of Program in MB * 15ms/MB
			print("loading program into memory", file.size * 15);
            

            print("marking partition as occupied", 3);
            PCB new_pcb(current.PID, -1, program_name, file.size, -1);
			if (!allocate_memory(&new_pcb)) {
                print("ERROR: Could not allocate memory partition for program '" + program_name + "' with size " + to_string(file.size) + "MB. Aborting EXEC instruction...", 0);
                continue;
            }
            
			print("updating PCB", 6);

            print("scheduler called", 0);
			print("IERT", 1);

            ///////////////////////////////////////////////////////////////////////////////////////////


            std::ifstream exec_trace_file(program_name + ".txt");

            std::vector<std::string> exec_traces;
            std::string exec_trace;
            while(std::getline(exec_trace_file, exec_trace)) {
                exec_traces.push_back(exec_trace);
            }


            ///////////////////////////////////////////////////////////////////////////////////////////
            //With the exec's trace (i.e. trace of external program), run the exec (HINT: think recursion)

            // Remove parent PCB from wait queue
            auto pcb = wait_queue.begin();
            while (pcb != wait_queue.end()) {
                if (pcb->PID == current.PID) {
                    pcb = wait_queue.erase(pcb);
                } else {
                    pcb ++;
                }
            }

            // Print the PCB + queue to the system status
            printSystemStatus(new_pcb, trace);

            // Recursively call simulate_trace with the new pcb
            auto [exec_execution, exec_system_status, exec_time] =  simulate_trace(exec_traces, current_time, vectors, delays, external_files, new_pcb, wait_queue);
            execution += exec_execution;            
            current_time = exec_time;
            system_status += exec_system_status;
            // Free memory for exec PCB after it finishes
            free_memory(&new_pcb);

            ///////////////////////////////////////////////////////////////////////////////////////////

            // Break so that the 'old' code doesn't get to run anymore
            // As EXEC replaces the code.
            break; //Why is this important? (answer in report)
        }
    }

    // Free memory for current PCB after finishing its trace
    free_memory(&current);
    return {execution, system_status, current_time};
}

int main(int argc, char** argv) {

    //vectors is a C++ std::vector of strings that contain the address of the ISR
    //delays  is a C++ std::vector of ints that contain the delays of each device
    //the index of these elemens is the device number, starting from 0
    //external_files is a C++ std::vector of the struct 'external_file'. Check the struct in 
    //interrupt.hpp to know more.
    auto [vectors, delays, external_files] = parse_args(argc, argv);
    std::ifstream input_file(argv[1]);

    //Just a sanity check to know what files you have
    print_external_files(external_files);

    //Make initial PCB (notice how partition is not assigned yet)
    PCB current(0, -1, "init", 1, -1);
    //Update memory (partition is assigned here, you must implement this function)
    if(!allocate_memory(&current)) {
        std::cerr << "ERROR! Memory allocation failed!" << std::endl;
    }

    std::vector<PCB> wait_queue;

    /******************ADD YOUR VARIABLES HERE*************************/


    /******************************************************************/

    //Converting the trace file into a vector of strings.
    std::vector<std::string> trace_file;
    std::string trace;
    while(std::getline(input_file, trace)) {
        trace_file.push_back(trace);
    }

    auto [execution, system_status, _] = simulate_trace(   trace_file, 
                                            0, 
                                            vectors, 
                                            delays,
                                            external_files, 
                                            current, 
                                            wait_queue);

    input_file.close();

    write_output(execution, "execution.txt");
    write_output(system_status, "system_status.txt");

    return 0;
}
