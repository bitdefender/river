
int check(const char* data, const int size)
{
  if (data[0] == 'x')
  {
     if (data[1] == 'y')
        return 2;
     else
    	return 1;
  }
  return 0;
}

int main(int ac, const char **av)
{
  return check(av[1], 5);
}

