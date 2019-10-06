#ifndef CONSTRAINTS_H
#define CONSTRAINTS_H

#include <string>
#include <vector>
#include <unordered_set>

// This class holds a single test (branch test) that has a jump condition in the assembly code
class Test 
{
public:
	std::string test_address;		// Address (HEXA string all) of the block where this test happens
	std::string taken_address;		// Address of the block where to go next if taken
	std::string notTaken_address;	// Address of the block where to go next if not taken

	std::vector<std::string> pathBlockAddresses;	// The ordered set of hexa addresses of basic blocks encountered by execution from last test up to this test


	bool was_taken = false;			// True if the jump condition associated to this test was taken or not during execution
	std::string Z3_code;			// This contains the Z3 code asserts associated with this test (no jump conditions or variable declarations here !)
	std::vector<int> variables; 	// The different sets of variables used by this test

	bool isInverted = false;		// True if we need to get the inverted condition out of this test

	Test() {}

	void setInverted(const bool value) 
	{
		this->isInverted = value;
	}
};



// This is a full path program with ordered constraints (jumps) encountered during execution
class PathConstraint 
{
public:
	std::vector<Test> constraints;	// The ordered set of tests
	std::unordered_set<int> variables; // Variables aggregated from all individual constraints

	void reset()
	{
		constraints.clear();
		variables.clear();
	}

	PathConstraint() 
	{
	}
};

#endif