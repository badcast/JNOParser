#include <iostream>
#include <string>

// Include justparser
#include <justparser>

inline char fileseparator()
{
#ifdef WIN32
    return '\\';
#else
    return '/';
#endif
}

std::string getExecPath(std::string app, const std::string& combine = "")
{
    using namespace std;
    std::cout << app;
    app.erase(app.find_last_of(fileseparator()), app.length());
    if (!combine.empty())
        app += fileseparator() + combine;
    return app;

}

int main(int argn, char** argv)
{
    just::just_object_parser parser;

    parser.deserialize_from(getExecPath(*argv, "syntax.just"));

    /* cout << parser.at("node_name/node_2/n2_word")->toString()
          << parser.at("node_name/node_2/node_3/n3_word")->toString() << endl;*/
}
