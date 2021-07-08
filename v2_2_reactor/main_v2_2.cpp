#include "Reactor.h"
#include "Acceptor.h"
#include "SelectDemultiplexer.h"
using namespace std;


const string HttpServer::rootDir = "../web_root";

int main() {
    Reactor *pReactor = Reactor::getInstance();
    pReactor->setEventDemultiplexer(new SelectDemultiplexer());
    pReactor->registerHandler(new Acceptor(80, "localhost"), EventType::OP_ACCEPT);
    pReactor->handleEvents(1);

    return 0;
}