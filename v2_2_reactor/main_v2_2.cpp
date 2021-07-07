#include "Reactor.h"
#include "Acceptor.h"
#include "SelectDemultiplexer.h"
using namespace std;

int main() {
    Reactor reactor = Reactor::getInstance();
    reactor.setEventDemultiplexer(new SelectDemultiplexer());
    reactor.registerHandler(new Acceptor(80, "localhost"), EventType::OP_ACCEPT);
    reactor.handleEvents(1);

    return 0;
}