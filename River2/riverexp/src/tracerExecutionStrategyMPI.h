#ifndef TRACER_EXECUTION_STRATEGY_MPI_H
#define TRACER_EXECUTION_STRATEGY_MPI_H

#include "tracerExecutionStrategy.h"
#include <assert.h>

class InputPayload;
class PathConstraint;
class ConcolicExecutor;

// TODO: implement this
class TracerExecutionStrategyMPI : public TracerExecutionStrategy
{
public:
    TracerExecutionStrategyMPI(const ExecutionOptions& execOptions) : TracerExecutionStrategy(execOptions) 
	{ assert(false && " not implemented");}
	virtual void executeTracerSymbolically(const InputPayload& payload, PathConstraint &outPathConstraint) override {}
	virtual bool executeTracerTracking(InputPayload& input) override {}

	virtual ExecutionState* getExecutionState() {return nullptr;}
};

#endif


