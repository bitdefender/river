#ifndef CONCOLIC_EXECUTOR_H
#define CONCOLIC_EXECUTOR_H

#include "constraintsSolver.h"
#include <string>
#include <unordered_set>
#include <vector>
#include <queue>

//////////
// Usefull code starts here

using ArrayOfUnsignedChars = std::vector<unsigned char>;

// This is the object that instruments the concolic execution
// TODO: add serialized IPC communication between this and tracer
class ConcolicExecutor
{
public:
	ConcolicExecutor(const int MAX_TRACER_OUTPUT_SIZE, const char* testedLibrary);
	virtual ~ConcolicExecutor();

	// Executes the tracer with a given input and fill out the path constraint
	// This is the "debug" version, with the tracer being invoked in an external process and reading the output generated file
	// TERRIBLE PERFORMANCE - use only for debugging
	void executeTracerSymbolically_external(const InputPayload& input, PathConstraint &outPathConstraint);

	// This function executes the library under test against input and:
	// fills the score inside 
	// reports any errors, crashes etc.
	void executeTracerTracking_external(InputPayload& input);

	// Run the input symbolically, negate the constraints one by one and get new inputs by solving them with the SMT
	// Returns in the out variable the input childs of the base input parameter
	void ExpandExecution(const InputPayload& input, std::vector<InputPayload>& outGeneratedInputChildren);

    // Search the solutions for a problem
    // TODO: This is subject to API change for sure. What do we do with the output ?
	void searchSolutions(const ArrayOfUnsignedChars&, const bool outputAllCoverageInputs = false, const char* fileToOutputCoverageInputs ="covinputsresults.txt");

private:
	const int m_MAX_TRACER_OUTPUT_SIZE;

	// The name (or full path) of the library under test.
	std::string m_testedLibrary; 

	// This is a set for all blocks (identified by their addresses) touched durin execution		
	std::unordered_set<int> m_blockAddresesTouched; 

	std::vector<InputPayload> m_outGeneratedInputChildren;

	// Worklist scored by the items scores
	std::priority_queue<InputPayload> m_workList;

	// Buffers to store the last output from execution of the tracer
	char* m_lastTracerOutputBuffer = nullptr;
	int m_lastTracerOutputSize = 0;

};

#endif