#ifndef TRACER_EXECUTION_STRATEGY_H
#define TRACER_EXECUTION_STRATEGY_H

#include "concolicDefs.h"

class InputPayload;
class PathConstraint;

class TracerExecutionStrategy
{
public:
    // Executes the tracer with a given input and fill out the path constraint
    TracerExecutionStrategy(const ExecutionOptions& execOptions) : m_execOptions(execOptions){}
	virtual void executeTracerSymbolically(const InputPayload& payload, PathConstraint &outPathConstraint) = 0;

    // This function executes the library under test against input and:
    // fills the score inside 
    // reports any errors, crashes etc.
    // Returns true if the input is ok, false otherwise
	virtual bool executeTracerTracking(InputPayload& input) = 0;

    // Maybe strategies want to lazy initialize resources..
    virtual void init() {}

    virtual ExecutionState* getExecutionState() = 0;

protected:
    ExecutionOptions m_execOptions;
};

#endif


