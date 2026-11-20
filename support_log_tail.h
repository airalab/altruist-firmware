#ifndef ALTRUIST_SUPPORT_LOG_TAIL_H
#define ALTRUIST_SUPPORT_LOG_TAIL_H

#include <stddef.h>
#include <stdint.h>
#include <string.h>

template <size_t Capacity, bool WholeLines>
class SupportLogTail {
public:
	void append(const uint8_t* data, size_t size) {
		if (size >= Capacity) {
			data += size - Capacity;
			size = Capacity;
			clear();
		}
		if (size > Capacity - count) {
			size_t drop = size - (Capacity - count);
			if (WholeLines) {
				while (drop < count && at(drop - 1) != '\n') ++drop;
			}
			discard(drop);
		}
		const size_t tail = (head + count) % Capacity;
		const size_t first = size < Capacity - tail ? size : Capacity - tail;
		memcpy(bytes + tail, data, first);
		memcpy(bytes, data + first, size - first);
		count += size;
	}

	size_t batchSize() const {
		const size_t limit = count < 1801 ? count : 1801;
		unsigned lines = 0;
		size_t cursor = head;
		for (size_t i = 1; i <= limit; ++i) {
			if (bytes[cursor] == '\n' && (++lines == 40 || i > 1200)) return i;
			if (++cursor == Capacity) cursor = 0;
		}
		return limit;
	}

	// A batch occupies at most two contiguous spans.
	const uint8_t* firstData() const { return bytes + head; }
	const uint8_t* secondData() const { return bytes; }
	size_t firstSize(size_t size) const { return size < Capacity - head ? size : Capacity - head; }

	void discard(size_t size) {
		head = (head + size) % Capacity;
		count -= size;
	}

	void clear() { head = count = 0; }

private:
	uint8_t at(size_t offset) const { return bytes[(head + offset) % Capacity]; }

	uint8_t bytes[Capacity];
	size_t head = 0;
	size_t count = 0;
};

#endif
