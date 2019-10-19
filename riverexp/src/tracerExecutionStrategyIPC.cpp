#include "inputpayload.h"
#include "constraints.h"
#include "tracerExecutionStrategyIPC.h"
#include <set>
#include <assert.h>
#include <string.h>
#include "utils.h"

#include <sys/socket.h> 
#include <netinet/in.h> 
#include <sys/un.h>
#include <semaphore.h>  /* Semaphore */
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <fstream>
#include <unistd.h>

#include "BinFormatConcolic.h"

void TracerExecutionStrategyIPC::init()
{
    // Create one input/output buffer for each worker
    m_lastTracerInputBuffer = new char[m_execOptions.MAX_TRACER_INPUT_SIZE];
	m_lastTracerOutputBuffer = new char[m_execOptions.MAX_TRACER_OUTPUT_SIZE];
	m_lastTracerOutputSize   = 0;

	m_execState.m_syncSemaphore = sem_open("/concolicSem", O_CREAT, S_IRUSR | S_IWUSR, 0);
	assert (m_execState.m_syncSemaphore > 0 && "Couldn't create the semaphpre");

	assert(m_execOptions.m_numProcessesToUse == 1 && "In the current version we support a single tracer process");
	handshakeWithSymbolicTracer();
}

void TracerExecutionStrategyIPC::handshakeWithSymbolicTracer()
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wwrite-strings"
    char* const tracerProgramPath = "/usr/local/bin/river.tracer";
    char * const tracerArgv[] = { tracerProgramPath, "-p", (char*)m_execOptions.testedLibrary, "--annotated", "--z3", "--flow", "--addrName", SOCKET_ADDRESS_COMM, "--exprsimplify", (char*)0};
    char * const tracerEnviron[] = { "LD_LIBRARY_PATH=/usr/local/lib/", (char*)0 };
#pragma GCC diagnostic pop

    bool doServerJob = true;
    //m_execState..push_back()
    if (!m_execOptions.spawnTracersManually)
    {
		// Spawn the requested number of tracers by hand
		for (int i = 0 ; i < m_execOptions.m_numProcessesToUse; i++)
		{
			const int nTracerProcessId = fork();
			if (0 == nTracerProcessId) 
			{
				doServerJob = false;
				// run child process image
			
				// Wait for server to be ready with socket created and listening
				sem_wait(m_execState.m_syncSemaphore);
				int nResult = execve(tracerProgramPath, tracerArgv, tracerEnviron);
				if (nResult < 0)
				{
					printf("1 Oh dear, something went wrong with execve! %s. command %s \n", strerror(errno), tracerProgramPath );
				}
				else
				{
					printf("Successfully started child process\n");
				}
			
				// if we get here at all, an error occurred, but we are in the child
				// process, so just exit
				exit(nResult);
			}
			else
			{
				m_execState.m_tracersPID.push_back(nTracerProcessId);
				doServerJob = true;
			}
		}
    }

    if (doServerJob)
    {
        unlink(SOCKET_ADDRESS_COMM);

        // Communicate with the tracer process just spawned through sockets

        // Step 1: create a socket, bind it to a common address then listen.
        if ((m_execState.m_serverSocket = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) 
        {
            perror("server: socket");
            exit(1);
        }

        struct sockaddr_un saun, fsaun;
        saun.sun_family = AF_UNIX;
        strcpy(saun.sun_path, SOCKET_ADDRESS_COMM);
        unlink(SOCKET_ADDRESS_COMM);
        const int len = sizeof(saun.sun_family) + strlen(saun.sun_path);

        if (bind(m_execState.m_serverSocket, (struct sockaddr *)&saun, len) < 0) 
        {
            perror("server: bind");
            exit(1);
        }

        if (listen(m_execState.m_serverSocket, m_execOptions.m_numProcessesToUse) < 0) 
        {
              perror("server: listen");
              exit(1);
        }

		// Inform all tracers that we can start and wait for each
		for (int i = 0; i < m_execOptions.m_numProcessesToUse; i++)
		{
			sem_post(m_execState.m_syncSemaphore);

			// Accept a new client. 
			// TODO: for multiple clients, create a thread to handle each individual client
			int fromLen = 0;
			int clientSocket = -1;
			if ((clientSocket = accept(m_execState.m_serverSocket, (struct sockaddr *)&fsaun, (socklen_t*)&fromLen)) < 0) 
			{
				perror("server: accept");
				exit(1);
			}

			FILE* socketReadStream = fdopen(clientSocket, "rb");
			if (socketReadStream == 0) 
			{
				perror("can't open socker as read stream");
				exit(1);
			}
			IPCWorkerInfo newWorker;
			newWorker.socket = clientSocket;
			newWorker.socketReadStream = socketReadStream;
			m_execState.m_workers.push_back(newWorker);
		}
    }
}

