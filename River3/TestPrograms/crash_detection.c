#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Sleep
#include <unistd.h>

// Crashes
#include <sys/mman.h>
#include <signal.h>

int is_uppercase(char c) {
    return c >= 'A' && c <= 'Z';
}

int is_lowercase(char c) {
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

void RIVERTestOneInput(char *ptr) {

    int fst = (unsigned char)ptr[0];
    int snd = (unsigned char)ptr[1];
    int thd = (unsigned char)ptr[2];
    int fth = (unsigned char)ptr[3];

    int x = -0xffff;
    if (is_uppercase(fst) || is_lowercase(fst)) {
        if (is_operator(snd)) {
            if (is_number(thd)) 
            {
                x = fth;
            }            
        }
    }

    // Not interested in the input
    if(x == -0xffff)
        return;

    int error = 0;

    // Segmentation fault
    if (x >= 0 && x <= 20) {
        int a[10];
        for (int i = 0; i < 1000; ++i)
            a[i] = i;

       error = 1; 
    }        

    // Bus error
    if (x >= 21 && x <= 40) {   
        FILE *f = tmpfile();
        int *m = mmap(0, 4, PROT_WRITE, MAP_PRIVATE, fileno(f), 0);
        *m = 0;

        error = 2;
    }

    if (x >= 41 && x <= 60) {
        raise(SIGTERM);

        error = 3;
    }

    if (x >= 61 && x <= 80) {
        raise(SIGABRT);

        error = 4;
    }

    if (x >= 81 && x <= 120) {   
        raise(SIGFPE);

        error = 5;
    }

    if (x >= 121 && x <= 127) {
        sleep(5);
        // raise (SIGHUP);

        error = 6;
    }

    if (error != 0) {
        printf("Error found: %d!", error);
        return;
    }
}

int main(int ac, char **av)
{
    /* Reading input with command line args
    int ret;

    if (ac != 2)
    {
        printf("First param must be the string password");
        return -1;
    }

    int fth = 0;
    if (strlen(av[1]) >= 4)
        fth = (unsigned char) av[1][3];

    printf("Executing: %s %s. Fourth char: %d\n", av[0], av[1], fth);
    RIVERTestOneInput(av[1]);
    */

    /* Reading input as a string from stdin
    char input[100];
    scanf("%s", input);
    RIVERTestOneInput(input);
    */

    /* Reading input as bytes from stdin */
    freopen(NULL, "rb", stdin);

    char buffer[1000];
    while (fread(buffer, sizeof(char), 1000, stdin)) {
        // Reusing buffer?
        RIVERTestOneInput(buffer);

        if (feof(stdin)) {
            break;
        }
    }

    return 0;
}

