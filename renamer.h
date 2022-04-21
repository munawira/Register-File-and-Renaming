
class renamer {
private:
	/////////////////////////////////////////////////////////////////////
	// Put private class variables here.
	/////////////////////////////////////////////////////////////////////
	unsigned int numLogRegs;
	unsigned int numPhysRegs;
	unsigned int numUnresolvedBranches;
	unsigned int freeListSize;
	unsigned int activeListSize;
	unsigned int loadDispatchVar,loadCommitVar;
	/////////////////////////////////////////////////////////////////////
	// Structure 1: Rename Map Table
	// Entry contains: physical register mapping
	/////////////////////////////////////////////////////////////////////
	unsigned long *RMT;
	unsigned long **SMT;
	/////////////////////////////////////////////////////////////////////
	// Structure 2: Architectural Map Table
	// Entry contains: physical register mapping
	/////////////////////////////////////////////////////////////////////
	unsigned long *AMT;
	/////////////////////////////////////////////////////////////////////
	// Structure 3: Free List
	//
	// Entry contains: physical register number
	//
	// Notes:
	// * Structure includes head, tail, and possibly other variables
	//   depending on your implementation.
	/////////////////////////////////////////////////////////////////////
	unsigned long *freeList;
	bool headPhaseFL;
	int headFL;
	bool tailPhaseFL;
	int tailFL;
	
	/////////////////////////////////////////////////////////////////////
	// Structure 4: Active List
	//
	// Entry contains:
	// 1. destination flag (indicates whether or not the instr. has a
	//    destination register)
	// 2. logical register number of the instruction's destination
	// 3. physical register number of the instruction's destination
	// 4. completed bit
	// 5. exception bit
	// 6. load flag (indicates whether or not the instr. is a load)
	// 7. store flag (indicates whether or not the instr. is a store)
	// 8. branch flag (indicates whether or not the instr. is a branch)
	// 9. program counter of the instruction
	//
	// Notes:
	// * Structure includes head, tail, and possibly other variables
	//   depending on your implementation.
	/////////////////////////////////////////////////////////////////////
	struct AL{
		bool hasDest;
		int logDest;
		int physDest;
		bool completed;
		bool exception;
		bool isLoad;
		bool isStore;
		bool isBranch;
		unsigned long PC;
		}; 
	AL *ActiveList;
	bool headPhaseAL;
	int headAL;
	bool tailPhaseAL;
	int tailAL;
	
	/////////////////////////////////////////////////////////////////////
	// Structure 5: Physical Register File
	// Entry contains: value
	//
	// Notes:
	// * The value must be of the following type: unsigned long long
	/////////////////////////////////////////////////////////////////////
	unsigned long long *PRF;
	bool *PRFReadyBits;
	
	/////////////////////////////////////////////////////////////////////
	// Structure 6: Physical Register File Ready Bit Array
	// Entry contains: ready bit
	/////////////////////////////////////////////////////////////////////
	

	/////////////////////////////////////////////////////////////////////
	// Structure 7: Global Branch Mask (GBM)
	//
	// The Global Branch Mask (GBM) is a bit vector that keeps track of
	// all unresolved branches. A '1' bit corresponds to an unresolved
	// branch. The "branch ID" of the unresolved branch is its position
	// in the bit vector.
	//
	// The GBM serves two purposes:
	//
	// 1. It provides a means for allocating checkpoints to unresolved
	//    branches. There are as many checkpoints as there are bits in
	//    the GBM. If all bits in the GBM are '1', then there are no
	//    free bits, hence, no free checkpoints. On the other hand, if
	//    not all bits in the GBM are '1', then any of the '0' bits
	//    are free and the corresponding checkpoints are free.
	//    
	// 2. Each in-flight instruction needs to know which unresolved
	//    branches it depends on, i.e., which unresolved branches are
	//    logically before it in program order. This information
	//    makes it possible to squash instructions that are after a
	//    branch, in program order, and not instructions before the
	//    branch. This functionality will be implemented using
	//    branch masks, as was done in the MIPS R10000 processor.
	//    An instruction's initial branch mask is the value of the
	//    the GBM when the instruction is renamed.
	//
	// The simulator requires an efficient implementation of bit vectors,
	// for quick copying and manipulation of bit vectors. Therefore, you
	// must implement the GBM as an "unsigned long long".
	// An "unsigned long long" contains 64 bits, therefore, the simulator
	// cannot support a processor configuration with more than 64
	// unresolved branches. The maximum number of unresolved branches
	// is configurable by the user of the simulator, and can range from
	// 1 to 64.
	/////////////////////////////////////////////////////////////////////
	unsigned long long GBM;
	unsigned long long branchMask;
	unsigned int branchCount;

