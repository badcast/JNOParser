#pragma once

#include <map>
#include <string>
#include <vector>

namespace jno {

enum JNOSyntaxFormat{ Normal, WithOut_EOF, WithOut_Tabs };

constexpr int Node_ValueFlag = 1;
constexpr int Node_ArrayFlag = 2;
constexpr int Node_StructFlag = 3;

class JNOParser;

enum DataType { Unknown = 0, String = 1, Boolean = 2, Real = 3, Number = 4 };

class JNode {
    friend class JNOParser;
    std::string propertyName;
    std::uint8_t valueFlag;
    void* value = 0;
    mutable int* uses = nullptr;

    void*& setMemory(void* mem);
    int decrementMemory();
    int incrementMemory();

#ifdef _DEBUG
    ObjectNode* prevNode = nullptr;
    ObjectNode* nextNode = nullptr;
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
    std::string& getPropertyName();

    std::int64_t& toNumber();
    double& toReal();
    std::string& toString();
    bool& toBoolean();

    std::vector<std::int64_t>* toNumbers();
    std::vector<double>* toReals();
    std::vector<std::string>* toStrings();
    std::vector<bool>* toBooleans();
};

class JNOParser {
   public:
    using _StructType = std::map<int, JNode>;

   private:
    _StructType entry;
    int avail(JNOParser::_StructType& entry, const char* source, int len, int levels = 0);

   public:
    JNOParser();
    JNOParser(const JNOParser&) = delete;
    ~JNOParser() = default;

    ///Optimizing and recalculated. Return result size
    std::size_t defrag_memory();

    bool is_defraged_memory();
    
    bool parse(const char* filename);
    bool parse(const std::string& filename);
    bool Deserialize(const char* source, int len);
    bool deserialize(const std::string& content, int depth = -1);
    std::string Serialize();

    // Find node
    JNode* GetNode(const std::string& name);

    _StructType& GetContainer();
    _StructType& GetContainer(JNode* node);

    // Find node from child
    // example, key="First/Second/Triple" -> Node
    // for has a node, HasNode
    JNode* FindNode(const std::string& nodePath);

    bool ContainsNode(const std::string& nodePath);

    void Clear();
};

int stringToHash(const char* str, int len = INT32_MAX);

}  // namespace RoninEngine
