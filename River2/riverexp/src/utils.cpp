#include "utils.h"
#include "constraints.h"
#include "BinFormatConcolic.h"
#include <sstream>

// Using a stack allocated tempStringStream, you can covert multiple values from dword to strings
// Q: why stream is not static ? because multiple threads might want to access it
const void setDWORDAsHexString(std::stringstream& tempStringStream, const DWORD value, std::string& outStr)
{
    tempStringStream.clear();
    tempStringStream.str("");
    tempStringStream << std::hex << value;
    outStr = tempStringStream.str(); //std::move(tempStringStream.str());
}

void Utils::convertExecResultToPathConstraint(const ConcolicExecutionResult& inExecResults, PathConstraint& outPathConstraint)
{
    // Resize then copy each test
    outPathConstraint.constraints.resize(inExecResults.m_tests.size());

    for (int i = 0 ; i <inExecResults.m_tests.size(); i++)
    {
        const SingleTestDetails& testResult = inExecResults.m_tests[i];
        Test& outTest                       = outPathConstraint.constraints[i];

        // Ugly copy but we'll get soon to binary Z3 and avoid working with strings..
        if (Utils::MANUALLY_ADD_ASSERT_PREFIX)
        {
            outTest.Z3_code.assign("(assert ");
        }
        outTest.Z3_code.append(testResult.ast.address, testResult.ast.size);
        if (Utils::MANUALLY_ADD_ASSERT_PREFIX)
        {
            outTest.Z3_code.append(")");
        }


        std::stringstream hexConvStream;
        outTest.isInverted = false;
        outTest.was_taken = testResult.taken;
        outTest.variables.clear();
        std::copy(testResult.indicesOfInputBytesUsed.begin(), 
                  testResult.indicesOfInputBytesUsed.end(), 
                  std::back_inserter(outTest.variables));


        outTest.pathBlockAddresses.resize(testResult.pathBBlocks.size());
        int j = 0;
        for (const DWORD pathEntry : testResult.pathBBlocks)
        {
            setDWORDAsHexString(hexConvStream, pathEntry, outTest.pathBlockAddresses[j]);
            j++;
        }

        setDWORDAsHexString(hexConvStream, testResult.parentBlock, outTest.test_address);
        setDWORDAsHexString(hexConvStream, testResult.blockOptionTaken, outTest.taken_address);
        setDWORDAsHexString(hexConvStream, testResult.blockOptionNotTaken, outTest.notTaken_address);
    }

    // Now unify all individual variables
    mergeAllVariables(outPathConstraint);
}

void Utils::mergeAllVariables(PathConstraint& outPathConstraint)
{
	// 0. Add all different variables used by all individual constraints
	outPathConstraint.variables.clear();
	for(const Test& test : outPathConstraint.constraints) 
	{
		std::copy(test.variables.begin(), test.variables.end(), std::inserter(outPathConstraint.variables, outPathConstraint.variables.end()));
	}
}
