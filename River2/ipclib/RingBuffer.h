#ifndef _RING_BUFFER_H_
#define _RING_BUFFER_H_

namespace ipc {
	template <int SZ> class RingBuffer {
	private:
		char buffer[SZ];
		volatile int head, tail;

	public:
		void Init() {
			head = tail = 0;
			buffer[0] = 0;
		}

		bool IsEmpty() const {
			return head == tail;
		}

		bool IsFull() const {
			int nextTail = (tail + 1) % SZ;
			return nextTail == head;
		}

		void Read(char *buf, int size, int &read) {
			int vH = head;
			if (vH < tail) {
				vH += SZ;
			}

			read = vH - tail;
			if (size < read) {
				read = size;
			}

			int dwLast = (tail + read) % SZ;

			for (int p = 0; tail != dwLast; ++p) {
				buf[p] = buffer[tail];
				tail++;
				if (tail >= SZ) {
					tail -= SZ;
				}
			}
		}

		void Write(char *buf, int size) {
			int vT = tail;

			if (vT <= head) {
				vT += SZ;
			}

			int write = vT - head - 1;

			if (size > write) {
				tail = (head + write + 1) % SZ;
			}
			else if (size < write) {
				write = size;
			}

			int dwLast = (head + write) % SZ;

			for (int p = 0; head != dwLast; ++p) {
				buffer[head] = buf[p];
				head++;
				if (head >= SZ) {
					head -= SZ;
				}
			}
			buffer[dwLast] = '\0';
		}
	};
};

#endif