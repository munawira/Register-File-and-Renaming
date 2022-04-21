/*
 *ECE721 Project2
 *unityid : mkotyad
 *Author: Munawira Kotyad
 */
 
#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <bitset>
#include <assert.h>
using namespace std;

#include "renamer.h"


renamer::renamer(unsigned int n_log_regs,unsigned int n_phys_regs,unsigned int n_branches)
{
	//Check for assertions
	assert(n_log_regs < n_phys_regs);
	assert(1<=n_branches);
	assert(n_branches<=64);
	
	numLogRegs = n_log_regs;
	numPhysRegs = n_phys_regs;
	numUnresolvedBranches = n_branches;
	activeListSize = numPhysRegs - numLogRegs;
	freeListSize = numPhysRegs - numLogRegs;
	
	RMT = new unsigned long [numLogRegs];
	AMT = new unsigned long [numLogRegs];
	
	SMT = new unsigned long* [numUnresolvedBranches];
	for(int i=0;i<numUnresolvedBranches;i++)
	{
		SMT[i] = new unsigned long[numLogRegs];	
	}
	
	
	brCheckPoint = new branchCheckPoints [n_branches];
	GBM = 0;
	branchMask =1;
	branchCount =0;
	freeList = new unsigned long [freeListSize];
	headFL = 0;
	headPhaseFL = false;
	tailFL = freeListSize - 1;
	tailPhaseFL = false;
	
	headAL =0;
	headPhaseAL = false;
	tailAL =0;
	tailPhaseAL = false; 
	ActiveList = new AL [activeListSize];
	
	PRF = new unsigned long long[numPhysRegs];
	PRFReadyBits = new bool[numPhysRegs];
	
	for(int i=0; i<numPhysRegs;i++)
	{
		PRF[i] = 0;
		PRFReadyBits[i] = true;	
	}
	int i;
	for(i=0;i<numLogRegs;i++)
	{
		AMT[i] = i;
		RMT[i] = i;
 	}
	int j;
	for(i= numLogRegs,j=0;i<numPhysRegs;i++,j++)
	{
		freeList[j]=i;
	}
	
	
}

bool renamer::stall_reg(unsigned int bundle_dst)
{
	int numFreeRegs = 0;
		
	if(headPhaseFL == tailPhaseFL)
	{
		numFreeRegs = tailFL - headFL +1;
	}
	else
	{
		numFreeRegs = freeListSize -headFL + tailFL +1;
	}	
		
	
	if(numFreeRegs < bundle_dst)
	{
		return true;
	}
	else
		return false;


}
		
bool renamer::stall_branch(unsigned int bundle_branch)
{
	branchMask =1;
	int count =0;
	for(int i=0;i<numUnresolvedBranches;i++)
	{
		if((GBM & branchMask) == 0)
		{
			count++;
		}
		branchMask = branchMask << 1; 
		
	}
	
	branchMask =1;
	if (count < bundle_branch)
	{
		return true;
	}
	else
	{
		return false;
	}
	
}

unsigned long long renamer:: get_branch_mask()
{
	return GBM;
}


unsigned int renamer::rename_rsrc(unsigned int log_reg)
{
	return RMT[log_reg];
}		


unsigned int renamer::rename_rdst(unsigned int log_reg)
{
	RMT[log_reg] = freeList[headFL];
	headFL++;
	if (headFL == freeListSize)
	{
		headFL = 0;
		flipPhase(&headPhaseFL);
		
	}
	return RMT[log_reg];
}

unsigned int renamer::checkpoint()
{
	unsigned int branchId =0;
	unsigned long long tempGBM =0;
	
	branchMask =1;
	
	for(int i=0;i<numUnresolvedBranches;i++)
	{
		tempGBM = GBM & branchMask;
		if(tempGBM == 0)
		{
			GBM = GBM|branchMask;
			branchId = i;
			
			mapCopy(RMT,SMT[branchId],numLogRegs);
	
	
			brCheckPoint[branchId].GBM = GBM;
			brCheckPoint[branchId].SMT = branchId;
			brCheckPoint[branchId].FLheadPhase = headPhaseFL;
			brCheckPoint[branchId].FLhead = headFL;
			brCheckPoint[branchId].branchCount = branchCount;
			branchCount++;
			break;
		
		}
		
		branchMask = branchMask<<1;
	}
	branchMask =1;

	return branchId;
 }


