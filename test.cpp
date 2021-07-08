#include <iostream>
#include "include/clipp.h"
#include "include/util.h"

using namespace clipp; using std::cout; using std::string;

int main(int argc, char* argv[]) {
    string IP = getLocalIP();
    cout << IP << endl;
}