	/////////////////////////////////////////////////////////////////////
	// Structure 8: Branch Checkpoints
	//
	// Each branch checkpoint contains the following:
	// 1. Shadow Map Table (checkpointed Rename Map Table)
	// 2. checkpointed Free List head index
	// 3. checkpointed GBM
	/////////////////////////////////////////////////////////////////////
	struct branchCheckPoints{
		int SMT;
		int FLhead;
		unsigned long long GBM;
		unsigned int branchCount;
		bool FLheadPhase;
	};
	branchCheckPoints *brCheckPoint;
	/////////////////////////////////////////////////////////////////////
	// Private functions.
	// e.g., a generic function to copy state from one map to another.
	/////////////////////////////////////////////////////////////////////
	void mapCopy(unsigned long* Source, unsigned long* Dest, unsigned int size)
	{
		int i=0;
		while(i<size)
		{
			Dest[i] = Source[i];
			i++;
		}
	}
	
	void flipPhase(bool *phase)
	{
		if((*phase) == true)
			(*phase) = false;
		else
			(*phase) = true;	
	}
public:
	////////////////////////////////////////
	// Public functions.
	////////////////////////////////////////

	/////////////////////////////////////////////////////////////////////
	// This is the constructor function.
	// When a renamer object is instantiated, the caller indicates:
	// 1. The number of logical registers (e.g., 32).
	// 2. The number of physical registers (e.g., 128).
	// 3. The maximum number of unresolved branches.
	//    Requirement: 1 <= n_branches <= 64.
	//
	// Tips:
	//
	// Assert the number of physical registers > number logical registers.
	// Assert 1 <= n_branches <= 64.
	// Then, allocate space for the primary data structures.
	// Then, initialize the data structures based on the knowledge
	// that the pipeline is intially empty (no in-flight instructions yet).
	/////////////////////////////////////////////////////////////////////
	renamer(unsigned int n_log_regs,
		unsigned int n_phys_regs,
		unsigned int n_branches);

	/////////////////////////////////////////////////////////////////////
	// This is the destructor, used to clean up memory space and
	// other things when simulation is done.
	// I typically don't use a destructor; you have the option to keep
	// this function empty.
	/////////////////////////////////////////////////////////////////////
	~renamer();


	//////////////////////////////////////////
	// Functions related to Rename Stage.   //
	//////////////////////////////////////////

	/////////////////////////////////////////////////////////////////////
	// The Rename Stage must stall if there aren't enough free physical
	// registers available for renaming all logical destination registers
	// in the current rename bundle.
	//
	// Inputs:
	// 1. bundle_dst: number of logical destination registers in
	//    current rename bundle
	//
	// Return value:
	// Return "true" (stall) if there aren't enough free physical
	// registers to allocate to all of the logical destination registers
	// in the current rename bundle.
	/////////////////////////////////////////////////////////////////////
	bool stall_reg(unsigned int bundle_dst);

	/////////////////////////////////////////////////////////////////////
	// The Rename Stage must stall if there aren't enough free
	// checkpoints for all branches in the current rename bundle.
	//
	// Inputs:
	// 1. bundle_branch: number of branches in current rename bundle
	//
	// Return value:
	// Return "true" (stall) if there aren't enough free checkpoints
	// for all branches in the current rename bundle.
	/////////////////////////////////////////////////////////////////////
	bool stall_branch(unsigned int bundle_branch);

