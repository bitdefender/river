/*
 * Tester.cpp
 *
 *  Created on: Jan 3, 2012
 *      Author: sorin
 */

#include <stdio.h>
#include <iostream>
#include <dlfcn.h>
#include <string.h>
#include "CLoad.h"
#include <elf.h>

#define MAX_STRING      80

using namespace std;

void custom_invoke( char *lib, char *method, float argument ){
	MyLib* result = myLibLoad(lib);
	float (*func)(float);

	func = (float (*)(float))getFunctPointer(result, method);
	dout << "found it at " << (long)func << endl;
	printf("  %f\n", (*func)(argument) );

}

void invoke_method( char *lib, char *method, float argument )
{
  void *dl_handle;
  float (*func)(float);
  char *error;

  /* Open the shared object*/
  dl_handle = dlopen( lib, RTLD_LAZY );
  if (!dl_handle) {
    printf( "!!! %s\n", dlerror() );
    return;
  }

  /* Resolve the symbol (method) from the object*/
  func = (float (*)(float))dlsym( dl_handle, method );
  error = dlerror();
  if (error != NULL) {
    printf( "!!! %s\n", error );
    return;
  }

  /* Call the resolved method and print the result*/
  printf("  %f\n", (*func)(argument) );

  /* Close the object*/
  dlclose( dl_handle );

  return;
}

int main( int argc, char *argv[] )
{
  char line[MAX_STRING+1];
  char lib[MAX_STRING+1];
  char method[MAX_STRING+1];
  float argument;
  int custom;

  while (1) {

    printf("> ");

    line[0]=0;
    fgets( line, MAX_STRING, stdin);

    if (!strncmp(line, "bye", 3)) break;

    sscanf( line, "%s %s %f %d", lib, method, &argument, &custom);

    if (!custom)
    	invoke_method( lib, method, argument );
    else
    	custom_invoke( lib, method, argument );
  }

}


