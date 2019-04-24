#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

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

