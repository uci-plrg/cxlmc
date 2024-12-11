#include "rfentry.h"

void rfEntry::dump() {
	printf("constraints: {");
	cl_store.dump(addr);	
	printf("}, "); 

	//processes crashes due to the read can be inferred from cls
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
		printf("(+%u, val=%lx, seq=%u, tid=%u, type=%s), ", i<<3, write->get_value(), write->get_seq_num(), write->get_thread_id(),
			action_type2str(write->get_type()));
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
