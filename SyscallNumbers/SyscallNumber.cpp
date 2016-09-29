#include <stdio.h>
#include <Windows.h>
#define IS_ADDRESS_BETWEEN( left, right, address ) ( (address) >= (left) && (address) < (right) )

#include <vector>
#include <utility>
#include <string>
#include <algorithm>

#include "Loader/PE.ldr.h"

DWORD GreaterPow2(DWORD x) {
	x--;
	x |= x >> 16;
	x |= x >> 8;
	x |= x >> 4;
	x |= x >> 2;
	x |= x >> 1;
	return x + 1;
}

template <typename V> class SemiSparseMap {
private :
	std::vector<std::pair<DWORD, V> > data;

	struct Segment {
		DWORD start;
		DWORD count;
		DWORD lookup;

		Segment(DWORD s, DWORD c, DWORD l) {
			start = s;
			count = c;
			lookup = l;
		}
	};

	std::vector<Segment> segments;
	bool optimized;

public :
	SemiSparseMap() {
		optimized = true;
	}

	void Add(DWORD key, V value) {
		data.push_back(std::pair<DWORD, V>(key, value));
		optimized = false;
	}

	void Optimize() {
		std::sort(data.begin(), data.end(), [](auto &a, auto &b) {
			return a.first < b.first;
		});

		segments.clear();
		segments.push_back(Segment(data.begin()->first, 0, 0));

		for (DWORD i = 0; i < data.size(); ++i) {
			if (data[i].first == segments.back().start + segments.back().count) {
				segments.back().count++;
			} else {
				segments.push_back(Segment(data[i].first, 1, i));
			}
		}

		optimized = true;
	}

	V *Find(DWORD key) {
		unsigned int s = GreaterPow2(segments.size() + 1);
		unsigned int p = -1;

		for (; s > 0; s >>= 1) {
			if (p + s < segments.size()) {
				if (segments[p + s].start <= key) {
					p += s;
					if (segments[p].start + segments[p].count > key) {
						int delta = key - segments[p].start;
						return &data[segments[p].lookup + delta].second;
					}
				}
			}
		}

		return nullptr;
	}
};


int main(int argc, char* argv[]) {

	printf("SYSCALL,RVA,NAME\n");
	// loop each exported symbol by name
	FloatingPE fpe("c:\\windows\\system32\\ntdll.dll");

	SemiSparseMap<std::string> syscalls;

	if (fpe.IsValid()) {
		fpe.ForAllExports([&syscalls](const char *funcName, const DWORD rva, const unsigned char *body) {
			if (body[0] == 0xB8 &&									            // mov eax, imm32
				(
				(
					body[5] == 0x33 && body[6] == 0xC9 &&		            // xor ecx, ecx
					!memcmp(&body[7], "\x8D\x54\x24\x04", 4) &&          // lea edx, [esp+04h]
					!memcmp(&body[11], "\x64\xFF\x15\xC0\x00\x00\x00", 7) // call fs:[C0h]
					)
					||
					(
						body[5] == 0xB9 &&								        // mov ecx, imm32
						!memcmp(&body[10], "\x8D\x54\x24\x04", 4) &&          // lea edx, [esp+04h]
						!memcmp(&body[13], "\x64\xFF\x15\xC0\x00\x00\x00", 7) // call fs:[C0h]
						)
					)
				||
				(
					body[5] == 0xBA &&								        // mov edx, imm32
					!memcmp(&body[10], "\xFF\xD2", 2)                       // call edx
					)
				)
			{
				/*
				* Extract the IMM32 part, this is our syscall number.
				*/
				if (!strncmp((char *)funcName, "Nt", 2)) {
					DWORD dwSyscall = *(DWORD *)(body + 1);
					printf("0x%08X,0x%08X,%s\n", dwSyscall, rva, funcName);

					syscalls.Add(dwSyscall, std::string(funcName));
				}
			}
		});
	}

	syscalls.Optimize();

	std::string *sName = syscalls.Find(1);

	//printf("%s\n", sName->c_str());
	return 0;
}