	/////////////////////////////////////////////////////////////////////
	// This function is used to get the branch mask for an instruction.
	/////////////////////////////////////////////////////////////////////
	unsigned long long get_branch_mask();

	/////////////////////////////////////////////////////////////////////
	// This function is used to rename a single source register.
	//
	// Inputs:
	// 1. log_reg: the logical register to rename
	//
	// Return value: physical register name
	/////////////////////////////////////////////////////////////////////
	unsigned int rename_rsrc(unsigned int log_reg);

	/////////////////////////////////////////////////////////////////////
	// This function is used to rename a single destination register.
	//
	// Inputs:
	// 1. log_reg: the logical register to rename
	//
	// Return value: physical register name
	/////////////////////////////////////////////////////////////////////
	unsigned int rename_rdst(unsigned int log_reg);

	/////////////////////////////////////////////////////////////////////
	// This function creates a new branch checkpoint.
	//
	// Inputs: none.
	//
	// Output:
	// 1. The function returns the branch's ID. When the branch resolves,
	//    its ID is passed back to the renamer via "resolve()" below.
	//
	// Tips:
	//
	// Allocating resources for the branch (a GBM bit and a checkpoint):
	// * Find a free bit -- i.e., a '0' bit -- in the GBM. Assert that
	//   a free bit exists: it is the user's responsibility to avoid
	//   a structural hazard by calling stall_branch() in advance.
	// * Set the bit to '1' since it is now in use by the new branch.
	// * The position of this bit in the GBM is the branch's ID.
	// * Use the branch checkpoint that corresponds to this bit.
	// 
	// The branch checkpoint should contain the following:
	// 1. Shadow Map Table (checkpointed Rename Map Table)
	// 2. checkpointed Free List head index
	// 3. checkpointed GBM
	/////////////////////////////////////////////////////////////////////
	unsigned int checkpoint();

	//////////////////////////////////////////
	// Functions related to Dispatch Stage. //
	//////////////////////////////////////////

	/////////////////////////////////////////////////////////////////////
	// The Dispatch Stage must stall if there are not enough free
	// entries in the Active List for all instructions in the current
	// dispatch bundle.
	//
	// Inputs:
	// 1. bundle_inst: number of instructions in current dispatch bundle
	//
	// Return value:
	// Return "true" (stall) if the Active List does not have enough
	// space for all instructions in the dispatch bundle.
	/////////////////////////////////////////////////////////////////////
	bool stall_dispatch(unsigned int bundle_inst);

	/////////////////////////////////////////////////////////////////////
	// This function dispatches a single instruction into the Active
	// List.
	//
	// Inputs:
	// 1. dest_valid: If 'true', the instr. has a destination register,
	//    otherwise it does not. If it does not, then the log_reg and
	//    phys_reg inputs should be ignored.
	// 2. log_reg: Logical register number of the instruction's
	//    destination.
	// 3. phys_reg: Physical register number of the instruction's
	//    destination.
	// 4. load: If 'true', the instr. is a load, otherwise it isn't.
	// 5. store: If 'true', the instr. is a store, otherwise it isn't.
	// 6. branch: If 'true', the instr. is a branch, otherwise it isn't.
	// 7. PC: Program counter of the instruction.
	//
	// Return value:
	// Return the instruction's index in the Active List.
	//
	// Tips:
	//
	// Before dispatching the instruction into the Active List, assert
	// that the Active List isn't full: it is the user's responsibility
	// to avoid a structural hazard by calling stall_dispatch()
	// in advance.
	/////////////////////////////////////////////////////////////////////
	unsigned int dispatch_inst(bool dest_valid,
				   unsigned int log_reg,
				   unsigned int phys_reg,
				   bool load,
				   bool store,
				   bool branch,
				   unsigned int PC);


