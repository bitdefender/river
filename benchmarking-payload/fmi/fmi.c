#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

// Crashes
#include <sys/mman.h>
#include <signal.h>

void print_int(char *s, int a) {
    puts(s);
    if (a < 0) {
        puts("-");
        a = -a;
    }

    int v[10] = {0};
    int k = 0;    

    while (a) {
        v[k++] = a % 10;
        a /= 10;
    }

    for (int i = k - 1; i >= 0; --i) {
        switch (v[i]) {
            case 0: puts("0"); break;
            case 1: puts("1"); break;
            case 2: puts("2"); break;
            case 3: puts("3"); break;
            case 4: puts("4"); break;
            case 5: puts("5"); break;
            case 6: puts("6"); break;
            case 7: puts("7"); break;
            case 8: puts("8"); break;
            case 9: puts("9"); break;
        }
    }

}

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
    x = x / 3;

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

    if (x >= 51 && x <= 60) {
        sleep(5);
    }
}

int is_uppercase(int c) {
    /*
    char tst[2];
    tst[0] = c;
    tst[1] = '\0';
    write(1, tst, 2);
    fflush(stdout);

    write(1, "\n", 2);
    fflush(stdout);
    */
    return c >= 'A' && c <= 'Z';
}

int is_lowercase(int c) {
    return c >= 'a' && c <= 'z';
}

int is_number(char c) {
    return c >= '0' && c <= '9';
}

int is_operator(char c) {
    return c == '=' || c == '<' || c == '>';
}

int is_voyel(char c) {
    if ((char) c == 'A' || (char) c == 'E' || (char) c == 'I' || (char) c == 'O' || (char) c == 'U' || 
        (char) c == 'a' || (char) c == 'e' || (char) c == 'i' || (char) c == 'o' || (char) c == 'u')
        return 1;
    return 0;
}

// First target
void test_simple_2(const unsigned char *buf) 
{
    printf("ceva");

    // int pos = (int)&printf;
    // print_int("pos: ", pos);

    int a = 12;
    printf("\na = %d\n", a);
    printf("\na = %d\n", a);
    return;

    int fst = (int)buf[0];
    int snd = (int)buf[1];
    int thd = (int)buf[2];
    int fth = (int)buf[3];
    int fifth = (int)buf[4];

    fst = 'A';
    snd = '=';
    thd = '1';
    fth = 175;

    if (snd == '=')
        puts("snd este =\n");
    if (is_operator(snd) == 1)
        puts("snd e operator\n");

    if (is_uppercase(fst) || is_lowercase(fst)) {
        if (is_operator(snd)) {
            if (is_number(thd)) 
            {
                crash(fth);
            }            
        }

    }
}


void test_simple(const unsigned char *buf) 
{
	int a = 3;
	if (buf[0] < 'Z')
        {
		a=4;
		int b = (buf[0] + buf[1]) / 2;
		if (b < 'Z')
		{
			a=5;
		}

	}
}

