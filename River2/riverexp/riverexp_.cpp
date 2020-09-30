// A simple program that computes the square root of a number
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "RiverexpConfig.h"
#include <z3.h>

#ifdef USE_MYMATH
#include "MathFunctions.h"
#endif

#include <iostream>
#include <assert.h>
#include"z3++.h"
using namespace z3;

class Test{};

void example1()
{
    context c;
    expr x = c.int_const("x");
    expr y = c.int_const("y");
    solver s(c);

    s.add(x >= 1);
    s.add(y < x + 3);
    std::cout<<s.check() <<"\n";

    model m = s.get_model();
    std::cout<<m<<"\n";

    // traverse the model
    for (unsigned i = 0; i < m.size(); i++)
    {
      func_decl v = m[i];
      assert(v.arity() == 0);
      std::cout<<v.name() << " = " << m.get_const_interp(v) <<"\n";
    }

    std::cout<<"X + y + 1 = " << m.eval(x + y + 1) << "\n";
}

void example2()
{
    std::cout<<"bit vector example \n";
    context c;
    expr x = c.bv_const("x", 32);
    expr y = c.bv_const("y", 32);
    solver s(c);

    s.add((x ^ y) - 103 == x * y);
    std::cout<<s <<"\n";
    std::cout<<s.check() <<"\n";
    std::cout<<s.get_model() <<"\n";

    Z3_string res = Z3_solver_to_string(c, s);

    context c2;
    solver s2(c2);
    Z3_solver_from_string(c2, s2, res);
    std::cout<<s2<<"\n";
}

void example3()
{
    std::cout<<"two contexts\n";
    context c1, c2;

    expr x = c1.int_const("x");
    expr n = x + 1;
  std::cout<<Z3_translate(c1, n, c2)<<"\n";

    expr n1 = to_expr(c2, Z3_translate(c1, n, c2));
    std::cout<< n1 <<"\n";
}

void opt_example() 
{
    context c;
    optimize opt(c);
    params p(c);
    p.set("priority",c.str_symbol("pareto"));
    opt.set(p);
    expr x = c.int_const("x");
    expr y = c.int_const("y");
    opt.add(10 >= x && x >= 0);
    opt.add(10 >= y && y >= 0);
    opt.add(x + y <= 11);
    optimize::handle h1 = opt.maximize(x);
    optimize::handle h2 = opt.maximize(y);
    while (true) {
        if (sat == opt.check()) {
            std::cout << x << ": " << opt.lower(h1) << " " << y << ": " << opt.lower(h2) << "\n";
        }
        else {
            break;
        }
    }
}

void parse_string() 
{
    std::cout << "parse string\n";
    z3::context c;
    z3::solver s(c);
    //Z3_solver_from_string(c, s, "(declare-const x Int)(assert (> x 10))");
    s.from_string("(declare-const x Int)(assert (> x 10))");
    std::cout << s.check() << "\n";
}

void string_values() 
{
    context c;
    expr s = c.string_val("abc\n\n\0\0", 7);
    std::cout << s << "\n";
    std::string s1 = s.get_string();
    std::cout << s1 << "\n";
}

#include <vector>
#include "BinFormatConcolic.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h> 
#include <sys/types.h>   
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <assert.h>

