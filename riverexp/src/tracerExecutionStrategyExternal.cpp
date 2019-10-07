#include "inputpayload.h"
#include "constraints.h"
#include "tracerExecutionStrategyExternal.h"
#include <set>
#include <assert.h>
#include <string.h>
#include "utils.h"
#include "concolicExecutor.h"

void TracerExecutionStrategyExternal::executeTracerSymbolically(const InputPayload& payload, PathConstraint &outPathConstraint)
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
		command.append(m_execOptions.testedLibrary);
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

									if (Utils::MANUALLY_ADD_ASSERT_PREFIX)
									{
										currentTest->Z3_code.append(")"); // For the assert end
									}

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
										if (Utils::MANUALLY_ADD_ASSERT_PREFIX)
										{
											// Add the (assert ) in front of the z3 code condition
											currentTest->Z3_code.append("(assert ");
										}
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
			if (Utils::MANUALLY_ADD_ASSERT_PREFIX)
			{
				if (currentTest && !currentTest->Z3_code.empty())
				{
					currentTest->Z3_code.append(")"); // For the assert end
				}
			}
		}
		
	    // Post process the path constraint
		Utils::mergeAllVariables(outPathConstraint);

	    fclose(fp);
	    if (line)
	        free(line);
	}
}

void TracerExecutionStrategyExternal::executeTracerTracking(InputPayload& input)
{
	// TODO: run m_testedLibrary with basic block tracking and take the output text generated
	// 1. Get errors
	// 2. Fill the different addresses of blocks touched by executing the given input payload to score with the code below
	std::set<int> blocksTouchedByInput;
	// Score it
	int score = 0;
	for(const int blockAlreadyTouched : m_execState.m_blockAddresesTouched) 
	{
		const bool is_in = blocksTouchedByInput.find(blockAlreadyTouched) != blocksTouchedByInput.end();
		if(is_in == false) 
		{
			m_execState.m_blockAddresesTouched.insert(blockAlreadyTouched);
			score++;
		}
	}
	
	input.score = score;
}
