extern "C" unsigned int fibo0 = 1, fibo1 = 1;


int Payload() {
	unsigned int fib0 = fibo0, fib1 = fibo1;
	for (int i = 0; i < 1000 - 2; ++i) {
		unsigned int t = fib0 + fib1;
		fib0 = fib1;
		fib1 = t;
	}

	return fib1;
}

