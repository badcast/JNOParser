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

std::string get_exec_pwd(std::string app, const std::string& combine)
{
    using namespace std;
    app.erase(app.find_last_of(fileseparator()), app.length());
    if (!combine.empty())
        app += fileseparator() + combine;
    return app;

}

int main(int argn, char** argv)
{
    just::just_object_parser parser;

    parser.deserialize_from(get_exec_pwd(*argv, "syntax.just"));
    /* cout << parser.at("node_name/node_2/n2_word")->toString()
          << parser.at("node_name/node_2/node_3/n3_word")->toString() << endl;*/
}
