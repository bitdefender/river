#include "concolicExecutor.h"
#include "utils.h"
#include <string.h>
#include <set>
#include <iostream>
#include <fstream>

////////
// Some global definitions & configs

// If this is not commented, it will output all possible solutions in the given file
#define OUTPUT_ALL_GENERATED_TESTS 

// If below is enabled, we just take the textual output saved in the local folder and run over it.
// Usefull to debug things
//#define SIMULATE_TRACER_EXECUTION

// Enable this to append (assert X)  before any let instruction read from output file
#define MANUALLY_ADD_ASSERT_PREFIX


ConcolicExecutor::ConcolicExecutor(const int MAX_TRACER_OUTPUT_SIZE, const char* testedLibrary)
: m_MAX_TRACER_OUTPUT_SIZE(MAX_TRACER_OUTPUT_SIZE)
{
	m_lastTracerOutputBuffer = new char[m_MAX_TRACER_OUTPUT_SIZE];
	m_lastTracerOutputSize   = 0;
	m_testedLibrary = testedLibrary;
}

ConcolicExecutor::~ConcolicExecutor()
{
	delete m_lastTracerOutputBuffer;
}

// Executes the tracer with a given input and fill out the path constraint
// This is the "debug" version, with the tracer being invoked in an external process and reading the output generated file
// TERRIBLE PERFORMANCE - use only for debugging
void ConcolicExecutor::executeTracerSymbolically_external(const InputPayload& payload, PathConstraint &outPathConstraint) 
{
	outPathConstraint.reset();
#ifndef SIMULATE_TRACER_EXECUTION
    // Step 1: call tracer with the given input to obtain the trace simple output file
	//----------
	//printf("System call to RIVER is starting \n");
	// We put the input to a file because we can't write leading 0's with python pipe trick (it interprets them as the end of the string)
	{
		std::string command("");
/*
		for (unsigned char chr : payload.input)
		{
			command.append(1, chr);
		}
*/
		FILE* f = fopen("./sampleinput.txt", "wb");
		assert(f && "can't create the file sampleinput.txt to write some input");
		fwrite(payload.input.data(), sizeof(payload.input[0]), payload.input.size(), f);
		fclose(f);
		
		command.append("river.tracer -p ");
		command.append(m_testedLibrary);
		command.append(" --annotated --z3 --exprsimplify < ./sampleinput.txt");
		int st = system(command.c_str());
	}
	//printf("System call to RIVER finished \n");
	//------------
#endif

	// Step 2: parse the produced output file and fill an array with all test constraints encountered during the execution
	//----------
	{
		FILE * fp = fopen("trace.simple.out", "r");
	    if (fp == nullptr)
        {
            printf("ERROR: there was no trace.simple.out file produced !");
	        exit(EXIT_FAILURE);
        }

	    enum TextParserState : char
	    {
	    	STATE_WAITING_TEST = 0,
	    	STATE_VARIABLES,
	    	STATE_BLOCKS_PATH,
	    	STATE_Z3_CODE,
	    };

		bool isNewTestCase 			= true;
		TextParserState parserState = STATE_WAITING_TEST;
	    ssize_t read 				= 0;
	    size_t len 					= 1024;	// Initial len a line buffer
	    char * line 				= (char*) malloc(len);
	    Test* currentTest 			= nullptr;
	    bool skipNextLineRead 		= false;

		while (true) 
		{
			if (!skipNextLineRead)
			{
				read = getline(&line, &len, fp);
				if (read == -1)
					break;
			}
			else
			{
				skipNextLineRead = false;
			}

			std::vector<std::string> tokens = Utils::splitStringBySeparator(line, ' ');
			if (tokens.empty())
				{
					break;
				}

				if (tokens[0].compare("\n") == 0)
					continue;

				switch(parserState)
				{
					case STATE_WAITING_TEST:
						{
							if(tokens[0].compare("Test:") == 0) 
							{
								outPathConstraint.constraints.emplace_back();
								currentTest = &outPathConstraint.constraints.back();
								currentTest->test_address = tokens[1];
								currentTest->taken_address = tokens[4].substr(0, tokens[4].size()-1); // removing also the last character
								currentTest->notTaken_address = tokens[6].substr(0, tokens[6].size()-1);

								// Remove the last character if endfile
								std::string& lastString = tokens.back();
								if (!lastString.empty() && (lastString.back() == '\n' || tokens.back().back() == '\r'))
								{
									lastString = lastString.substr(0, lastString.length() - 1);
								}

								if(lastString.compare("No") == 0) 
								{
									currentTest->was_taken = false;
								}
								else 
								{
									currentTest->was_taken = true;
								}

								parserState = STATE_VARIABLES;
							}
							else
							{
								assert(false && "Expecting a test entry line");
								break;
							}
							break;
						}
						

					case STATE_VARIABLES:
						{
							const int numVariables = atoi(tokens[0].c_str());
							for (int i = 1; i <= numVariables; i++)
							{
								const int r = atoi(tokens[i].c_str());
								currentTest->variables.push_back(r);
							}

							parserState = STATE_BLOCKS_PATH;
						
							break;
						}

					case STATE_BLOCKS_PATH:
						{
							const int numBlocksOnPath = atoi(tokens[0].c_str());
							for (int i = 1; i <= numBlocksOnPath; i++)
							{								
								currentTest->pathBlockAddresses.push_back(tokens[i]);
							}

							parserState = STATE_Z3_CODE;
						}
						//break;

					case STATE_Z3_CODE:
						{
							// Read the Z3 code line by line until we get to a new Test: line
							while ((read = getline(&line, &len, fp)) != -1)
							{
								if (read > 4 && memcmp(line, "Test:", 4) == 0)
								{

#ifdef MANUALLY_ADD_ASSERT_PREFIX
									currentTest->Z3_code.append(")"); // For the assert end
#endif

									// Then prepare for the next test
									skipNextLineRead = true;
									parserState = STATE_WAITING_TEST;
									break;
								}
								else if (read > 0)
								{
									// Eliminate the end of the line 
									if (line[read-1] == '\n' || line[read-1] == '\r')
										line[read-1] = '\0';

									if (currentTest->Z3_code.empty())
									{
#ifdef MANUALLY_ADD_ASSERT_PREFIX
										// Add the (assert ) in front of the z3 code condition
										currentTest->Z3_code.append("(assert ");
#endif
									}

									currentTest->Z3_code.append(line);
								}
							}
						parserState = STATE_WAITING_TEST; // Redundant because it will exit switch however, but better for clarity
						break;
					}
			};
	    }
	    if (parserState != STATE_WAITING_TEST)
		{
			assert(false && "Invalid termination while reading the output log");
		}
		else
		{
			// Add last ')' if manually adding the assert in front of code
#ifdef MANUALLY_ADD_ASSERT_PREFIX
			if (currentTest && !currentTest->Z3_code.empty())
			{
				currentTest->Z3_code.append(")"); // For the assert end
			}
#endif
		}
		
	    // Post process the path constraint
	    // 0. Add all different variables used by all individual constraints
		outPathConstraint.variables.clear();
	    for(const Test& test : outPathConstraint.constraints) 
		{
			std::copy(test.variables.begin(), test.variables.end(), std::inserter(outPathConstraint.variables, outPathConstraint.variables.end()));
		}
	    fclose(fp);
	    if (line)
	        free(line);
	}
}
// This function executes the library under test against input and:
// fills the score inside 
// reports any errors, crashes etc.
void ConcolicExecutor::executeTracerTracking_external(InputPayload& input)
{
	// TODO: run m_testedLibrary with basic block tracking
	// 1. Get errors
	// 2. Fill the different addresses of blocks touched by executing the given input payload to score with the code below
	std::set<int> blocksTouchedByInput;
	// Score it
	int score = 0;
	for(const int blockAlreadyTouched : m_blockAddresesTouched) 
	{
		const bool is_in = blocksTouchedByInput.find(blockAlreadyTouched) != blocksTouchedByInput.end();
		if(is_in == false) 
		{
			m_blockAddresesTouched.insert(blockAlreadyTouched);
			score++;
		}
	}
	input.score = score;
}

