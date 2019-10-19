#include "../src/concolicExecutor.h"
#include <string>
#include "../src/ezOptionParser.h"

#define USE_IPC

void setupOptions(ez::ezOptionParser& opt, int argc, const char *argv[])
{
	opt.overview 	= "Concolic river";
	opt.syntax 		= "riverexp [OPTIONS]";
	opt.example		= "riverexp -p libfmi.so --numProcs 1 --outformat text --outfilter";

	opt.add(
			"",  // Default.
			1,   // Required?
			1,   // Number of args expected.
			0,   // Delimiter if expecting multiple args.
			"Set the payload file", // Help description.
			"-p",
			"--payload"
		   );

	opt.add(
			"",
			1,
			1,
			0,
			"Set the out format: either binary or text. If binary, then a binary file will be generated for each produced input",
			"-of",
			"--outformat"
		   );

	opt.add(
			"",
			0,
			0,
			0,
			"If enabled, this will output only interesting inputs. If not, all produced inputs are shown",
			"-ef",
			"--outfilter"
		   );
	
	opt.add(
			"",
			0,
			0,
			0,
			"If enabled, this will wait for simpletracer processes to be spawned manually.",
			"-ms"
			"--manualtracers"
		   );


	opt.add(
			"1024",
			1,
			1,
			0,
			"Maximum size (bytes) for input given to tracer",
			"-mi",
			"--maxInputSize"
		   );

	opt.add(
			"10000000",
			1,
			1,
			0,
			"Maximum size (bytes) for output given to tracer",
			"-mo",
			"--maxOutputSize"
		   );

	opt.add(
			"1",
			1,
			1,
			0,
			"Number of processes for distributed concolic execution",
			"-nP",
			"--numProcs"
		   );


	opt.parse(argc, argv);
}

int main(int argc, const char *argv[])
{
	ez::ezOptionParser opt;
	setupOptions(opt, argc, argv);

	ExecutionOptions execOp;

	std::string testedLibrary;
	opt.get("-p")->getString(testedLibrary);
	execOp.testedLibrary = testedLibrary.c_str();

	std::string inputSizeStr, outputSizeStr;
	opt.get("--maxInputSize")->getString(inputSizeStr);
	opt.get("--maxOutputSize")->getString(outputSizeStr);
	execOp.MAX_TRACER_INPUT_SIZE = atoi(inputSizeStr.c_str());
	execOp.MAX_TRACER_OUTPUT_SIZE = atoi(outputSizeStr.c_str());

	std::string numProcsStr;
	opt.get("--numProcs")->getString(numProcsStr);
	execOp.m_numProcessesToUse = atoi(numProcsStr.c_str());
	
	// Output options
	{
		if (opt.isSet("--outformat"))
		{
			std::string outFormatStr;
			opt.get("--outformat")->getString(outFormatStr);
			if (outFormatStr == "text")
			{
				execOp.outputOptions = ExecutionOptions::OPTION_TEXT;
			}
			else if (outFormatStr == "binary")
			{
				execOp.outputOptions = ExecutionOptions::OPTION_BINARY;
			}
			else
			{
				fprintf(stderr, "Invalid option for outformat: %s\n", outFormatStr.c_str());
			}
		}
		else
		{
			// Default:
			execOp.outputOptions = ExecutionOptions::OPTION_TEXT;
		}
	}

	if (opt.isSet("--outfilter"))
	{
		execOp.outputOptions |= ExecutionOptions::OPTION_FILTER_NON_INTERESTING;
	}

	execOp.spawnTracersManually = false;
	if (opt.isSet("--manualtracers"))
	{
		execOp.spawnTracersManually = true;
	}

#ifdef USE_IPC
	execOp.m_execType = ExecutionOptions::EXEC_DISTRIBUTED_IPC;
	ConcolicExecutor cexec(execOp);
#else
	execOp.m_execType = ExecutionOptions::EXEC_SERIAL;
	ConcolicExecutor cexec("libfmi.so");
#endif
	ArrayOfUnsignedChars payloadInputExample = {'g', 'o', 'o', 'd'};
	cexec.searchSolutions(payloadInputExample);

	return 0;
}

