#include "JNOParser.hpp"
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <cstring>
#include <cmath>
#include <vector>
#include <fstream>
namespace jno {

const struct {
    char dot = '.';
    char obstacle = ',';
    char split = ':';
    char space = ' ';
    char nodePathBreaker = '/';
    char commentLine[3] = "//";
    char left_seperator = '\\';
    char eof_segment = '\n';
    char format_string = '\"';
    char true_string[5]{"true"};
    char false_string[6]{"false"};
    char array_segments[2]{'{', '}'};
    char trim_segments[5]{' ', '\t', '\n', '\r', '\v'};
    char node_name_concqt = '_';
} syntax;

enum data_type { unknown = 0, string = 1, boolean = 2, real = 3, number = 4 };

static uint16_t trimpos(const char* c, int len = INT32_MAX) {
    uint16_t i, j;
    for (i = 0; c[i] && i <= len;) {
        for (j = 0; j < sizeof(syntax.trim_segments);)
            if (c[i] == syntax.trim_segments[j])
                break;
            else
                ++j;
        if (j != sizeof(syntax.trim_segments))
            ++i;
        else
            break;
    }
    return i;
}

static uint16_t trimposInvert(const char* c, int len = INT32_MAX) {
    uint16_t i, j;
    for (i = 0; c[i] && i <= len;) {
        for (j = 0; j < sizeof(syntax.trim_segments);)
            if (c[i] == syntax.trim_segments[j])
                break;
            else
                ++j;
        if (j == sizeof(syntax.trim_segments))
            ++i;
        else
            break;
    }
    return i;
}

static bool is_number(const char c) { return (c >= '0' && c <= '9') || c == '-'; }

static bool is_real(const char* c, int* getLength) {
    *getLength = 0;
    bool real = false;
    while (true) {
        if (!is_number(c[*getLength])) {
            if (real) break;

            real = (c[*getLength] == syntax.dot);

            if (!real) break;
        }
        ++*getLength;
    }
    return real;
}

static bool is_bool(const char* c, int* getLength) {
    if (!memcmp(c, syntax.true_string, *getLength = 4)) return true;
    if (!memcmp(c, syntax.false_string, *getLength = 5)) return true;
    *getLength = 0;
    return false;
}

static int go_end_line(const char* c, int len = INT32_MAX) {
    int i = 0;
    for (; c[i] && i < len && c[i] != syntax.eof_segment; ++i)
        ;
    return i;
}

static int Get_FormatType(const char* c, void** mem, DataType& out) {
    int temp = 0;

    if (mem) *mem = nullptr;

    // string type
    if (*c == syntax.format_string) {
        ++temp;
        for (; c[temp] != syntax.format_string; ++temp)
            if (c[temp] == syntax.left_seperator && c[temp + 1] == syntax.format_string) ++temp;
        --temp;
        if (temp) {
            if (mem) {
                std::string str = std::string();
                str.reserve(temp);
                for (size_t i = 0; i < temp; ++i) {
                    if (c[i + 1] == syntax.left_seperator) ++i;
                    str.push_back(c[i + 1]);
                }
                *mem = reinterpret_cast<void*>(new std::string(str));
            }
            out = DataType::String;
        } else
            out = DataType::Unknown;

        temp += 2;
    } else if (is_real(c, &temp)) {
        if (mem) *mem = new double(atof(c));
        out = DataType::Real;
    } else if (is_number(*c)) {
        if (mem) *mem = new int64_t(atoll(c));
        out = DataType::Number;
    } else if (is_bool(c, &temp)) {
        if (mem) *mem = new bool(temp == sizeof(syntax.true_string) - 1);
        out = DataType::Boolean;
    } else
        temp = 0;

    return temp;
}

static bool Has_DataType(const char* currentPtr) {
    DataType type;
    Get_FormatType(const_cast<char*>(currentPtr), nullptr, type);
    return type != DataType::Unknown;
}

inline int trim(const char* c, int len = INT32_MAX) {
    int i = 0, j;
    if (c != nullptr)
        for (; i < len && c[i];) {
            for (j = 0; j < static_cast<int>(sizeof(syntax.trim_segments));)
                if (c[i] == syntax.trim_segments[j])
                    break;
                else
                    ++j;
            if (j != sizeof(syntax.trim_segments))
                ++i;
            else
                break;
        }
    return i;
}

inline int skip(const char* c, int len = INT32_MAX) {
    int i, j;
    char skipping[sizeof(syntax.trim_segments) + sizeof(syntax.array_segments)];
    skipping[0] = '\0';
    strncpy(skipping, syntax.trim_segments, sizeof(syntax.trim_segments));
    strncpy(skipping + sizeof syntax.trim_segments, syntax.array_segments, sizeof(syntax.array_segments));
    for (i = 0; c[i] && i <= len;) {
        for (j = 0; j < sizeof(skipping);)
            if (c[i] == skipping[j])
                break;
            else
                ++j;
        if (j == sizeof(skipping))
            ++i;
        else
            break;
    }
    return i;
}

inline bool isProperty(const char* c, int len) {
    for (size_t i = 0; i < len; ++i)
        if (!((toupper(c[i]) >= 'A' && toupper(c[i]) <= 'Z') || (i > 0 && is_number(c[i])) || c[i] == syntax.node_name_concqt)) return false;
    return len != 0;
}

inline bool isCommentLine(const char* c, int len) { return len > 0 && !strncmp(c, syntax.commentLine, std::min(2, len)); }

inline int ignoreComment(const char* c, int len) {
    int i = 0;
    i += trim(c + i, len - i);
    while (isCommentLine(c + i, len - i)) {
        i += go_end_line(c + i, len - i);
        i += trim(c + i, len - i);
    }
    return i;
}

inline bool isSpace(char c) {
    for (int i = 0; i <= sizeof(syntax.trim_segments); ++i)
        if (syntax.trim_segments[i] == c) return true;
    return false;
}

static bool isArray(const char* c, int& endpoint, int len = INT32_MAX) {
    int i = 0;
    bool result = false;

    if (*c == syntax.array_segments[0]) {
        ++i;
        i += trim(c + i);

        i += ignoreComment(c + i, len - i);
        i += trim(c + i);
        if (Has_DataType(c + i) || c[i] == syntax.array_segments[1]) {
            for (; c[i] && i < len; ++i) {
                if (c[i] == syntax.array_segments[0])
                    break;
                else if (c[i] == syntax.array_segments[1]) {
                    endpoint = i;
                    result = true;
                    break;
                }
            }
        }
    }
    return result;
}

int stringToHash(const char* str, int len) {
    int r = 1;
    int i = 0;
    while (*(str + i) && i < len) r *= (int)*(str + i++);
    return r;
}

JNode::JNode(const JNode& copy) {
    this->value = copy.value;
    this->valueFlag = copy.valueFlag;
    this->propertyName = copy.propertyName;
    decrementMemory();
    uses = copy.uses;
    incrementMemory();
    copy.uses = uses;
}
JNode& JNode::operator=(const JNode& copy) {
    this->value = copy.value;
    this->valueFlag = copy.valueFlag;
    this->propertyName = copy.propertyName;
    decrementMemory();
    uses = copy.uses;
    incrementMemory();
    copy.uses = uses;
    ++*copy.uses;
    return *this;
}
JNode::~JNode() {
    if (!decrementMemory() && value) {
        if (isStruct()) {
            delete ((JNOParser::_StructType*)value);
        } else if (isArray()) {
            switch (valueFlag >> 2) {
                case DataType::Boolean:
                    delete (toBooleans());
                    break;
                case DataType::String:
                    delete (toStrings());
                    break;
                case DataType::Number:
                    delete (toNumbers());
                    break;
                case DataType::Real:
                    delete (toReals());
                    break;
            }
        } else if (isValue()) {
            if (isString()) {
                delete ((std::string*)value);
            } else
                free(value);
        }
    }
}

void*& JNode::setMemory(void* mem) {
    value = mem;

    return mem;
}

int JNode::decrementMemory() {
    int res = 0;
    if (uses)
        if (!(res = --(*uses))) {
            res = *uses;
            delete (uses);
        }
    uses = nullptr;
    return res;
}

int JNode::incrementMemory() {
    if (!uses) uses = new int;
    return ++*uses;
}

bool JNode::isValue() { return (this->valueFlag & 3) == Node_ValueFlag; }
bool JNode::isArray() { return (this->valueFlag & 3) == Node_ArrayFlag; }
bool JNode::isStruct() { return (this->valueFlag & 3) == Node_StructFlag; }
bool JNode::isNumber() { return ((this->valueFlag & 0x1C) >> 2) == DataType::Number; }
bool JNode::isReal() { return ((this->valueFlag & 0x1C) >> 2) == DataType::Real; }
bool JNode::isString() { return ((this->valueFlag & 0x1C) >> 2) == DataType::String; }
bool JNode::isBoolean() { return ((this->valueFlag & 0x1C) >> 2) == DataType::Boolean; }

std::string& JNode::getPropertyName() { return this->propertyName; }
std::int64_t& JNode::toNumber() {
    if (!isNumber()) throw std::bad_cast();

    return *(std::int64_t*)this->value;
}
double& JNode::toReal() {
    if (!isReal()) throw std::bad_cast();
    return *(double*)this->value;
}
std::string& JNode::toString() {
    if (!isString()) throw std::bad_cast();
    return *(std::string*)this->value;
}
bool& JNode::toBoolean() {
    if (!isBoolean()) throw std::bad_cast();
    return *(bool*)this->value;
}
std::vector<std::int64_t>* JNode::toNumbers() {
    if (!isArray() && (valueFlag >> 2) != DataType::Number) throw std::bad_cast();
    return (std::vector<std::int64_t>*)value;
}
std::vector<double>* JNode::toReals() {
    if (!isArray() && (valueFlag >> 2) != DataType::Real) throw std::bad_cast();
    return (std::vector<double>*)value;
}
std::vector<std::string>* JNode::toStrings() {
    if (!isArray() && (valueFlag >> 2) != DataType::String) throw std::bad_cast();
    return (std::vector<std::string>*)value;
}
std::vector<bool>* JNode::toBooleans() {
    if (!isArray() && (valueFlag >> 2) != DataType::Boolean) throw std::bad_cast();
    return (std::vector<bool>*)value;
}

JNOParser::JNOParser() {}

#ifdef _DEBUG
static ObjectNode* _dbgLastNode;
#endif

// big algorithm, please free me.
int JNOParser::avail(JNOParser::_StructType& entry, const char* source, int len, int levels) {
    int i, j;

    void* memory = nullptr;
    DataType arrayType;
    DataType valueType;
    JNode curNode;

    //Базовый случаи
    if (!len) return 0;

    for (i = 0; i < len && source[i];) {
        // has comment line
        j = i += ignoreComment(source + i, len - i);
        if (levels > 0 && source[i] == syntax.array_segments[1]) break;
        i += skip(source + i, len - i);
        // check property name
        if (!isProperty(source + j, i - j)) throw std::bad_exception();
        curNode = {};
        curNode.propertyName.append(source + j, static_cast<size_t>(i - j));
        i += trim(source + i, len - i);  // trim string
        // has comment line
        i += ignoreComment(source + i, len - i);
        i += trim(source + i, len - i);  // trim string
        // is block or array
        if (source[i] == syntax.array_segments[0]) {
            ++levels;
            if (isArray(source + i, j, len - i)) {
                j += i++;
                arrayType = DataType::Unknown;
                curNode.valueFlag = Node_ArrayFlag;
                for (; i < j;) {
                    i += ignoreComment(source + i, j - i);
                    // next index
                    if (source[i] == syntax.obstacle)
                        ++i;
                    else {
                        i += Get_FormatType(source + i, &memory, valueType);
                        if (valueType != DataType::Unknown) {
                            if (arrayType == DataType::Unknown) {
                                arrayType = valueType;

                                switch (arrayType) {
                                    case DataType::String:
                                        curNode.setMemory((void*)new std::vector<std::string> ());
                                        break;
                                    case DataType::Boolean:
                                        curNode.setMemory((void*)new std::vector<bool> ());
                                        break;
                                    case DataType::Real:
                                        curNode.setMemory((void*)new std::vector<double>());
                                        break;
                                    case DataType::Number:
                                        curNode.setMemory((void*)new std::vector<std::int64_t>());
                                        break;
                                }

                            } else if (arrayType != valueType && arrayType == DataType::Real && valueType != DataType::Number) {
                                throw std::runtime_error("Multi type is found.");
                            }

                            switch (arrayType) {
                                case DataType::String: {
                                    auto ref = (std::string*)memory;
                                    ((std::vector<std::string>*)curNode.value)->emplace_back(*ref);
                                    delete (ref);
                                    break;
                                }
                                case DataType::Boolean: {
                                    auto ref = (bool*)memory;
                                    ((std::vector<bool>*)curNode.value)->emplace_back(*ref);
                                    delete (ref);
                                    break;
                                }
                                case DataType::Real: {
                                    double* ref;
                                    if (valueType == DataType::Number) {
                                        ref = new double(static_cast<double>(*(std::int64_t*)memory));
                                        delete ((std::int64_t*)memory);
                                    } else
                                        ref = (double*)memory;

                                    ((std::vector<double>*)curNode.value)->emplace_back(*ref);
                                    delete (ref);
                                    break;
                                }
                                case DataType::Number: {
                                    auto ref = (std::int64_t*)memory;
                                    ((std::vector<std::int64_t>*)curNode.value)->emplace_back(*ref);
                                    delete (ref);
                                    break;
                                }
                            }
                        }
                    }
                    i += ignoreComment(source + i, j - i);
                }

                // shrink to fit
                switch (arrayType) {
                    case DataType::String: {
                        ((std::vector<std::string>*)curNode.value)->shrink_to_fit();
                        break;
                    }
                    case DataType::Boolean: {
                        ((std::vector<bool>*)curNode.value)->shrink_to_fit();
                        break;
                    }
                    case DataType::Real: {
                        ((std::vector<double>*)curNode.value)->shrink_to_fit();
                        break;
                    }
                    case DataType::Number: {
                        ((std::vector<int64_t>*)curNode.value)->shrink_to_fit();
                        break;
                    }
                }

                curNode.valueFlag |= (arrayType) << 2;
            } else {  // get the next node
                ++i;
                _StructType* _nodes = new _StructType;
                i += avail(*_nodes, source + i, len - i, levels);
                curNode.valueFlag = Node_StructFlag;
                curNode.setMemory(_nodes);
                i += ignoreComment(source + i, len - i);
            }
            --levels;
            if (source[i] != syntax.array_segments[1]) {
                throw std::bad_exception();
            }
            ++i;
        } else {  // get also value
            i += Get_FormatType(source + i, &memory, valueType);
            curNode.setMemory(memory);
            memory = nullptr;
            curNode.valueFlag = Node_ValueFlag | valueType << 2;
        }

        entry.insert(std::make_pair(j = stringToHash(curNode.propertyName.c_str()), curNode));

#if defined(QDEBUG) || defined(DEBUG)
        // get iter
        auto _curIter = entry.find(j);
        if (_dbgLastNode) _dbgLastNode->nextNode = &_curIter->second;
        _curIter->second.prevNode = _dbgLastNode;
        _dbgLastNode = &_curIter->second;
#endif
        i += ignoreComment(source + i, len - i);
    }

    return i;
}
void JNOParser::Deserialize(const char* filename) {
    long sz;
    char* buf;
    std::ifstream file;
    file.open(filename);
    sz = file.seekg(0, std::ios::end).tellg();
    file.seekg(0, std::ios::beg);

    if ((buf = (char*)malloc(sz)) == nullptr) throw std::bad_alloc();

    memset(buf, 0, sz);
    sz = file.read(buf, sz).gcount();
    file.close();
    Deserialize((char*)buf, sz);
    free(buf);
}
void JNOParser::Deserialize(const std::string& source) { Deserialize(source.data(), source.size()); }
void JNOParser::Deserialize(const char* source, int len) {
    int i;

    Clear();
#ifdef _DEBUG
    _dbgLastNode = nullptr;
#endif
    i = avail(entry, source, len);

    auto node = FindNode("first/ops/ps");
}
std::string JNOParser::Serialize() {
    std::string data{nullptr};
    return data;
}
JNode* JNOParser::GetNode(const std::string& name) {
    JNode* node = nullptr;
    int id = stringToHash(name.c_str());

    auto iter = this->entry.find(id);

    if (iter != std::end(this->entry)) node = &iter->second;

    return node;
}

JNOParser::_StructType& JNOParser::GetContainer() { return entry; }

JNode* JNOParser::FindNode(const std::string& nodePath) {
    JNode* node = nullptr;
    decltype(this->entry)* entry = &this->entry;
    decltype(entry->begin()) iter;
    int l = 0, r;
    int hash;
    // get splits
    do {
        if ((r = nodePath.find(syntax.nodePathBreaker, l)) == ~0) r = static_cast<int>(nodePath.length());
        hash = stringToHash(nodePath.c_str() + l, r - l);
        iter = entry->find(hash);
        if (iter != end(*entry)) {
            if (iter->second.isStruct())
                entry = decltype(entry)(iter->second.value);
            else {
                if (r == nodePath.length()) node = &iter->second;  // get the next section
                break;
            }
        }
        l = r + 1;
        r = nodePath.length();

    } while (l < r);
    return node;
}
bool JNOParser::ContainsNode(const std::string& nodePath) { return FindNode(nodePath) != nullptr; }
void JNOParser::Clear() {
    // todo: Free allocated data
    this->entry.clear();
}
}  // namespace RoninEngine
