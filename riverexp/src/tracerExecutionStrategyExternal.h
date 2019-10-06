#ifndef TRACER_EXECUTION_STRATEGY_EXTERNAL_H
#define TRACER_EXECUTION_STRATEGY_EXTERNAL_H

#include "tracerExecutionStrategy.h"
#include "concolicDefs.h"

class InputPayload;
class PathConstraint;
class ConcolicExecutor;

// External textual execution of tracer - must be used only for debug purposes
// This is the "debug" version, with the tracer being invoked in an external process and reading the output generated file
// TERRIBLE PERFORMANCE - use only for debugging
class TracerExecutionStrategyExternal : public TracerExecutionStrategy
{
public:

    // Executes the tracer with a given input and fill out the path constraint
    TracerExecutionStrategyExternal(const ExecutionOptions& execOptions) : TracerExecutionStrategy(execOptions) { }
	virtual void executeTracerSymbolically(const InputPayload& payload, PathConstraint &outPathConstraint) override;

    // This function executes the library under test against input and:
	// fills the score inside 
	// reports any errors, crashes etc.
	virtual void executeTracerTracking(InputPayload& input) override;

    virtual ExecutionState* getExecutionState() { return &m_execState;}

private:
	ExecutionState 	 m_execState;
};

#endif


