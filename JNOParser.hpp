#pragma once

#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <map>
#include <memory>
#include <stdexcept>
#include <vector>

namespace jno {

enum JNOSyntaxFormat { Normal, WithOut_EOF, WithOut_Tabs };
enum JData { Unknown = 0, String = 1, Boolean = 2, Real = 3, Number = 4 };

///undefined type
typedef void* jvariant;
///string type
typedef std::string jstring;
///integer type
typedef std::int64_t jnumber;
///real type
typedef double jreal;
///logical type
typedef bool jbool;

class JNOParser;

class JNode {
    friend class JNOParser;
    jstring propertyName;
    std::uint8_t valueFlag;
    void* value = 0;
    mutable int* uses = nullptr;

    void*& setMemory(void* mem);
    int decrementMemory();
    int incrementMemory();

#ifdef DEBUG
    JNode* prevNode = nullptr;
    JNode* nextNode = nullptr;
#endif

   public:
    JNode() = default;
    JNode(const JNode&);

    JNode& operator=(const JNode&);

    ~JNode();

    inline bool isArray();
    inline bool isStruct();
    inline bool isValue();
    inline bool isNumber();
    inline bool isReal();
    inline bool isString();
    inline bool isBoolean();

    // Property name it is Node
    jstring getPropertyName();

    jnumber& toNumber();
    jreal& toReal();
    jstring& toString();
    jbool& toBoolean();

    std::vector<std::int64_t>* toNumbers();
    std::vector<double>* toReals();
    std::vector<jstring>* toStrings();
    std::vector<bool>* toBooleans();

   public:
    /// convert to signed int
    operator int();
    /// convert ot unsigned int
    operator std::uint32_t();
    /// convert to signed int 64
    operator jnumber();
    /// convert to unsigned int 64
    operator std::uint64_t();
    /// convert to double
    operator jreal();
    /// convert to float
    operator float();
    /// convert to string
    operator jstring();
    /// convert to bool
    operator jbool();
};

class JNOParser {
   public:
    using JStruct = std::map<int, JNode>;

   private:
    void* memory_ordering;
    std::uint8_t flag_ensure;
    JStruct entry;
    int avail(JNOParser::JStruct& entry, const char* source, int len, int levels = 0);

   public:
    JNOParser();
    JNOParser(const JNOParser&) = delete;
    ~JNOParser() = default;

    std::size_t defrag_require_memory();
    /// Optimizing and recalculated. Return result size
    std::size_t defrag_memory();

    bool is_defraged_memory();

    bool parse(const char* filename);
    bool parse(const jstring& filename);
    bool Deserialize(const char* source, int len);
    bool deserialize(const jstring& jcontent, int depth = -1);
    jstring Serialize();

    // Find node
    JNode* GetNode(const jstring& jnodename);

    JStruct& GetContainer();
    JStruct& GetContainer(JNode* jnode);

    // Find node from child
    // example, key="First/Second/Triple" -> Node
    // for has a node, HasNode
    JNode* FindNode(const jstring& jnodePath);

    bool ContainsNode(const jstring& jnodePath);

    void Clear();

    JNOParser& operator>>(const jstring& parser);
    JNOParser& operator>>(std::istream& istream);
    jstring operator<<(std::iostream& ostream);
};

int stringToHash(const char* str, int len = INT32_MAX);

}  // namespace jno
