#include <sstream>

#include "api.h"
#include "user.h"

int main() {
    for (int i = 0; i < 4; i++) {
        std::ostringstream oss;
        oss << "user B iter " << i;
        user_action(oss.str());
    }
    return 0;
}
