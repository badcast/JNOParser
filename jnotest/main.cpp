#include <iostream>
#include <string>

#include <jnoparser>

int main(int argn, char** argv) {
    using namespace std;
    jno::jno_object_parser parser;

    string file = argv[0];
    file.erase(file.find_last_of('/') + 1, file.length());
    parser.deserialize_from((file + "syntax.jno").c_str());

   /* cout << parser.at("node_name/node_2/n2_word")->toString()
         << parser.at("node_name/node_2/node_3/n3_word")->toString() << endl;*/

    return 0;
}