	//////////////////////////////////////////
	// Functions related to Schedule Stage. //
	//////////////////////////////////////////

	/////////////////////////////////////////////////////////////////////
	// Test the ready bit of the indicated physical register.
	// Returns 'true' if ready.
	/////////////////////////////////////////////////////////////////////
	bool is_ready(unsigned int phys_reg);

	/////////////////////////////////////////////////////////////////////
	// Clear the ready bit of the indicated physical register.
	/////////////////////////////////////////////////////////////////////
	void clear_ready(unsigned int phys_reg);

	/////////////////////////////////////////////////////////////////////
	// Set the ready bit of the indicated physical register.
	/////////////////////////////////////////////////////////////////////
	void set_ready(unsigned int phys_reg);


	//////////////////////////////////////////
	// Functions related to Reg. Read Stage.//
	//////////////////////////////////////////

	/////////////////////////////////////////////////////////////////////
	// Return the contents (value) of the indicated physical register.
	/////////////////////////////////////////////////////////////////////
	unsigned long long read(unsigned int phys_reg);


	//////////////////////////////////////////
	// Functions related to Writeback Stage.//
	//////////////////////////////////////////

	/////////////////////////////////////////////////////////////////////
	// Write a value into the indicated physical register.
	/////////////////////////////////////////////////////////////////////
	void write(unsigned int phys_reg, unsigned long long value);

	/////////////////////////////////////////////////////////////////////
	// Set the completed bit of the indicated entry in the Active List.
	/////////////////////////////////////////////////////////////////////
	void set_complete(unsigned int AL_index);

	/////////////////////////////////////////////////////////////////////
	// This function is for handling branch resolution.
	//
	// Inputs:
	// 1. AL_index: Index of the branch in the Active List.
	// 2. branch_ID: This uniquely identifies the branch and the
	//    checkpoint in question.  It was originally provided
	//    by the checkpoint function.
	// 3. correct: 'true' indicates the branch was correctly
	//    predicted, 'false' indicates it was mispredicted
	//    and recovery is required.
	//
	// Outputs: none.
	//
	// Tips:
	//
	// While recovery is not needed in the case of a correct branch,
	// some actions are still required with respect to the GBM and
	// all checkpointed GBMs:
	// * Remember to clear the branch's bit in the GBM.
	// * Remember to clear the branch's bit in all checkpointed GBMs.
	//
	// In the case of a misprediction:
	// * Restore the GBM from the checkpoint. Also make sure the
	//   mispredicted branch's bit is cleared in the restored GBM,
	//   since it is now resolved and its bit and checkpoint are freed.
	// * You don't have to worry about explicitly freeing the GBM bits
	//   and checkpoints of branches that are after the mispredicted
	//   branch in program order. The mere act of restoring the GBM
	//   from the checkpoint achieves this feat.
	// * Restore other state using the branch's checkpoint.
	//   In addition to the obvious state ...  *if* you maintain a
	//   freelist length variable (you may or may not), you must
	//   recompute the freelist length. It depends on your
	//   implementation how to recompute the length.
	//   (Note: you cannot checkpoint the length like you did with
	//   the head, because the tail can change in the meantime;
	//   you must recompute the length in this function.)
	/////////////////////////////////////////////////////////////////////
	void resolve(unsigned int AL_index,
		     unsigned int branch_ID,
		     bool correct);

	//////////////////////////////////////////
	// Functions related to Retire Stage.   //
	//////////////////////////////////////////

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
	// 6. offending_PC: If the exception output is 'true', this output
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
	void commit(bool &committed, bool &load, bool &store, bool &branch,
		    bool &exception, unsigned int &offending_PC);


	//////////////////////////////////////////
	// Functions not tied to specific stage.//
	//////////////////////////////////////////

	/////////////////////////////////////////////////////////////////////
	// Set the exception bit of the indicated entry in the Active List.
	/////////////////////////////////////////////////////////////////////
	void set_exception(unsigned int AL_index);
};
