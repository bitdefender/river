#ifndef CONCOLIC_EXECUTOR_H
#define CONCOLIC_EXECUTOR_H

#include "constraintsSolver.h"
#include <string>
#include <unordered_set>
#include <vector>
#include <queue>
#include <semaphore.h>

//////////
// Usefull code starts here

using ArrayOfUnsignedChars = std::vector<unsigned char>;

struct ExecutionOptions
{
public:
	bool m_useIPC = false;			// If true, we use inter process communication between this and tracer. Highly recommended for release versions
	int m_numProcessesToUse = -1;	// The number of processes to use for tracer execution
	bool spawnTracersManually; 		// If true, tracers are spawned by hand
};

struct WorkerInfo
{
	int socket = -1;				// The client socket associated with this symbolic worker
	FILE* socketReadStream = 0;		// The socket above opened as a read stream (to mimic a file reading from it)
};

struct ExecutionState
{
	sem_t* m_syncSemaphore;			// Used to synchronize comm between executor and tracer
	std::vector<WorkerInfo> m_workers;
	int m_serverSocket = -1;

	std::vector<int> m_tracersPID;	// Pids of all child workers spawned
};

#define TERMINATION_TAG  -1

// This is the object that instruments the concolic execution
// TODO: add serialized IPC communication between this and tracer
class ConcolicExecutor
{
public:
	ConcolicExecutor(const int MAX_TRACER_INPUT_SIZE, const int MAX_TRACER_OUTPUT_SIZE,
	const char* testedLibrary, 
	const ExecutionOptions& execOptions = ExecutionOptions());
	virtual ~ConcolicExecutor();


	// External textual execution of tracer - must be used only for debug purposes
	// --------------------------------------------------------------------------------------
	// Executes the tracer with a given input and fill out the path constraint
	// This is the "debug" version, with the tracer being invoked in an external process and reading the output generated file
	// TERRIBLE PERFORMANCE - use only for debugging
	void executeTracerSymbolically_external(const InputPayload& input, PathConstraint &outPathConstraint);

	// This function executes the library under test against input and:
	// fills the score inside 
	// reports any errors, crashes etc.
	void executeTracerTracking_external(InputPayload& input);
	// --------------------------------------------------------------------------------------


	// IPC - socket communication between this and tracer - should be preferred to test performance and real life deployment
	// --------------------------------------------------------------------------------------
	// These serve the same purpose as the above two functions. Normally it should be a strategy pattern here but we only have two strategies...
	void executeTracerSymbolically_ipc(const InputPayload& input, PathConstraint &outPathConstraint);
	void executeTracerTracking_ipc(InputPayload& input);
	// --------------------------------------------------------------------------------------

    // Search the solutions for a problem
    // TODO: This is subject to API change for sure. What do we do with the output ?
	void searchSolutions(const ArrayOfUnsignedChars&, const bool outputAllCoverageInputs = false, const char* fileToOutputCoverageInputs ="covinputsresults.txt");

private:
	const int m_MAX_TRACER_OUTPUT_SIZE;
	const int m_MAX_TRACER_INPUT_SIZE;

	static constexpr char* SOCKET_ADDRESS_COMM = (char*)"/home/ciprian/socketriver";

	// Run the input symbolically, negate the constraints one by one and get new inputs by solving them with the SMT
	// Returns in the out variable the input childs of the base input parameter
	void ExpandExecution(const InputPayload& input, std::vector<InputPayload>& outGeneratedInputChildren);

	// The name (or full path) of the library under test.
	std::string m_testedLibrary; 

	// This is a set for all blocks (identified by their addresses) touched durin execution		
	std::unordered_set<int> m_blockAddresesTouched; 

	std::vector<InputPayload> m_outGeneratedInputChildren;

	// Worklist scored by the items scores
	std::priority_queue<InputPayload> m_workList;

	// Buffer to store the last output from execution of the tracer
	char* m_lastTracerOutputBuffer = nullptr;
	int m_lastTracerOutputSize = 0;

	// Buffer to store input for a tracer 
	char *m_lastTracerInputBuffer = nullptr;
	int m_lastTracerInputSize = 0;

	// IPC / distributed execution details and functionality. Should put them in a separate structure if more members are added
	//---------------------------------------------------
	ExecutionOptions 	m_execOptions;
	ExecutionState 	 	m_execState;


	// This solves the connection between this and tracer (tracers)
	void handshakeWithSymbolicTracer();

	// Close connections with workers
	void closeConnections();

	// Sends a message to worker. If size = TERMINATION_TAG (-1) => termination message (ugly but fast)
	void sendTaskMessageToWorker(WorkerInfo& worker, const int size, const unsigned char* content);
	//---------------------------------------------------
};

#endif
