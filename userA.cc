#include <sstream>
#include "model.h"

void* thread(void* arg) {
    for (int i = 0; i < 2; i++) {
        std::ostringstream oss;
        oss << "user A iter (in thread) " << i;
        action(oss.str());
    }
    return nullptr;
}

int main() {
    scheduler->new_thread(&thread, nullptr);

    for (int i = 0; i < 4; i++) {
        std::ostringstream oss;
        oss << "user A iter " << i;
        action(oss.str());
    }
    return 0;
}
