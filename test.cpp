#include <iostream>
#include "common/clipp.h"
#include "common/util.cpp"

using namespace clipp; using std::cout; using std::string;

int main(int argc, char* argv[]) {
    string IP = getLocalIP();
    cout << IP << endl;
}