#include <sstream>

#include "api.h"
#include "user.h"

int main() {
    for (int i = 0; i < 4 + process_id * 20; i++) {
        std::ostringstream oss;
        oss << "user B iter " << i;
        user_action(oss.str());

        if (i == 7) {
            model->process_crash();
        }
    }
    
    return 0;
}
