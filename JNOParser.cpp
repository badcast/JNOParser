#include "JNOParser.hpp"

// TODO: feature - sync read
// TODO: read block from disk

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

constexpr int Node_ValueFlag = 1;
constexpr int Node_ArrayFlag = 2;
constexpr int Node_StructFlag = 3;

enum data_type { unknown = 0, string = 1, boolean = 2, real = 3, number = 4 };

template <typename T>
inline std::enable_if<!std::is_pointer<T>::value, T*> allocate() {
    T* mc = static_cast<T*>(malloc(sizeof(T)));
    return mc;
}

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
    bool real = false;
    if (getLength == nullptr) throw std::bad_exception();
    *getLength = 0;
    for (;;) {
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

static int Get_FormatType(const char* c, void** mem, JData& out) {
    int temp;

    if (mem) *mem = nullptr;

    // string type
    if (*c == syntax.format_string) {
        temp = 1;
        for (; c[temp] != syntax.format_string; ++temp)
            if (c[temp] == syntax.left_seperator && c[temp + 1] == syntax.format_string) ++temp;
        --temp;
        if (temp) {
            if (mem) {
                jstring str;
                str.reserve(temp);
                for (size_t i = 0; i < temp; ++i) {
                    if (c[i + 1] == syntax.left_seperator) ++i;
                    str.push_back(c[i + 1]);
                }
                *mem = reinterpret_cast<void*>(new jstring(str));
            }
            out = JData::String;
        } else
            out = JData::Unknown;

        temp += 2;
    } else if (is_real(c, &temp)) {
        if (mem) *mem = new double(atof(c));
        out = JData::Real;
    } else if (is_number(*c)) {
        if (mem) *mem = new int64_t(atoll(c));
        out = JData::Number;
    } else if (is_bool(c, &temp)) {
        if (mem) *mem = new bool(temp == sizeof(syntax.true_string) - 1);
        out = JData::Boolean;
    } else {
        out = JData::Unknown;
        temp = 0;
    }
    return temp;
}

static bool Has_DataType(const char* currentPtr) {
    JData type;
    Get_FormatType(const_cast<char*>(currentPtr), nullptr, type);
    return type != JData::Unknown;
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
    for (int i = 0; c[i] && i < len; ++i)
        if (!((toupper(c[i]) >= 'A' && toupper(c[i]) <= 'Z') || (i > 0 && is_number(c[i])) || c[i] == syntax.node_name_concqt))
            return false;
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
    int i;
    bool result = false;

    if (*c == syntax.array_segments[0]) {
        ++i;
        i += trim(c + i);

        i += ignoreComment(c + i, len - i);
        i += trim(c + i);
        if (Has_DataType(c + i) || c[i] == syntax.array_segments[1]) {
            for (i = 0; c[i] && i < len; ++i) {
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
            delete ((JNOParser::JStruct*)value);
        } else if (isArray()) {
            switch (valueFlag >> 2) {
                case JData::Boolean:
                    delete (toBooleans());
                    break;
                case JData::String:
                    delete (toStrings());
                    break;
                case JData::Number:
                    delete (toNumbers());
                    break;
                case JData::Real:
                    delete (toReals());
                    break;
            }
        } else if (isValue()) {
            if (isString()) {
                delete ((jstring*)value);
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
bool JNode::isNumber() { return ((this->valueFlag & 0x1C) >> 2) == JData::Number; }
bool JNode::isReal() { return ((this->valueFlag & 0x1C) >> 2) == JData::Real; }
bool JNode::isString() { return ((this->valueFlag & 0x1C) >> 2) == JData::String; }
bool JNode::isBoolean() { return ((this->valueFlag & 0x1C) >> 2) == JData::Boolean; }

jstring JNode::getPropertyName() { return this->propertyName; }
std::int64_t& JNode::toNumber() {
    if (!isNumber()) throw std::bad_cast();

    return *(std::int64_t*)this->value;
}
double& JNode::toReal() {
    if (!isReal()) throw std::bad_cast();
    return *(double*)this->value;
}
jstring& JNode::toString() {
    if (!isString()) throw std::bad_cast();
    return *(jstring*)this->value;
}
bool& JNode::toBoolean() {
    if (!isBoolean()) throw std::bad_cast();
    return *(bool*)this->value;
}
std::vector<std::int64_t>* JNode::toNumbers() {
    if (!isArray() && (valueFlag >> 2) != JData::Number) throw std::bad_cast();
    return (std::vector<std::int64_t>*)value;
}
std::vector<double>* JNode::toReals() {
    if (!isArray() && (valueFlag >> 2) != JData::Real) throw std::bad_cast();
    return (std::vector<double>*)value;
}
std::vector<jstring>* JNode::toStrings() {
    if (!isArray() && (valueFlag >> 2) != JData::String) throw std::bad_cast();
    return (std::vector<jstring>*)value;
}
std::vector<bool>* JNode::toBooleans() {
    if (!isArray() && (valueFlag >> 2) != JData::Boolean) throw std::bad_cast();
    return (std::vector<bool>*)value;
}

jno::JNode::operator int() { return static_cast<int>(toNumber()); }

jno::JNode::operator std::uint32_t() { return static_cast<std::uint32_t>(toNumber()); }

jno::JNode::operator std::int64_t() { return toNumber(); }

jno::JNode::operator std::uint64_t() { return toNumber(); }

jno::JNode::operator double() { return toReal(); }

jno::JNode::operator float() { return static_cast<float>(toReal()); }

jno::JNode::operator jstring() { return toString(); }

jno::JNode::operator bool() { return toBoolean(); }

jno::JNOParser::JNOParser() {}

std::size_t JNOParser::defrag_require_memory(){
    std::size_t _size = 0;
    if(this->memory_ordering == nullptr)
    {
        JStruct* recur = &entry;
        throw std::runtime_error("Memory is based");
    }
    return _size;
}

#ifdef DEBUG
static JNode* _dbgLastNode;
#endif

// big algorithm, please free me.
int jno::JNOParser::avail(JNOParser::JStruct& entry, const char* source, int len, int levels) {
    int i, j;
    void* memory = nullptr;
    JData arrayType;
    JData valueType;
    JNode curNode;

    //Базовый случаи
    if (!len) return 0;

    for (i = 0; i < len && source[i];) {
        // has comment line
        j = i += ignoreComment(source + i, len - i);
        if (levels > 0 && source[i] == syntax.array_segments[1]) break;
        i += skip(source + i, len - i);
        // check property name
        if (!isProperty(source + j, i - j)) throw std::runtime_error("incorrect property name");
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
                arrayType = JData::Unknown;
                curNode.valueFlag = Node_ArrayFlag;
                for (; i < j;) {
                    i += ignoreComment(source + i, j - i);
                    // next index
                    if (source[i] == syntax.obstacle)
                        ++i;
                    else {
                        i += Get_FormatType(source + i, &memory, valueType);
                        if (valueType != JData::Unknown) {
                            if (arrayType == JData::Unknown) {
                                arrayType = valueType;

                                switch (arrayType) {
                                    case JData::String:
                                        curNode.setMemory((void*)new std::vector<jstring>());
                                        break;
                                    case JData::Boolean:
                                        curNode.setMemory((void*)new std::vector<bool>());
                                        break;
                                    case JData::Real:
                                        curNode.setMemory((void*)new std::vector<double>());
                                        break;
                                    case JData::Number:
                                        curNode.setMemory((void*)new std::vector<std::int64_t>());
                                        break;
                                }

                            } else if (arrayType != valueType && arrayType == JData::Real && valueType != JData::Number) {
                                throw std::runtime_error("Multi type is found.");
                            }

                            switch (arrayType) {
                                case JData::String: {
                                    auto ref = (jstring*)memory;
                                    ((std::vector<jstring>*)curNode.value)->emplace_back(*ref);
                                    delete (ref);
                                    break;
                                }
                                case JData::Boolean: {
                                    auto ref = (bool*)memory;
                                    ((std::vector<bool>*)curNode.value)->emplace_back(*ref);
                                    delete (ref);
                                    break;
                                }
                                case JData::Real: {
                                    double* ref;
                                    if (valueType == JData::Number) {
                                        ref = new double(static_cast<double>(*(std::int64_t*)memory));
                                        delete ((std::int64_t*)memory);
                                    } else
                                        ref = (double*)memory;

                                    ((std::vector<double>*)curNode.value)->emplace_back(*ref);
                                    delete (ref);
                                    break;
                                }
                                case JData::Number: {
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
                    case JData::String: {
                        ((std::vector<jstring>*)curNode.value)->shrink_to_fit();
                        break;
                    }
                    case JData::Boolean: {
                        ((std::vector<bool>*)curNode.value)->shrink_to_fit();
                        break;
                    }
                    case JData::Real: {
                        ((std::vector<double>*)curNode.value)->shrink_to_fit();
                        break;
                    }
                    case JData::Number: {
                        ((std::vector<int64_t>*)curNode.value)->shrink_to_fit();
                        break;
                    }
                }

                curNode.valueFlag |= (arrayType) << 2;
            } else {  // get the next node
                ++i;
                JStruct* _nodes = new JStruct;
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

#if defined(G)
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
bool JNOParser::parse(const char* filename) {
    long sz;
    char* buf;
    std::ifstream file;
    file.open(filename);

    if (!file.good()) return false;

    sz = file.seekg(0, std::ios::end).tellg();
    file.seekg(0, std::ios::beg);

    if ((buf = (char*)malloc(sz)) == nullptr) throw std::bad_alloc();

    memset(buf, 0, sz);
    sz = file.read(buf, sz).gcount();
    file.close();
    Deserialize((char*)buf, sz);
    free(buf);
    return true;
}
bool JNOParser::parse(const jstring& filename) { return parse(filename.data()); }

bool JNOParser::Deserialize(const char* source, int len) {
    int i;

    Clear();
#ifdef DEBUG
    _dbgLastNode = nullptr;
#endif
    i = avail(entry, source, len);
}
jstring JNOParser::Serialize() {
    jstring data{nullptr};
    return data;
}
JNode* JNOParser::GetNode(const jstring& name) {
    JNode* node = nullptr;
    int id = stringToHash(name.c_str());

    auto iter = this->entry.find(id);

    if (iter != std::end(this->entry)) node = &iter->second;

    return node;
}

JNOParser::JStruct& JNOParser::GetContainer() { return entry; }

JNode* JNOParser::FindNode(const jstring& nodePath) {
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
                if (r == static_cast<int>(nodePath.length())) node = &iter->second;  // get the next section
                break;
            }
        }
        l = r + 1;
        r = nodePath.length();

    } while (l < r);
    return node;
}
bool JNOParser::ContainsNode(const jstring& nodePath) { return FindNode(nodePath) != nullptr; }
void JNOParser::Clear() {
    // todo: Free allocated data
    JStruct* entr = &this->entry;
    this->entry.clear();
}

jstring JNOParser::operator<<(std::iostream& ostream) {
    return Serialize();
}
}  // namespace jno
