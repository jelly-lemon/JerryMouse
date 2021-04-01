#include <winsock2.h>

class HttpResponse {
private:
    SOCKET connSokcet;
public:
    HttpResponse(SOCKET &connSocket);
    void* handleConnection(void *args);
};

void *HttpResponse::handleConnection(void *args) {


    return nullptr;
}

HttpResponse::HttpResponse(SOCKET &connSocket) {
    this->connSokcet = connSocket;
}
