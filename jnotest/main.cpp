#include <iostream>
#include "JNOParser.hpp"

using namespace std;

int main()
{
    jno::JNOParser parser;
    parser.Deserialize("./syntax.jno");
    cout << "Hello World!" << endl;
    return 0;
}
