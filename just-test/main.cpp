#include <iostream>
#include <string>

// Include justparser
#include <justparser>

int main(int argn, char** argv) {
    using namespace std;
    just::just_object_parser parser;

    string executable_path = argv[0];
    executable_path.erase(executable_path.find_last_of('/') + 1, executable_path.length());
    parser.deserialize_from((executable_path + "syntax.just").c_str());

   /* cout << parser.at("node_name/node_2/n2_word")->toString()
         << parser.at("node_name/node_2/node_3/n3_word")->toString() << endl;*/

    return 0;
}
