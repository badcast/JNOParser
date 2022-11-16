#pragma once

#include <cstdint>
#include <map>
#include <cstring>
#include <vector>
#include <fstream>
#include <stdexcept>

namespace jno {
class jno_object_parser;
class jno_object_node;

/// undefined type
typedef void* jvariant;
/// string type
typedef std::string jstring;
/// integer type
typedef std::int64_t jnumber;
/// real type
typedef double jreal;
/// logical type
typedef bool jbool;

using jstruct = std::map<int, jno_object_node>;

enum JNOSerializeFormat { JNOBeautify, JNOCompact };

enum JNOType : std::uint8_t { Null = 255, Unknown = 0, JNOBoolean, JNONumber, JNOReal, JNOString };

class jno_object_node {
    friend class jno_object_parser;
    void* handle = nullptr;

   public:
    std::string propertyName;

    jno_object_node() = default;
    jno_object_node(const jno_object_node&);
    virtual ~jno_object_node() = default;

    jno_object_node& operator=(const jno_object_node&);

    JNOType type();

    jno_object_node* tree(const jstring& child);

    // Property name it is Node
    std::string& getPropertyName();
};

class jno_object_parser {
   protected:
    void* _storage;
    jstruct entry;

   public:
    jno_object_parser();
    jno_object_parser(const jno_object_parser&) = delete;
    virtual ~jno_object_parser();

    void deserialize_from(const char* filename);
    void deserialize(const std::string& source);
    void deserialize(const char* source, int len);
    void deserialize(const std::string& content, int depth = -1);

    std::string serialize();

    // Find node
    jno_object_node* at(const std::string& name);

    jstruct& get_struct();

    // Find node from childrens
    // example, "First/Second/Triple" -> Node
    // for has a node, contains method use.
    jno_object_node* find_node(const std::string& nodePath);

    // search node by name or prefix value, example
    jno_object_node* search(const std::string& name);

    // result of occupied memory forevery nodes
    jnumber occupied_memory();

    bool contains(const std::string& nodePath);

    jno_object_node* tree(const jstring& child);
};

jno_object_node& operator<<(jno_object_node& root, const jstring& nodename);

jno_object_node& operator<<(jno_object_parser& root, const jstring& nodename);

int jno_string_to_hash_fast(const char* str, int len = INT32_MAX);

}  // namespace jno
