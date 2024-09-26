#include "rfentry.h"

void rfEntry::dump() {
	printf("cacheline: [");
	for (auto &pair: cl.get_range_map())
		printf("(%d, %d), ", pair.second.getBegin(), pair.second.getEnd()); 
	printf("], "); 
	//processes crashes due to the read can be inferred from crashed cachelines
	printf("crashed processes {");
	for (auto &pair: crashes) {
		printf("p%d at %d, ", pair.first, pair.second);
	}
	printf("}, ");

	printf("writes: [");
	for (uint i = 0; i < overlaps.size(); i++) {
		auto write = overlaps[i];
		if (!write)
			continue;
		printf("(+%u, val=%ld, seq=%u), ", i<<3, write->get_value(), write->get_seq_num());
	}
	printf("]\n");
}

uint64_t rfEntry::get_read_value(void *read_location) {
	uint64_t value = 0;
	for (int i= (int)overlaps.size()-1; i >= 0; i--) {
		value = value << 8;
		auto write = overlaps[i];
		if (!write)
			continue;
		int offset = i + (char *)read_location - (char *)write->get_location();
		uint64_t writevalue = write->get_value() >> (8 * offset);
		value |= writevalue & 0xff;
	}
	return value;
}
