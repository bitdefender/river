


int __cdecl overlap(unsigned int a1, unsigned int a2, unsigned int b1, unsigned int b2) {

	if (!(a1 < a2 || b1 < b2)) {
		return -1;
	}

	if (b1 > a2) {
		return 0;
	}

	if (b2 < a1) {
		return 0;
	}

	if (b1 <= a1 && b2 <= a2) {
		return 1;
	}

	return 0;
}