#define SOCKET_ADDRESS_COMM "/home/ciprian/socketriver"
#define NUM_MAX_CONNECTIONS 5
void simulateTracerChild() 
{
    const bool tracerIsSpawnedExternally = false; // True if you want to launch the tracer process externally

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wwrite-strings"
    char* const tracerProgramPath = "/usr/local/bin/river.tracer";
    char * const tracerArgv[] = { tracerProgramPath, "-p", "libfmi.so", "--annotated", "--z3", "--flow", "--addrName", SOCKET_ADDRESS_COMM, "--exprsimplify", nullptr};
    char * const tracerEnviron[] = { "LD_LIBRARY_PATH=/usr/local/lib/", };
#pragma GCC diagnostic pop

    bool doServerJob = true;
    int nTracerProcessId = -1;
    if (!tracerIsSpawnedExternally)
    {
        nTracerProcessId = fork();
        if (0 == nTracerProcessId) 
        {
          doServerJob = false;
          // run child process image
          sleep(5);
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
          doServerJob = true;
        }
    }

    if (doServerJob)
    {
        unlink(SOCKET_ADDRESS_COMM);

        // Communicate with the tracer process just spawned through sockets

        // Step 1: create a socket, bind it to a common address then listen.
        int serverSocket = -1;
        if ((serverSocket = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) 
        {
            perror("server: socket");
            exit(1);
        }

        struct sockaddr_un saun, fsaun;
        saun.sun_family = AF_UNIX;
        strcpy(saun.sun_path, SOCKET_ADDRESS_COMM);
        unlink(SOCKET_ADDRESS_COMM);
        const int len = sizeof(saun.sun_family) + strlen(saun.sun_path);

        if (bind(serverSocket, (struct sockaddr *)&saun, len) < 0) 
        {
            perror("server: bind");
            exit(1);
        }

        if (listen(serverSocket, NUM_MAX_CONNECTIONS) < 0) 
        {
              perror("server: listen");
              exit(1);
        }

        // Step 2: accept a new client. 
        // TODO: for multiple clients, create a thread to handle each individual client
        int clientSocket = 0;
        int fromLen = 0;
        if ((clientSocket = accept(serverSocket, (struct sockaddr *)&fsaun, (socklen_t*)&fromLen)) < 0) 
        {
              perror("server: accept");
              exit(1);
        }


        FILE* fp = fdopen(clientSocket, "rb");
        if (fp == 0) {
             perror("can't open");
              exit(1);
          }


        // Simulate some tasks sending/receiving with the tracer process
        const int MAX_SEND_BUFFER_SIZE = 1024;
        const int MAX_RECV_BUFFER_SIZE = 1024;
        char SEND_BUFFER[MAX_SEND_BUFFER_SIZE]={0};
        char RESPONSE_BUFFER[MAX_RECV_BUFFER_SIZE]={0};

        // Create some task samples and their sizes. The last one is the termination marker
        const int numTasks = 3;
        const char* taskContent[numTasks] = {"ABCD", "DEFGH", ""};
        const int taskSizes[numTasks] = {5,6,-1};

        for (int i = 0; i < numTasks; i++)
        {
            // Serialize the task [task_size | content]
            *(int*)(&SEND_BUFFER[0]) = taskSizes[i];
            const bool isTerminationTask = taskSizes[i] == -1;

            if (!isTerminationTask)
            {
                memcpy(&SEND_BUFFER[sizeof(int)], taskContent[i], sizeof(char)*taskSizes[i]);
          
                // Send it over the network
                printf("Send task %d: size %d. content %s\n", i, taskSizes[i], &SEND_BUFFER[sizeof(int)]);
                send(clientSocket, SEND_BUFFER, taskSizes[i] + sizeof(int), 0);
            }
            else
            {
                printf("Sending termination task\n");
                send(clientSocket, SEND_BUFFER, sizeof(int), 0);
            }
        
            if (!isTerminationTask)
            {
                // Get the result back for this task
                int responseSize = -1;
                fread(&responseSize, sizeof(int), 1, fp);
                fread(RESPONSE_BUFFER, sizeof(char), responseSize, fp);

                printf("Response task %d: size %d. content %s\n", i, responseSize, RESPONSE_BUFFER);
            }
        }

        // Wait the process if spawned from here
        if (!tracerIsSpawnedExternally)
        {
            int status = -1;
            int w = waitpid(nTracerProcessId, &status, WUNTRACED | WCONTINUED);
            if (w == -1) {
                perror("waitpid");
                exit(EXIT_FAILURE);
            }
        }
    }

}


void doZ3Experiments()
{
    unsigned major, minor, build, revision;
    Z3_get_version(&major, &minor, &build, &revision);
    printf("Z3 %d.%d.%d.%d\n", major, minor, build, revision);

    example2();

    //example1();
    //example2();
    //example3();
    //opt_example();

    parse_string();
    string_values();
}

int main (int argc, char *argv[])
{
#if 0
   char c;
    FILE *fp;
    int fromlen;
    register int i, s, ns, len;
    struct sockaddr_un saun, fsaun;

    unlink(SOCKET_ADDRESS_COMM);

    /*
     * Get a socket to work with.  This socket will
     * be in the UNIX domain, and will be a
     * stream socket.
     */
    if ((s = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
        perror("server: socket");
        exit(1);
    }

    /*
     * Create the address we will be binding to.
     */
    saun.sun_family = AF_UNIX;
    strcpy(saun.sun_path, SOCKET_ADDRESS_COMM);

    /*
     * Try to bind the address to the socket.  We
     * unlink the name first so that the bind won't
     * fail.
     *
     * The third argument indicates the "length" of
     * the structure, not just the length of the
     * socket name.
     */
    unlink(SOCKET_ADDRESS_COMM);
    len = sizeof(saun.sun_family) + strlen(saun.sun_path);

    if (bind(s, (struct sockaddr *)&saun, len) < 0) {
        perror("server: bind");
        exit(1);
    }

    /*
     * Listen on the socket.
     */
    if (listen(s, NUM_MAX_CONNECTIONS) < 0) {
        perror("server: listen");
        exit(1);
    }

    /*
     * Accept connections.  When we accept one, ns
     * will be connected to the client.  fsaun will
     * contain the address of the client.
     */
    if ((ns = accept(s, (struct sockaddr *)&fsaun, (socklen_t*)&fromlen)) < 0) {
        perror("server: accept");
        exit(1);
    }

    printf("connected with %d\n", ns);
#endif


#if 0
    doZ3Experiments();
#endif

#if 1
    simulateTracerChild();
#endif

  return 0;
}

