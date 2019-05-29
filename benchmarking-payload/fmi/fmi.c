#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

// Crashes
#include <sys/mman.h>
#include <signal.h>


// TODO: move this somewhere else
	char* myitoa(int value, char* result, int base) {
		// check that the base if valid
		if (base < 2 || base > 36) { *result = '\0'; return result; }

		char* ptr = result, *ptr1 = result, tmp_char;
		int tmp_value;

		do {
			tmp_value = value;
			value /= base;
			*ptr++ = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz" [35 + (tmp_value - value * base)];
		} while ( value );

		// Apply negative sign
		if (tmp_value < 0) *ptr++ = '-';
		*ptr-- = '\0';
		while(ptr1 < ptr) {
			tmp_char = *ptr;
			*ptr--= *ptr1;
			*ptr1++ = tmp_char;
		}
		return result;
	}
	

int r = 0;

void crash(int x) {
    // Make it more probable to crash (unsigned char <= 256)
    // x = x / 3;

    // Segmentation fault
    if (x >= 0 && x <= 10) {
        int a[10];
        for (int i = 0; i < 1000; ++i)
            a[i] = i;
    }        

    // Bus error
    if (x >= 11 && x <= 20) {   
        FILE *f = tmpfile();
        int *m = mmap(0, 4, PROT_WRITE, MAP_PRIVATE, fileno(f), 0);
        *m = 0;
    }

    if (x >= 21 && x <= 30) {
        raise(SIGTERM);
    }

    if (x >= 31 && x <= 40) {
        raise(SIGABRT);
    }

    if (x >= 41 && x <= 50) {   
        raise(SIGFPE);
    }
}

// First target
void test_simple(const unsigned char *buf) 
{
    /*
    // Print some text using the first 3 characters from buf
    {
 	static char localTextBuffer[256];
	strcpy(localTextBuffer, "\n\n########## Text is : ");
 	char charactersBuff[2]; // Character + 0
   	 myitoa(0, charactersBuff, 10);
         strcat(localTextBuffer, charactersBuff);
         strcat(localTextBuffer, " , ");
   	 myitoa(1, charactersBuff, 10);
         strcat(localTextBuffer, charactersBuff);
         strcat(localTextBuffer, "\n");

        const int len = strlen(localTextBuffer);
    	const ssize_t res = write(1, localTextBuffer, len);
    	fflush(stdout);
    }
	*/

    if (buf[0] == 'A')
    {
        r = 1;
        if (buf[3] == 'C')
        {
            r = 3;
        }
    }
    else if (buf[1] == 'B')
    {
        r = 2;

        if (buf[2] == '4')
        {
            r = 4;
        }
    }

    if (buf[0]=='A' && buf[1] == 'B' && buf[2] < 'D')
    {
	    for (int i = 0; i < buf[0]; i++)
	    {
	      r += i;
	    }
    }

    crash(buf[0]);
    
    //printf("%d", r);
}


void test_simple_2(const unsigned char *buf) 
{
	printf("cip");
	char a = buf[0];
	char b = buf[1];
	char c = a + b;
	if (c == 10)
	{
		r = 1;
	}
	else
	{
		r = 2;
	}
}

