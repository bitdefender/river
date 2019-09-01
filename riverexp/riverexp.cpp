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

#define PIPE_READ 0
#define PIPE_WRITE 1

int createChild(const char* szCommand, char* const aArguments[], char* const aEnvironment[], const char* szMessage) 
{
  int aStdinPipe[2];
  int aStdoutPipe[2];
  int nChild;
  char nChar;
  int nResult;

  if (pipe(aStdinPipe) < 0) {
    perror("allocating pipe for child input redirect");
    return -1;
  }
  if (pipe(aStdoutPipe) < 0) {
    close(aStdinPipe[PIPE_READ]);
    close(aStdinPipe[PIPE_WRITE]);
    perror("allocating pipe for child output redirect");
    return -1;
  }

  nChild = fork();
  if (0 == nChild) {
    // child continues here

    // redirect stdin
    if (dup2(aStdinPipe[PIPE_READ], STDIN_FILENO) == -1) {
      exit(errno);
    }

    // redirect stdout
    if (dup2(aStdoutPipe[PIPE_WRITE], STDOUT_FILENO) == -1) {
      exit(errno);
    }

    // redirect stderr
    if (dup2(aStdoutPipe[PIPE_WRITE], STDERR_FILENO) == -1) {
      exit(errno);
    }

    // all these are for use by parent only
    if (close(aStdinPipe[PIPE_READ]) < 0 ||
        close(aStdinPipe[PIPE_WRITE]) < 0 ||
        close(aStdoutPipe[PIPE_READ]) < 0 ||
        close(aStdoutPipe[PIPE_WRITE]) < 0)
    {
      exit(errno);
    }


    // run child process image
    // replace this with any exec* function find easier to use ("man exec")
    nResult = execve(szCommand, aArguments, aEnvironment);
    if (nResult < 0)
    {
      printf("1 Oh dear, something went wrong with execve! %s. command %s \n", strerror(errno), szCommand );
    }
    else
    {
      printf("Successfully started child process\n");
    }
    

    // if we get here at all, an error occurred, but we are in the child
    // process, so just exit
    exit(nResult);
  } else if (nChild > 0) {
    // parent continues here

    // close unused file descriptors, these are for child only
    close(aStdinPipe[PIPE_READ]);
    close(aStdoutPipe[PIPE_WRITE]); 

    //sleep(10);

    
    // Include error check here
    if (NULL != szMessage) {
      int sizeWritten = write(aStdinPipe[PIPE_WRITE], szMessage, strlen(szMessage));
      if (sizeWritten < 0)
      {
         printf("2 Oh dear, something went wrong with read()! %s\n", strerror(errno));
      }
      int err = close(aStdinPipe[PIPE_WRITE]);
      if (err < 0)
      {
         printf("3 Oh dear, something went wrong with read()! %s\n", strerror(errno));
      }
    }
      

    // Just a char by char read here, you can change it accordingly
    printf("starting to write output ..\n");
    while (true)
    {
      int sz = read(aStdoutPipe[PIPE_READ], &nChar, 1);
      if (sz <= 0)
      {
        if (sz < 0)
        {
          printf("4 Oh dear, something went wrong with read()! %s\n", strerror(errno));
        }
        break;
      }
      write(STDOUT_FILENO, &nChar, 1);
    }
    printf("ending to write output ..\n");


    // done with these in this example program, you would normally keep these
    // open of course as long as you want to talk to the child
    close(aStdinPipe[PIPE_WRITE]);
    close(aStdoutPipe[PIPE_READ]);
  } else {
    // failed to create child
    close(aStdinPipe[PIPE_READ]);
    close(aStdinPipe[PIPE_WRITE]);
    close(aStdoutPipe[PIPE_READ]);
    close(aStdoutPipe[PIPE_WRITE]);
  }
  return nChild;
}

int main (int argc, char *argv[])
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wwrite-strings"
   char* const tracerProgramPath = "/usr/local/bin/river.tracer";
   char * const tracerArgv[] = { tracerProgramPath, "-p", "libfmi.so", "--annotated", "--logfile", "log.txt" "--z3", "--outfile", "stdout", nullptr};// "--annotated", "--z3", "--outfile", "stdout", nullptr}; //, "--annotated", "--z3", /*"--logfile", "stdout",*/ "--binlog", "--binbuffered", "--exprsimplify",  "<", "~/testtools/river/benchmarking-payload/fmi/sampleinput.txt", nullptr };
   char * const tracerEnviron[] = { nullptr };
#pragma GCC diagnostic pop
    /*

    int nResult = execve("/bin/ls", newargv, newenviron);
    if (nResult < 0)
    {
      printf("Oh dear, something went wrong with read()! %s\n", strerror(errno));
    }
    */

# if 0  // Z3 experiments
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

#else  // Parsing experiments

/// DEBUG SHIT
/*
    int nResult = execve(tracerProgramPath, tracerArgv, tracerEnviron);
    if (nResult < 0)
    {
      printf("1 Oh dear, something went wrong with execve! %s. command %s \n", strerror(errno), tracerProgramPath );
    }
    else
    {
      printf("Successfully started child process\n");
    }*/
///---------


   createChild(tracerProgramPath, tracerArgv, tracerEnviron, "message");

#endif

  return 0;
}