bool renamer::stall_dispatch(unsigned int bundle_inst)
{
	int numFreeEntries = 0;
	
	if(headPhaseAL == tailPhaseAL)
	{
		
		numFreeEntries = activeListSize - tailAL + headAL;
		
	}
	else
	{
		numFreeEntries =headAL - tailAL;
		
	}	
	if (numFreeEntries >= bundle_inst)
	{
	return false;
	}	
	else
	{
	return true;
	}
	
}

unsigned int renamer::dispatch_inst(bool dest_valid,
				   unsigned int log_reg,
				   unsigned int phys_reg,
				   bool load,
				   bool store,
				   bool branch,
				   unsigned int PC)
{
	unsigned int index;
	index = tailAL;
	
	
	ActiveList[tailAL].hasDest = dest_valid;
	if(dest_valid)
	{
		ActiveList[tailAL].logDest = log_reg;
		ActiveList[tailAL].physDest = phys_reg;
	}
	ActiveList[tailAL].isLoad = load;
	ActiveList[tailAL].isStore = store;
	ActiveList[tailAL].isBranch = branch;
	ActiveList[tailAL].PC = PC;
	ActiveList[tailAL].completed = false;
	ActiveList[tailAL].exception = false;
	if(load)
		{
		loadDispatchVar++;
		}
	tailAL++;
	
	if(tailAL == activeListSize)
	{
	tailAL = 0;
	flipPhase(&tailPhaseAL);
	}
	
	
	return index;
	
}

bool renamer::is_ready(unsigned int phys_reg){
	return PRFReadyBits[phys_reg];

}

void renamer::clear_ready(unsigned int phys_reg)
{
	PRFReadyBits[phys_reg]=false;

}

void renamer::set_ready(unsigned int phys_reg)
{
	PRFReadyBits[phys_reg] = true;
}

unsigned long long renamer::read(unsigned int phys_reg)
{
	return PRF[phys_reg];
}


void renamer::write(unsigned int phys_reg, unsigned long long value)
{
	PRF[phys_reg]=value;
}

void renamer::set_complete(unsigned int AL_index)
{
	ActiveList[AL_index].completed = true;

}

void renamer::resolve(unsigned int AL_index,
		     unsigned int branch_ID,
		     bool correct)
{
	branchMask = 1;
	branchMask = branchMask << branch_ID;
	unsigned long long tempMask =0;
	tempMask = ~branchMask;
	if(correct)
	{
		GBM = GBM & tempMask;
		for(int i=0;i<numUnresolvedBranches;i++)
		{
			brCheckPoint[i].GBM = brCheckPoint[i].GBM & tempMask;	
		}
		branchCount--;
		branchMask =1;
		
		return;
	}
	else
	{
		brCheckPoint[branch_ID].GBM = brCheckPoint[branch_ID].GBM & tempMask;
		GBM = brCheckPoint[branch_ID].GBM;

		branchCount = brCheckPoint[branch_ID].branchCount;

		headFL = brCheckPoint[branch_ID].FLhead;
		headPhaseFL = brCheckPoint[branch_ID].FLheadPhase;
		
	/*	if(headFL >= brCheckPoint[branch_ID].FLhead)
		{
			headFL = brCheckPoint[branch_ID].FLhead;
		}
		else
		{
			headFL = brCheckPoint[branch_ID].FLhead;
			flipPhase(&headPhaseFL);
		}
*/
		if(((AL_index + 1) % activeListSize) > tailAL) {
			flipPhase(&tailPhaseAL);
		}
		tailAL = ( AL_index + 1 ) % activeListSize;

/*		if(tailAL >= AL_index)
		{
			tailAL = AL_index + 1;
			if (tailAL == activeListSize)
			{tailAL =0;	
			flipPhase(&tailPhaseAL);
			}	
		}
		else
		{
			tailAL = AL_index + 1;
			
			flipPhase(&tailPhaseAL);
			if(tailAL == activeListSize)
			{tailAL =0;
			flipPhase(&tailPhaseAL);
			}
		}
*/		
		mapCopy(SMT[branch_ID],RMT,numLogRegs);
		
		return;
	}
	branchMask =1;
}


