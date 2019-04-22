#include <stdio.h>

int r = 0;

// First target
void test_simple(const unsigned char *buf) 
{
    //printf("cip");
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

