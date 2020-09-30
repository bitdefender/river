#include "concolicExecutor.h"
#include "utils.h"
#include <string.h>
#include <set>
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <sys/types.h> 
#include <sys/stat.h>
#include <unistd.h> 
#include "tracerExecutionStrategy.h"
#include "tracerExecutionStrategyExternal.h"
#include "tracerExecutionStrategyIPC.h"
#include "tracerExecutionStrategyMPI.h"

// If below is enabled, we just take the textual output saved in the local folder and run over it.
// Usefull to debug things
//#define SIMULATE_TRACER_EXECUTION


ConcolicExecutor::ConcolicExecutor(const ExecutionOptions& execOptions)
{
	m_execOptions = execOptions;

	switch (m_execOptions.m_execType)
	{
	case ExecutionOptions::EXEC_SERIAL:
		m_tracerExecutionStrategy = new TracerExecutionStrategyExternal(m_execOptions);
		break;
	case ExecutionOptions::EXEC_DISTRIBUTED_IPC:
		m_tracerExecutionStrategy = new TracerExecutionStrategyIPC(m_execOptions);
		break;
	case ExecutionOptions::EXEC_DISTRIBUTED_MPI:
		m_tracerExecutionStrategy = new TracerExecutionStrategyMPI(m_execOptions);
		break;	
	default:
		break;
	}

	m_tracerExecutionStrategy->init();

	// Get the state created by the strategy
	m_execState = m_tracerExecutionStrategy->getExecutionState();

	// Output setup
	const bool showOutputAsText = m_execOptions.IsOutputOptionEnabled(ExecutionOptions::OPTION_TEXT);
	const bool showOUtputAsBinary = m_execOptions.IsOutputOptionEnabled(ExecutionOptions::OPTION_BINARY);
	const char* outputFolderPath = m_execOptions.m_outputFolderPath.c_str();

	m_generatedInputsCount = 0;
	// If either text or binary output requested, delete and make the outputs folder again
	if (showOutputAsText || showOUtputAsBinary)
	{
		//int err = rmdir("outputs");
		const int dir_err = mkdir(outputFolderPath, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
		if (-1 == dir_err && errno != EEXIST)
		{
			std::cerr << "Error :  " << strerror(errno) << std::endl;
			printf("Error creating directory!n");
			exit(1);
		}
	}

	if (showOutputAsText)
	{
		char textCoverageInputPath[4096];
		snprintf(textCoverageInputPath, sizeof(textCoverageInputPath), "%s/allinputs.txt", outputFolderPath);
		m_outTextCoverage.open(textCoverageInputPath);
	}

}

ConcolicExecutor::~ConcolicExecutor()
{
	delete m_tracerExecutionStrategy;
}

// Run the input symbolically, negate the constraints one by one and get new inputs by solving them with the SMT
// Returns in the out variable the input childs of the base input parameter
void ConcolicExecutor::ExpandExecution(const InputPayload& input, std::vector<InputPayload>& outGeneratedInputChildren) 
{
	//execute and fill the path constraints for this input
	PathConstraint pathConstraint;
	m_tracerExecutionStrategy->executeTracerSymbolically(input, pathConstraint);

	const int numConstraints = pathConstraint.constraints.size();
	outGeneratedInputChildren.clear();
	outGeneratedInputChildren.reserve(numConstraints);
	// Initialize a path constraint solver then use it to inverse and solve inputs for all its children according to the algorithm
	PathConstraintZ3Solver pcsolver;
	pcsolver.init(&pathConstraint);
	for(int j = input.bound + 1; j < numConstraints; j++) 
	{
		// Step 1: generate first an input with j'th condition inverted
		//-------------
		pcsolver.pushState(); // Store state to revert later to it
		// Invert the test (i.e. if it was taken then go to not taken, and converse)
		pathConstraint.constraints[j].setInverted(true);
		// add to the interval solver the inverted j'th constraint
		pcsolver.addConstraint(j);

		// Get new input if conditions can be satisfied
		InputPayload newInputPayload;
		newInputPayload.input = input.input;  // Copy the original input and modify only the affected bytes
		if (pcsolver.solve(newInputPayload))
		{
			newInputPayload.bound = j;
			outGeneratedInputChildren.emplace_back(newInputPayload);
		}
		// Step 2: remove the inverted condition and add to the solver the normal condition before going to next children
		//-------------
		pcsolver.popState(); // Basically here we removed the last inverted condition
		// Put the flag back for consistency
		pathConstraint.constraints[j].setInverted(false);
		pcsolver.addConstraint(j);	// And add the normal condition here. before going to the next
	}
}

void ConcolicExecutor::searchSolutions(const ArrayOfUnsignedChars& startInput) 
{
	// Set the first input received and add it to the worklist
	InputPayload initialInput;
	initialInput.input = startInput;
	initialInput.bound = -1;
	m_workList.push(std::move(initialInput));
	

	const bool showOutputAsText = m_execOptions.IsOutputOptionEnabled(ExecutionOptions::OPTION_TEXT);
	const bool showOUtputAsBinary = m_execOptions.IsOutputOptionEnabled(ExecutionOptions::OPTION_BINARY);
	const bool shouldFilterOutOkInputs = m_execOptions.IsOutputOptionEnabled(ExecutionOptions::OPTION_FILTER_NON_INTERESTING);
	const char* outputsPath = m_execOptions.m_outputFolderPath.c_str();


	// Perform a search inside the existing tree of tasks
	while(!m_workList.empty())  
	{
		// Take the top scored path in the worklist - note that this is not executed yet and its constraints are not valid 
		const InputPayload inputPicked(std::move(m_workList.top()));
		m_workList.pop();
		
		// Execute tracer and library using this input and produce the children input
		// Expand the input by negating branch test along the path
		m_outGeneratedInputChildren.clear();
		ExpandExecution(inputPicked, m_outGeneratedInputChildren);
		// Iterate over all children input, run & check them, score then add to the worklist
		for (InputPayload& payloadChildren : m_outGeneratedInputChildren)
		{
			// TODO: get results and report potential problems somewhere
			const bool executedOk = m_tracerExecutionStrategy->executeTracerTracking(payloadChildren);

			// Either the input is not OK or the filtering option is disabled..
			if (executedOk == false || shouldFilterOutOkInputs == false)
			{
				m_generatedInputsCount++;
				if (showOutputAsText)
				{
					for (unsigned char chr : inputPicked.input)
						m_outTextCoverage << (int)chr <<" ";
					m_outTextCoverage << std::endl;
				}
				else if (showOUtputAsBinary)
				{
					static char buff[1024];
					snprintf(buff, 1023, "%s/input%d.bin", outputsPath, m_generatedInputsCount);
					std::ofstream outTextCoverage(buff, std::ofstream::binary);
					assert(outTextCoverage.is_open() && "can't write the output file!");
					outTextCoverage.write((const char*)inputPicked.input.data(), inputPicked.input.size());
					outTextCoverage.close();
				}
			}

			m_workList.push(payloadChildren);
		}
	}

    if (showOutputAsText)
	{
		m_outTextCoverage.close();
	}
}