// Run the input symbolically, negate the constraints one by one and get new inputs by solving them with the SMT
// Returns in the out variable the input childs of the base input parameter
void ConcolicExecutor::ExpandExecution(const InputPayload& input, std::vector<InputPayload>& outGeneratedInputChildren) 
{
	//execute and fill the path constraints for this input
	PathConstraint pathConstraint;
	executeTracerSymbolically_external(input, pathConstraint);
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

void ConcolicExecutor::searchSolutions(const ArrayOfUnsignedChars& startInput, const bool outputAllCoverageInputs, const char* fileToOutputCoverageInputs) 
{
	// Set the first input received and add it to the worklist
	InputPayload initialInput;
	initialInput.input = startInput;
	initialInput.bound = -1;
	m_workList.push(std::move(initialInput));

	std::vector<std::vector<unsigned char>> coverageInputs; 

	// Perform a search inside the existing tree of tasks
	while(!m_workList.empty())  
	{
		// Take the top scored path in the worklist - note that this is not executed yet and its constraints are not valid 
		const InputPayload inputPicked(std::move(m_workList.top()));
		m_workList.pop();

        if (outputAllCoverageInputs)
        {
		    coverageInputs.push_back(inputPicked.input);
        }

		// Execute tracer and library using this input and produce the children input
		// Expand the input by negating branch test along the path
		m_outGeneratedInputChildren.clear();
		ExpandExecution(inputPicked, m_outGeneratedInputChildren);
		// Iterate over all children input, run & check them, score then add to the worklist
		for (InputPayload& payloadChildren : m_outGeneratedInputChildren)
		{
			// TODO: get results and report potential problems somewhere
			executeTracerTracking_external(payloadChildren); 
			m_workList.push(payloadChildren);
		}
	}
    
    if (outputAllCoverageInputs)
    {
        std::ofstream newfile(fileToOutputCoverageInputs);
        for (const std::vector<unsigned char> &s : coverageInputs) 
        {
			for (unsigned char chr : s)
            	newfile << (int)chr <<" ";
			newfile << std::endl;
        }
    }
}