void TracerExecutionStrategyIPC::executeTracerSymbolically(const InputPayload& payload, PathConstraint &outPathConstraint)
{
    // TODO: multi job here !
	const int workerToSendThisTaskTo = 0;
	IPCWorkerInfo& worker = m_execState.m_workers[workerToSendThisTaskTo];

    // Serialize the task [task_size | content]
	sendTaskMessageToWorker(worker, payload.input.size(), payload.input.data());

    // Get the result back for this task
    int responseSize = -1;
    fread(&responseSize, sizeof(int), 1, worker.socketReadStream);

#ifdef SHOW_LOGS
	printf("Response task size %d\n", responseSize);
#endif
    fread(m_lastTracerOutputBuffer, sizeof(char), responseSize, worker.socketReadStream);
	m_lastTracerOutputSize = responseSize;
#ifdef SHOW_LOGS
    printf("Response: size %d. content %s\n", responseSize, m_lastTracerOutputBuffer);
#endif

	ConcolicExecutionResult execResult;
	execResult.deserializeFromStream(m_lastTracerOutputBuffer, m_lastTracerOutputSize - sizeof(int));
	Utils::convertExecResultToPathConstraint(execResult, outPathConstraint);
}

void TracerExecutionStrategyIPC::sendTaskMessageToWorker(IPCWorkerInfo& worker, const int size, const unsigned char* content)
{
	int totalSize = 0;
    *(int*)(&m_lastTracerInputBuffer[0]) = size;
	if (size != TERMINATION_TAG)
	{
    	memcpy(&m_lastTracerInputBuffer[sizeof(int)], content, sizeof(content[0])*size);
		totalSize = sizeof(int) + sizeof(content[0])*size;
	}
	else
	{
		totalSize = sizeof(int);
	}	
          
    // Send it over the network

#ifdef SHOW_LOGS
    printf("Send task to worker 0 size %d. content %s\n", totalSize, &m_lastTracerInputBuffer[sizeof(int)]);
#endif
    send(worker.socket, m_lastTracerInputBuffer, totalSize, 0);
}


TracerExecutionStrategyIPC::~TracerExecutionStrategyIPC()
{
	closeConnections();

	delete m_lastTracerOutputBuffer;
	delete m_lastTracerInputBuffer;
}


// TODO:
void TracerExecutionStrategyIPC::closeConnections()
{
	// Send termination messages
	for (IPCWorkerInfo& worker : m_execState.m_workers)
	{
		sendTaskMessageToWorker(worker, -1, nullptr);
	}

	// Wait all spawned processes first
    if (!m_execOptions.spawnTracersManually)
    {
		for (const int pid : m_execState.m_tracersPID)
		{
			int status = -1;
			int w = waitpid(pid, &status, WUNTRACED | WCONTINUED);
			if (w == -1) 
			{
				perror("waitpid");
				exit(EXIT_FAILURE);
			}
		}
    }

	// Close their socket and server socket
	for (IPCWorkerInfo& worker : m_execState.m_workers)
	{
		for (IPCWorkerInfo& worker : m_execState.m_workers)
		{
			close(worker.socket);
			//close(worker.socketReadStream);
		}
		close(m_execState.m_serverSocket);
	}
}


bool TracerExecutionStrategyIPC::executeTracerTracking(InputPayload& input)
{
    // TODO: same as above code for _external, but execute simpletracer with IPC communication not textual !
	return true;
}
