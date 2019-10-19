#ifndef TRACER_EXECUTION_STRATEGY_IPC_H
#define TRACER_EXECUTION_STRATEGY_IPC_H

#include "tracerExecutionStrategy.h"
#include "concolicDefs.h"
#include <semaphore.h>

class InputPayload;
class PathConstraint;
class ConcolicExecutor;

// IPC - socket communication between this and tracer - should be preferred to test performance and real life deployment

struct IPCWorkerInfo
{
	int socket = -1;				// The client socket associated with this symbolic worker
	FILE* socketReadStream = 0;		// The socket above opened as a read stream (to mimic a file reading from it)
};

class ExecutionStateIPC : public ExecutionState
{
public:
	sem_t* m_syncSemaphore;			// Used to synchronize comm between executor and tracer
	std::vector<IPCWorkerInfo> m_workers;
	int m_serverSocket = -1;
	std::vector<int> m_tracersPID;	// Pids of all child workers spawned
};


class TracerExecutionStrategyIPC : public TracerExecutionStrategy
{
public:
    TracerExecutionStrategyIPC(const ExecutionOptions& execOptions) : TracerExecutionStrategy(execOptions) { }
	virtual ~TracerExecutionStrategyIPC();
	void executeTracerSymbolically(const InputPayload& payload, PathConstraint &outPathConstraint) override;
	bool executeTracerTracking(InputPayload& input) override;

	virtual ExecutionState* getExecutionState() { return &m_execState;}

private:
	static constexpr char* SOCKET_ADDRESS_COMM = (char*)"/home/ciprian/socketriver";

	void init();

	// Close connections with workers
	void closeConnections();

	// This solves the connection between this and tracer (tracers)
	void handshakeWithSymbolicTracer();

	// Sends a message to worker. If size = TERMINATION_TAG (-1) => termination message (ugly but fast)
	void sendTaskMessageToWorker(IPCWorkerInfo& worker, const int size, const unsigned char* content);

    ExecutionStateIPC m_execState;

	// Buffer to store the last output from execution of the tracer
	char* m_lastTracerOutputBuffer = nullptr;
	int m_lastTracerOutputSize = 0;

	// Buffer to store input for a tracer 
	char *m_lastTracerInputBuffer = nullptr;
	int m_lastTracerInputSize = 0;
};

#endif


