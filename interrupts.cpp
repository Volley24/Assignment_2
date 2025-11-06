/**
 *
 * @file interrupts.cpp
 * @author Sasisekhar Govind
 *
 */

#include <interrupts.hpp>
#include <string.h>

using namespace std;

std::tuple<std::string, std::string, int> simulate_trace(std::vector<std::string> trace_file, int time, std::vector<std::string> vectors, std::vector<int> delays, std::vector<external_file> external_files, PCB current, std::vector<PCB> wait_queue) {

    std::string trace;      //!< string to store single line of trace file
    std::string execution = "";  //!< string to accumulate the execution output
    std::string system_status = "";  //!< string to accumulate the system status output
    int current_time = time;

	// Utility function to avoid having to manually add a line to the execution and set the time
    // This appends the current timestamp, duration of the operation, and output to the execution string and it increments the global duration time
	auto print = [&](string output, int duration) {
		execution += std::to_string(current_time) + ", " + std::to_string(duration) + ", " + output + "\n";
		current_time += duration;
		cout << "add " << duration << endl;
	};

	auto printSystemStatus = [&]() {
		system_status += "Time: " + to_string(current_time) + "; Current Trace: " + trace + "\n";
		system_status += print_PCB(current, wait_queue) + "\n";
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

			printSystemStatus();

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

			// probably just call simulate_trace 
			// with the child's trace (code above does that for us)
			// copied PCB
			// time, vectors, and a bunch of other stuff.

			/*
				std::vector<std::string> trace_file = child_trace
				int time = currentTime
				std::vector<std::string> vectors = ???
				std::vector<int> delays = ???
				std::vector<external_file> external_files = ???
				PCB current = clone(parent_PCB)
				std::vector<PCB> wait_queue = ???
	
			*/

			// clone the PCB

			print("cloning the PCB", duration_intr);

			// PID, PPID, programName, size, partition_number

			// Clone the PCB
    		PCB cloned_PCB(1, 2, program_name, 1, -1);
			allocate_memory(&cloned_PCB);

			print("scheduler called", 0);

			print("IERT", 1);


			// TODO: Need to fix wait_queue, since it now has nothing.
			auto [child_execution, child_system_status, child_time] = simulate_trace(child_trace, current_time, vectors, delays, external_files, cloned_PCB, wait_queue);
			execution += child_execution;			
			current_time = child_time;
			system_status += child_system_status;

            ///////////////////////////////////////////////////////////////////////////////////////////


        } else if(activity == "EXEC") {
            auto [intr, time] = intr_boilerplate(current_time, 3, 10, vectors);
            current_time = time;
            execution += intr;

            ///////////////////////////////////////////////////////////////////////////////////////////
            //Add your EXEC output here

			printSystemStatus();

            ///////////////////////////////////////////////////////////////////////////////////////////


            std::ifstream exec_trace_file(program_name + ".txt");

            std::vector<std::string> exec_traces;
            std::string exec_trace;
            while(std::getline(exec_trace_file, exec_trace)) {
                exec_traces.push_back(exec_trace);
            }


            ///////////////////////////////////////////////////////////////////////////////////////////
            //With the exec's trace (i.e. trace of external program), run the exec (HINT: think recursion)

			// find corresponding external_file
			// in order to find file.size
			external_file file;
			for (external_file external_file : external_files) {
				if (external_file.program_name == program_name) {
					file = external_file;
				}
			}

			print("Program '" + program_name +  "' is " + to_string(file.size) + " MB large", duration_intr);

			print("loading program into memory", file.size * 15);
			print("marking partition as occupied", 3);
			print("updating PCB", 6);

			print("scheduler called", 0);
			print("IERT", 1);

			// New PCB for EXEC'd program
			PCB new_pcb(1, 2, program_name, 1, -1);
			allocate_memory(&new_pcb);

			// TODO: Need to fix wait_queue, since it now has nothing.
			auto [exec_execution, exec_system_status, exec_time] =  simulate_trace(exec_traces, current_time, vectors, delays, external_files, new_pcb, wait_queue);
			execution += exec_execution;			
			current_time = exec_time;
			system_status += exec_system_status;

            ///////////////////////////////////////////////////////////////////////////////////////////

            break; //Why is this important? (answer in report)
			// well, beacuse we don't want to execute next steps. We are SWAPPING PCB / TEXT for the EXEC program.

        }
    }

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

	// TODO - Not sure what is meant by needing to implement this function, it is given in interrupts.hpp
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
