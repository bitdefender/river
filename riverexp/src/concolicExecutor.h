#ifndef CONCOLIC_EXECUTOR_H
#define CONCOLIC_EXECUTOR_H

#include "constraintsSolver.h"
#include <string>
#include <unordered_set>
#include <vector>
#include <queue>
#include <semaphore.h>
#include "concolicDefs.h"

//////////
// Usefull code starts here

using ArrayOfUnsignedChars = std::vector<unsigned char>;

class TracerExecutionStrategy;

// This is the object that instruments the concolic execution
// TODO: add serialized IPC communication between this and tracer
class ConcolicExecutor
{
public:
	ConcolicExecutor(const ExecutionOptions& execOptions);
	virtual ~ConcolicExecutor();


    // Search the solutions for a problem
    // TODO: This is subject to API change for sure. What do we do with the output ?
	void searchSolutions(const ArrayOfUnsignedChars&, const bool outputAllCoverageInputs = false, const char* fileToOutputCoverageInputs ="covinputsresults.txt");

protected:
	// Run the input symbolically, negate the constraints one by one and get new inputs by solving them with the SMT
	// Returns in the out variable the input childs of the base input parameter
	void ExpandExecution(const InputPayload& input, std::vector<InputPayload>& outGeneratedInputChildren);

	std::vector<InputPayload> m_outGeneratedInputChildren;

	// Worklist scored by the items scores
	std::priority_queue<InputPayload> m_workList;

	ExecutionOptions 			m_execOptions;
	ExecutionState*				m_execState;	// You have global access to the execution state
	TracerExecutionStrategy* 	m_tracerExecutionStrategy; // This is the object hiding tracer execution implementation
};

#endif
