#include <iostream>

#include "../jnoparser.h"

using namespace std;

int main(int argn, char** argv) {
    jno::JNOParser parser;
    string file = argv[0];
    file.erase(file.find_last_of('/') + 1, file.length());
    if (!parser.parse(file + "syntax.jno")) {
        cout << "File not found or File access is denied!";
        return 0;
    }
    cout << parser.FindNode("node_name/node_2/n2_word")->toString()
         << parser.FindNode("node_name/node_2/node_3/n3_word")->toString() << endl;
    return 0;
}
