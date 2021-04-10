#include "ObjectParser.h"
#include <stdexcept>

#include "framework_ex.h"

// TOP

namespace RoninEngine {

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
                *mem = reinterpret_cast<void*>(GC::gc_alloc<std::string>(str));
            }
            out = DataType::String;
        } else
            out = DataType::Unknown;

        temp += 2;
    } else if (config::is_real(c, &temp)) {
        if (mem) *mem = GC::gc_alloc<double>(atof(c));
        out = DataType::Real;
    } else if (config::is_number(*c)) {
        if (mem) *mem = GC::gc_alloc<int64_t>(atoll(c));
        out = DataType::Number;
    } else if (config::is_bool(c, &temp)) {
        if (mem) *mem = GC::gc_alloc<bool>(temp == sizeof(syntax.true_string) - 1);
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

inline int trim(const char* c, int len = INT_MAX) {
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

inline int skip(const char* c, int len = INT_MAX) {
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
        if (!(toupper(c[i]) >= 'A' && toupper(c[i]) <= 'Z' || (i > 0 && config::is_number(c[i])) || c[i] == '_')) return false;
    return len != 0;
}

inline bool isCommentLine(const char* c, int len) { return len > 0 && !strncmp(c, syntax.commentLine, Mathf::Min(2, len)); }

inline int ignoreComment(const char* c, int len) {
    int i = 0;
    i += trim(c + i, len - i);
    while (isCommentLine(c + i, len - i)) {
        i += config::go_end_line(c + i, len - i);
        i += trim(c + i, len - i);
    }
    return i;
}

inline bool isSpace(char c) {
    for (int i = 0; i <= sizeof(syntax.trim_segments); ++i)
        if (syntax.trim_segments[i] == c) return true;
    return false;
}

static bool isArray(const char* c, int& endpoint, int len = INT_MAX) {
    int i = 0;
    bool result = false;

    if (*c == syntax.array_segments[0]) {
        ++i;
        i += trim(c + i);

        i += ignoreComment(c + i, len - i);
        i += trim(c + i);
        if (config::Has_DataType(c + i) || c[i] == syntax.array_segments[1]) {
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

ObjectNode::ObjectNode(const ObjectNode& copy) {
    this->value = copy.value;
    this->valueFlag = copy.valueFlag;
    this->propertyName = copy.propertyName;
    decrementMemory();
    uses = copy.uses;
    incrementMemory();
    copy.uses = uses;
}
ObjectNode& ObjectNode::operator=(const ObjectNode& copy) {
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
ObjectNode::~ObjectNode() {
    if (!decrementMemory() && value) {
        if (isStruct()) {
            GC::gc_unalloc((ObjectParser::_StructType*)value);
        } else if (isArray()) {
            switch (valueFlag >> 2) {
                case DataType::Boolean:
                    GC::gc_unalloc(toBooleans());
                    break;
                case DataType::String:
                    GC::gc_unalloc(toStrings());
                    break;
                case DataType::Number:
                    GC::gc_unalloc(toNumbers());
                    break;
                case DataType::Real:
                    GC::gc_unalloc(toReals());
                    break;
            }
        } else if (isValue()) {
            if (isString()) {
                GC::gc_unalloc((std::string*)value);
            } else
                GC::gc_free(value);
        }
    }
}

void*& ObjectNode::setMemory(void* mem) {
    value = mem;

    return mem;
}

int ObjectNode::decrementMemory() {
    int res = 0;
    if (uses)
        if (!(res = --(*uses))) {
            res = *uses;
            GC::gc_unalloc(uses);
        }
    uses = nullptr;
    return res;
}

int ObjectNode::incrementMemory() {
    if (!uses) uses = GC::gc_alloc<int>();
    return ++*uses;
}

bool ObjectNode::isValue() { return (this->valueFlag & 3) == Node_ValueFlag; }
bool ObjectNode::isArray() { return (this->valueFlag & 3) == Node_ArrayFlag; }
bool ObjectNode::isStruct() { return (this->valueFlag & 3) == Node_StructFlag; }
bool ObjectNode::isNumber() { return ((this->valueFlag & 0x1C) >> 2) == DataType::Number; }
bool ObjectNode::isReal() { return ((this->valueFlag & 0x1C) >> 2) == DataType::Real; }
bool ObjectNode::isString() { return ((this->valueFlag & 0x1C) >> 2) == DataType::String; }
bool ObjectNode::isBoolean() { return ((this->valueFlag & 0x1C) >> 2) == DataType::Boolean; }

std::string& ObjectNode::getPropertyName() { return this->propertyName; }
std::int64_t& ObjectNode::toNumber() {
    if (!isNumber()) throw std::bad_cast();

    return *(std::int64_t*)this->value;
}
double& ObjectNode::toReal() {
    if (!isReal()) throw std::bad_cast();
    return *(double*)this->value;
}
std::string& ObjectNode::toString() {
    if (!isString()) throw std::bad_cast();
    return *(std::string*)this->value;
}
bool& ObjectNode::toBoolean() {
    if (!isBoolean()) throw std::bad_cast();
    return *(bool*)this->value;
}
vector<std::int64_t>* ObjectNode::toNumbers() {
    if (!isArray() && (valueFlag >> 2) != DataType::Number) throw std::bad_cast();
    return (vector<std::int64_t>*)value;
}
vector<double>* ObjectNode::toReals() {
    if (!isArray() && (valueFlag >> 2) != DataType::Real) throw std::bad_cast();
    return (vector<double>*)value;
}
vector<std::string>* ObjectNode::toStrings() {
    if (!isArray() && (valueFlag >> 2) != DataType::String) throw std::bad_cast();
    return (vector<std::string>*)value;
}
vector<bool>* ObjectNode::toBooleans() {
    if (!isArray() && (valueFlag >> 2) != DataType::Boolean) throw std::bad_cast();
    return (vector<bool>*)value;
}

ObjectParser::ObjectParser() {}

#ifdef _DEBUG
static ObjectNode* _dbgLastNode;
#endif

// big algorithm, please free me.
int ObjectParser::avail(ObjectParser::_StructType& entry, const char* source, int len, int levels) {
    int i, j;

    void* memory = nullptr;
    DataType arrayType;
    DataType valueType;
    ObjectNode curNode;

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
                                        curNode.setMemory((void*)GC::gc_alloc<std::vector<std::string>>());
                                        break;
                                    case DataType::Boolean:
                                        curNode.setMemory((void*)GC::gc_alloc<std::vector<bool>>());
                                        break;
                                    case DataType::Real:
                                        curNode.setMemory((void*)GC::gc_alloc<std::vector<double>>());
                                        break;
                                    case DataType::Number:
                                        curNode.setMemory((void*)GC::gc_alloc<std::vector<std::int64_t>>());
                                        break;
                                }

                            } else if (arrayType != valueType && arrayType == DataType::Real && valueType != DataType::Number) {
                                throw std::runtime_error("Multi type is found.");
                            }

                            switch (arrayType) {
                                case DataType::String: {
                                    auto ref = (std::string*)memory;
                                    ((vector<string>*)curNode.value)->emplace_back(*ref);
                                    GC::gc_unalloc(ref);
                                    break;
                                }
                                case DataType::Boolean: {
                                    auto ref = (bool*)memory;
                                    ((vector<bool>*)curNode.value)->emplace_back(*ref);
                                    GC::gc_unalloc(ref);
                                    break;
                                }
                                case DataType::Real: {
                                    double* ref;
                                    if (valueType == DataType::Number) {
                                        ref = new double(static_cast<double>(*(std::int64_t*)memory));
                                        GC::gc_unalloc((std::int64_t*)memory);
                                    } else
                                        ref = (double*)memory;

                                    ((vector<double>*)curNode.value)->emplace_back(*ref);
                                    GC::gc_unalloc(ref);
                                    break;
                                }
                                case DataType::Number: {
                                    auto ref = (std::int64_t*)memory;
                                    ((vector<int64_t>*)curNode.value)->emplace_back(*ref);
                                    GC::gc_unalloc(ref);
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
                        ((vector<string>*)curNode.value)->shrink_to_fit();
                        break;
                    }
                    case DataType::Boolean: {
                        ((vector<bool>*)curNode.value)->shrink_to_fit();
                        break;
                    }
                    case DataType::Real: {
                        ((vector<double>*)curNode.value)->shrink_to_fit();
                        break;
                    }
                    case DataType::Number: {
                        ((vector<int64_t>*)curNode.value)->shrink_to_fit();
                        break;
                    }
                }

                curNode.valueFlag |= (arrayType) << 2;
            } else {  // get the next node
                ++i;
                _StructType* _nodes = GC::gc_alloc<_StructType>();
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

        entry.insert(make_pair(j = stringToHash(curNode.propertyName.c_str()), curNode));

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
void ObjectParser::Deserialize(const char* filename) {
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
void ObjectParser::Deserialize(const std::string& source) { Deserialize(source.data(), source.size()); }
void ObjectParser::Deserialize(const char* source, int len) {
    int i;

    Clear();
#ifdef _DEBUG
    _dbgLastNode = nullptr;
#endif
    i = avail(entry, source, len);

    auto node = FindNode("first/ops/ps");
}
std::string ObjectParser::Serialize() {
    std::string data{nullptr};
    return data;
}
ObjectNode* ObjectParser::GetNode(const std::string& name) {
    ObjectNode* node = nullptr;
    int id = stringToHash(name.c_str());

    auto iter = this->entry.find(id);

    if (iter != std::end(this->entry)) node = &iter->second;

    return node;
}

ObjectParser::_StructType& ObjectParser::GetContainer() { return entry; }

ObjectNode* ObjectParser::FindNode(const std::string& nodePath) {
    ObjectNode* node = nullptr;
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
bool ObjectParser::ContainsNode(const std::string& nodePath) { return FindNode(nodePath) != nullptr; }
void ObjectParser::Clear() {
    // todo: Free allocated data
    this->entry.clear();
}
}  // namespace RoninEngine
