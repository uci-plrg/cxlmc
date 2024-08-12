#include <sstream>

#include "model.h"

int main() {
    for (int i = 0; i < 2 + thread_id % 5; i++) {
        std::ostringstream oss;
        oss << "user B iter " << i;
        thing(oss.str());
    }
    return 0;
}
