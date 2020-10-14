#include <stdio.h>

void RIVERTestOneInput(char input[4])
{
	int cnt = 0;
	if (input[0] == 'b') cnt++;
	if (input[1] == 'a') cnt++;
	if (input[2] == 'd') cnt++;
	if (input[3] == '!') cnt++;
	if (cnt >= 4)
	{
		printf("Error\n");
	}
	else
		printf("%d\n", cnt);
} 
	

int main(int ac, char **av)
{
	int ret;

	if (ac != 2)
	{
	    printf("First param must be the string input");
	    return -1;
	}
	
	RIVERTestOneInput(av[1]);
	return 0;
}

