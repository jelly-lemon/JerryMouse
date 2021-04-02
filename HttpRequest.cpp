#include <map>
#include <string>
using namespace std;

class HttpRequest {
private:
    map<string, string> requestLine;
    map<string, string> requestHeader;
    string requestBody;

public:
    HttpRequest();
    HttpRequest(string data);
    static bool isAllData(string data);
    string getURL();
    string getMethod();
};

HttpRequest::HttpRequest() {

}

HttpRequest::HttpRequest(string data) {

}

bool HttpRequest::isAllData(string data) {

    return false;
}

string HttpRequest::getURL() {
    return requestLine["url"];
}

string HttpRequest::getMethod() {
    return requestLine["method"];
}