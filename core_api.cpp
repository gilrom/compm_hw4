/* 046267 Computer Architecture - Spring 2020 - HW #4 */

#include "core_api.h"
#include "sim_api.h"

#include <stdio.h>


class Thread{
	public:
		tcontext context;
		bool isActive;
		Instruction current_inst; // thread itself doesnot tuch this
		uint32_t current_line_inst; // thread itself doesnot tuch this
		int last_exe_cycle; // thread itself doesnot tuch this
		bool last_comm_store, last_comm_load;
		Thread();
		void executeCommand();
};

class Core
{
	private:
		int currentCyc;
		int inst_ctr;
		int currentThread;
		int store_lat;
		int load_lat;
		int switch_cyc;
		int threads_number;
		Thread* threads;
		int threadRR (); //returns the number of next thread availble
	public:
		Core();
		void run_prog_fineGrained();
		void run_prog_blocked();
		tcontext getCTX(int threadid);
		double getCPI();
};

Thread::Thread():isActive(true),current_line_inst(0), last_exe_cycle(-1),
 last_comm_store(false), last_comm_load(false) {
	for (int i = 0; i < REGS_COUNT; i++)
	{
		context.reg[i] = 0;
	}
}

void Thread::executeCommand()
{
	last_comm_load = false;
	last_comm_store = false;
	uint32_t addr;
	switch (current_inst.opcode)
	{
	case CMD_NOP:
		/* code */
		break;
	
	case CMD_ADD:
		context.reg[current_inst.dst_index] = 
			context.reg[current_inst.src1_index] + context.reg[current_inst.src2_index_imm];
		break;

	case CMD_SUB:
		context.reg[current_inst.dst_index] = 
			context.reg[current_inst.src1_index] - context.reg[current_inst.src2_index_imm];
		break;
	
	case CMD_ADDI:
		context.reg[current_inst.dst_index] = 
			context.reg[current_inst.src1_index] + current_inst.src2_index_imm;
		break;
	
	case CMD_SUBI:
		context.reg[current_inst.dst_index] = 
			context.reg[current_inst.src1_index] - current_inst.src2_index_imm;
		break;
	
	case CMD_LOAD:
		if(current_inst.isSrc2Imm)
		{
			addr = context.reg[current_inst.src1_index] + current_inst.src2_index_imm;
		}
		else
		{
			addr = context.reg[current_inst.src1_index] + context.reg[current_inst.src2_index_imm];
		}
		SIM_MemDataRead(addr, &context.reg[current_inst.dst_index]);
		last_comm_load = true;
		break;
	
	case CMD_STORE:
		if(current_inst.isSrc2Imm)
		{
			addr = context.reg[current_inst.dst_index] + current_inst.src2_index_imm;
		}
		else
		{
			addr = context.reg[current_inst.dst_index] + context.reg[current_inst.src2_index_imm];
		}
		SIM_MemDataWrite(addr, context.reg[current_inst.src1_index]);
		last_comm_store = true;
		break;
	
	case CMD_HALT:
		isActive = false;
		break;
	
	default:
		break;
	}	
}

Core::Core() : 
	currentCyc(0), inst_ctr(0), currentThread(0)
{
	
	store_lat = SIM_GetStoreLat();
	load_lat = SIM_GetLoadLat();
	switch_cyc = SIM_GetSwitchCycles();
	threads_number = SIM_GetThreadsNum();
	threads = new Thread[threads_number];
	for(int i = 0; i < threads_number; i++)
	{
		SIM_MemInstRead(0, &threads[i].current_inst, i);
	}
	
}

int Core::threadRR()
{
	int ind = currentThread;
	for (int i = 0 ; i < threads_number ; i++)
	{
		ind = (ind) % threads_number;
		if(threads[ind].isActive)
		{
			if(threads[ind].last_exe_cycle == -1)//didn't run yet
			{
				return ind;
			}
			else if(threads[ind].last_comm_load)
			{
				if(currentCyc - threads[ind].last_exe_cycle > load_lat)
				{
					return ind;
				}
				else
				{
					ind++;
					continue;
				}
			}
			else if(threads[ind].last_comm_store)
			{
				if(currentCyc - threads[ind].last_exe_cycle > store_lat)
				{
					return ind;
				}
				else
				{
					ind++;
					continue;
				}
					
			}
			else //regular command
				return ind;
		}
		ind++;
	}
	return -1;//idle
}

void Core::run_prog_blocked()
{
	int active_threads = threads_number;
	while (active_threads)
	{
		int next_thread = threadRR();
		if(next_thread < 0)
		{
			currentCyc++;
			continue;
		}
		if(next_thread != currentThread)
		{
			currentThread = next_thread;
			currentCyc+=switch_cyc;
			continue;
		}
		threads[currentThread].executeCommand();
		inst_ctr++;
		//printf("current cycle is : %d and instruction counter is : %d\n", currentCyc, inst_ctr);
		if(threads[currentThread].isActive){
			threads[currentThread].current_line_inst++;
			threads[currentThread].last_exe_cycle = currentCyc;
			SIM_MemInstRead(threads[currentThread].current_line_inst, &threads[currentThread].current_inst, currentThread);
		}
		else
		{
			active_threads--;
		}
		currentCyc++;
	}
}

void Core::run_prog_fineGrained()
{
	int active_threads = threads_number;
	while (active_threads)
	{
		int next_thread = threadRR();
		if(next_thread < 0)
		{
			currentCyc++;
			continue;
		}
		currentThread = next_thread;
		threads[currentThread].executeCommand();
		inst_ctr++;
		//printf("current cycle is : %d and instruction counter is : %d\n", currentCyc, inst_ctr);
		if(threads[currentThread].isActive){
			threads[currentThread].current_line_inst++;
			threads[currentThread].last_exe_cycle = currentCyc;
			SIM_MemInstRead(threads[currentThread].current_line_inst, &threads[currentThread].current_inst, currentThread);
		}
		else
		{
			active_threads--;
		}
		currentThread++;
		currentCyc++;
	}
}

tcontext Core::getCTX(int threadid)
{
	return threads[threadid].context;
}

double Core::getCPI()
{
	return (double)currentCyc/inst_ctr;
}

Core* core;

void CORE_BlockedMT() {
	core =  new Core();
	core->run_prog_blocked();
}

void CORE_FinegrainedMT() {
	core =  new Core();
	core->run_prog_fineGrained();
}

double CORE_BlockedMT_CPI(){
	double cpi = core->getCPI();
	delete core;
	return cpi;
}

double CORE_FinegrainedMT_CPI(){
	double cpi = core->getCPI();
	delete core;
	return cpi;
}

void CORE_BlockedMT_CTX(tcontext* context, int threadid) {
	context[threadid] = core->getCTX(threadid);
	return;
}

void CORE_FinegrainedMT_CTX(tcontext* context, int threadid) {
	context[threadid] = core->getCTX(threadid);
	return;
}
