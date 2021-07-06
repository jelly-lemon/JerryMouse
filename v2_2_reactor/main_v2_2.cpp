#include "Reactor.h"
using namespace std;

int main() {
    Reactor reactor;

    while (true) {
        reactor.handleEvents(3000);
    }

    return 0;
}