/////////////////////////////////////////////////////////////////////
	// This function attempts to commit the instruction at the head
	// of the Active List.
	//
	// Input arguments: none.
	//
	// Output arguments:
	// 1. committed: Set this flag to 'true' if the head instruction
	//    was committed.
	// 2. load: Set this flag to 'true' if the head instruction is
	//    a load. (If committed and load are both 'true', they signal
	//    the processor to commit the corresponding load from the LQ.)
	// 3. store: Set this flag to 'true' if the head instruction is
	//    a store. (If committed and store are both 'true', they signal
	//    the processor to commit the corresponding store from the SQ.)
	// 4. branch: Set this flag to 'true' if the head instruction is
	//    a branch. (If committed and branch are both 'true', they signal
	//    the processor to commit the corresponding branch from the BQ.)
	// 5. exception: Set this flag to 'true' if the instruction at the
	//    head of the Active List is completed and marked as an exception.
	//    This signals to the processor that the pipeline should be
	//    squashed and restarted.
	// 6. offending_PC: If the exception output is 'true', this outputset
	//    should be the PC of the offending instruction, so that the
	//    processor knows which instruction to restart from.
	//
	// When this function is called:
	//
	// If the instruction at the head of the Active List is not completed,
	// then don't commit it and don't signal an exception. In this case,
	// both the committed and exception outputs should be 'false'
	// (in which case the load, store, branch, and offending_PC outputs
	// are ignored).
	// 
	// If the instruction at the head of the Active List is completed
	// and not marked as an exception, commit it. In this case, the
	// committed output is 'true' and the load, store, and branch outputs
	// depend on whether or not the committed instruction is a load, store,
	// or branch, respectively.
	// The exception output is 'false' (in which case the offending_PC
	// output is ignored).
	// 
	// If there is an offending instruction (completed=1, exception=1)
	// at the head of the Active List, don't commit the offending
	// instruction. Instead:
	// * Restore the Rename Map Table and Free List.
	// * Other structures need to be made consistent with an empty
	//   pipeline.
	// * The exception output should be 'true' and the offending_PC
	//   output should be set accordingly.
	// * The committed output should be 'false' (in which case the load,
	//   store, and branch outputs are ignored).
	/////////////////////////////////////////////////////////////////////
void renamer::commit(bool &committed, bool &load, bool &store, bool &branch, bool &exception, unsigned int &offending_PC)
{
	if((headAL == tailAL) && (headPhaseAL == tailPhaseAL))
	{
	committed = false;
	exception = false;
	return;
	}
	
	if(ActiveList[headAL].completed == true)
	{
	if (ActiveList[headAL].exception == true)
	{	//cout << "commit2";
		committed = false;
		exception = true;
		mapCopy(AMT,RMT,numLogRegs);
		headFL =0;
		tailFL = freeListSize -1;
		tailPhaseFL = headPhaseFL = false;
		tailAL = headAL;
		tailPhaseAL = headPhaseAL = false;
		offending_PC = ActiveList[headAL].PC;
		GBM =0;
		return;
	}
	else
	{	
		committed = true;
		exception = false;
		load = ActiveList[headAL].isLoad;
		if(load)
		{
		loadCommitVar++;
		}
		store = ActiveList[headAL].isStore;
		branch = ActiveList[headAL].isBranch;
		
		if(ActiveList[headAL].hasDest)
		{
			tailFL++;
			if(tailFL == freeListSize)
			{
				tailFL =0;
				flipPhase(&tailPhaseFL);
			}
			freeList[tailFL] = AMT[ActiveList[headAL].logDest];
			AMT[ActiveList[headAL].logDest] = ActiveList[headAL].physDest;
			
		}
		
		headAL++;
		if(headAL == activeListSize)
		{
			headAL =0;
			flipPhase(&headPhaseAL);
		}
		return;
	}
	}
	else
	{
		committed = false;
		exception = false;
		return;
	}
}
void renamer::set_exception(unsigned int AL_index)
{
	ActiveList[AL_index].exception= true;
   
}
		    


