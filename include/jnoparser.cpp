#include <cstdlib>
#include <limits>

#include "jnoparser.h"

namespace jno {

static const struct {
    char jno_dot = '.';
    char jno_obstacle = ',';
    char jno_space = ' ';
    char jno_nodePathBreaker = '/';
    char jno_commentLine[3] = "//";
    char jno_left_seperator = '\\';
    char jno_eof_segment = '\n';
    char jno_format_string = '\"';
    char jno_true_string[5] = "true";
    char jno_false_string[6] = "false";
    char jno_array_segments[2]{'{', '}'};
    char jno_trim_segments[5]{' ', '\t', '\n', '\r', '\v'};
    char jno_valid_property_char = '_';
} jno_syntax;

enum { Node_ValueFlag = 1, Node_ArrayFlag = 2, Node_StructFlag = 3 };

struct jno_evaluated {
    std::uint16_t jstrings;
    std::uint32_t jstrings_total_bytes;
    std::uint16_t jnumbers;
    std::uint16_t jreals;
    std::uint16_t jbools;
    std::uint16_t jarrstrings;
    std::uint32_t jarrstrings_total_bytes;
    std::uint16_t jarrnumbers;
    std::uint32_t jarrnumbers_total_bytes;
    std::uint16_t jarrreals;
    std::uint32_t jarrreals_total_bytes;
    std::uint16_t jarrbools;
    std::uint32_t jarrbools_total_bytes;
    std::uint16_t jdepths;

    jnumber calcBytes() {
        jnumber sz = 0;

        // calc jstring
        sz += jstrings_total_bytes;

        // calc jnumber
        sz += jnumbers * sizeof(jnumber);

        // calc jreals
        sz += jreals * sizeof(jreal);

        // calc jbools
        sz += jbools * sizeof(jbool);

        // calc arrays
        sz += jarrstrings_total_bytes + jarrnumbers_total_bytes + jarrbools_total_bytes + jarrreals_total_bytes;

        return sz;
    }
};

template <typename T>
inline T* jalloc() {
    return new T();  // static_cast<T*>(std::malloc(sizeof(T)));//RoninEngine::Runtime::GC::gc_alloc<T>();
}

template <typename T>
inline T* jalloc(const T& copy) {
    T* ptr = static_cast<T*>(std::malloc(sizeof(T)));  // RoninEngine::Runtime::GC::gc_alloc<T>(copy);
    std::memcpy(ptr, &copy, sizeof(T));
    return ptr;
}

template <typename T>
inline void jfree(T* pointer) {
    free(pointer);
    // RoninEngine::Runtime::GC::gc_unalloc(pointer);
}

inline void jfree(void* pointer) { std::free(pointer); }

jno_evaluated jno_analize(jno_object_parser* parser) {
    jno_evaluated eval = {};

    return eval;
}

void jno_evaluate(jno_evaluated* file) {}

jbool jno_is_jnumber(const char character) { return std::isdigit(character) || character == '-'; }

jbool jno_is_jreal(const char* content, int* getLength) {
    bool real = false;
    for (;;) {
        if (!jno_is_jnumber(content[*getLength])) {
            if (real) break;

            real = (content[*getLength] == jno_syntax.jno_dot);

            if (!real) break;
        }
        ++*getLength;
    }
    return real;
}

jbool jno_is_jbool(const char* content, int* getLength) {
    if (!strncmp(content, jno_syntax.jno_true_string, *getLength = sizeof(jno_syntax.jno_true_string) - 1)) return true;
    if (!strncmp(content, jno_syntax.jno_false_string, *getLength = sizeof(jno_syntax.jno_false_string) - 1)) return true;
    *getLength = 0;
    return false;
}

int jno_format(const char* content, void** mem, JNOType& out) {
    int offset = 0;

    out = JNOType::Unknown;

    if (mem) *mem = nullptr;

    // string type
    if (*content == jno_syntax.jno_format_string) {
        ++offset;
        for (; content[offset] != jno_syntax.jno_format_string; ++offset)
            if (content[offset] == jno_syntax.jno_left_seperator && content[offset + 1] == jno_syntax.jno_format_string)
                ++offset;
        if (--offset) {
            if (mem) {
                jstring str;
                str.reserve(offset);
                for (size_t i = 0; i < offset; ++i) {
                    if (content[i + 1] == jno_syntax.jno_left_seperator) ++i;
                    str.push_back(content[i + 1]);
                }
                *mem = reinterpret_cast<void*>(jalloc(jstring(str)));
            }
            out = JNOType::JNOString;
        }

        offset += 2;
    } else if (jno_is_jreal(content, &offset)) {
        if (mem) *mem = jalloc<jreal>(static_cast<double>(atof(content)));
        out = JNOType::JNOReal;
    } else if (jno_is_jnumber(*content)) {
        if (mem) *mem = jalloc<jnumber>(atoll(content));
        out = JNOType::JNONumber;
    } else if (jno_is_jbool(content, &offset)) {
        if (mem) *mem = jalloc<jbool>(offset == sizeof(jno_syntax.jno_true_string) - 1);
        out = JNOType::JNOBoolean;
    }  // another type

    return offset;
}

jbool jno_has_datatype(const char* content) {
    JNOType type;
    jno_format(const_cast<char*>(content), nullptr, type);
    return type != JNOType::Unknown;
}

int jno_trim(const char* content, int contentLength = std::numeric_limits<int>::max()) {
    int i = 0, j;
    if (content != nullptr)
        for (; i < contentLength && content[i];) {
            for (j = 0; j < static_cast<int>(sizeof(jno_syntax.jno_trim_segments));)
                if (content[i] == jno_syntax.jno_trim_segments[j])
                    break;
                else
                    ++j;
            if (j != sizeof(jno_syntax.jno_trim_segments))
                ++i;
            else
                break;
        }
    return i;
}

int jno_skip(const char* c, int len = std::numeric_limits<int>::max()) {
    int i, j;
    char skipping[sizeof(jno_syntax.jno_trim_segments) + sizeof(jno_syntax.jno_array_segments)];
    skipping[0] = '\0';
    strncpy(skipping, jno_syntax.jno_trim_segments, sizeof(jno_syntax.jno_trim_segments));
    strncpy(skipping + sizeof jno_syntax.jno_trim_segments, jno_syntax.jno_array_segments,
            sizeof(jno_syntax.jno_array_segments));
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

jbool jno_is_property(const char* abstractContent, int len) {
    int x;
    for (x ^= x; x < len; ++x, ++abstractContent)
        if (!((*abstractContent >= 'A' && *abstractContent <= 'z') ||
              (x && *abstractContent != '-' && jno_is_jnumber(*abstractContent)) ||
              *abstractContent == jno_syntax.jno_valid_property_char))
            return false;
    return len != 0;
}

jbool jno_is_comment_line(const char* c, int len) {
    return len > 0 && !strncmp(c, jno_syntax.jno_commentLine, std::min(2, len));
}

int jno_move_eof(const char* c, int len = std::numeric_limits<int>::max()) {
    int i = 0;
    for (; c[i] && i < len && c[i] != jno_syntax.jno_eof_segment; ++i)
        ;
    return i;
}

int jno_skip_comment(const char* c, int len) {
    int i = jno_trim(c, len);
    while (jno_is_comment_line(c + i, len - i)) {
        i += jno_move_eof(c + i, len - i);
        i += jno_trim(c + i, len - i);
    }
    return i;
}

jbool jno_is_space(char c) {
    for (int i = 0; i <= sizeof(jno_syntax.jno_trim_segments); ++i)
        if (jno_syntax.jno_trim_segments[i] == c) return true;
    return false;
}

jbool jno_is_array(const char* content, int& endpoint, int contentLength = std::numeric_limits<int>::max()) {
    int i = 0;
    jbool result = false;

    if (*content == jno_syntax.jno_array_segments[0]) {
        ++i;
        i += jno_trim(content + i);

        i += jno_skip_comment(content + i, contentLength - i);
        i += jno_trim(content + i);

        if (jno_has_datatype(content + i) || content[i] == jno_syntax.jno_array_segments[1]) {
            for (; content[i] && i < contentLength; ++i) {
                if (content[i] == jno_syntax.jno_array_segments[0])
                    break;
                else if (content[i] == jno_syntax.jno_array_segments[1]) {
                    endpoint = i;
                    result = true;
                    break;
                }
            }
        }
    }
    return result;
}

int jno_string_to_hash(const char* content, int contentLength) {
    int x = 1;
    int y = 0;
    while (*(content + y) && y < contentLength) x *= (int)*(content + y++);
    return x;
}

int jno_object_node::decrementMemory() {
    int res = 0;
    if (uses)
        if (!(res = --(*uses))) {
            res = *uses;
            jfree(uses);
        }
    uses = nullptr;
    return res;
}

int jno_object_node::incrementMemory() {
    if (!uses) uses = jalloc<int>();
    return ++*uses;
}

jbool jno_object_node::isValue() { return (this->valueFlag & 3) == Node_ValueFlag; }
jbool jno_object_node::isArray() { return (this->valueFlag & 3) == Node_ArrayFlag; }
jbool jno_object_node::isStruct() { return (this->valueFlag & 3) == Node_StructFlag; }
jbool jno_object_node::isNumber() { return ((this->valueFlag & 0x1C) >> 2) == JNOType::JNONumber; }
jbool jno_object_node::isReal() { return ((this->valueFlag & 0x1C) >> 2) == JNOType::JNOReal; }
jbool jno_object_node::isString() { return ((this->valueFlag & 0x1C) >> 2) == JNOType::JNOString; }
jbool jno_object_node::isBoolean() { return ((this->valueFlag & 0x1C) >> 2) == JNOType::JNOBoolean; }

jno_object_node* jno_object_node::tree(const jstring& child) { return nullptr; }

jstring& jno_object_node::getPropertyName() { return this->propertyName; }
void jno_object_node::set_native_memory(void* memory) { this->handle = memory; }
jnumber& jno_object_node::toNumber() {
    if (!isNumber()) throw std::bad_cast();

    return *(jnumber*)this->handle;
}
jreal& jno_object_node::toReal() {
    if (!isReal()) throw std::bad_cast();
    return *(jreal*)this->handle;
}
jstring& jno_object_node::toString() {
    if (!isString()) throw std::bad_cast();
    return *(jstring*)this->handle;
}
jbool& jno_object_node::toBoolean() {
    if (!isBoolean()) throw std::bad_cast();
    return *(jbool*)this->handle;
}
std::vector<jnumber>* jno_object_node::toNumbers() {
    if (!isArray() && (valueFlag >> 2) != JNOType::JNONumber) throw std::bad_cast();
    return (std::vector<jnumber>*)handle;
}
std::vector<jreal>* jno_object_node::toReals() {
    if (!isArray() && (valueFlag >> 2) != JNOType::JNOReal) throw std::bad_cast();
    return (std::vector<jreal>*)handle;
}
std::vector<jstring>* jno_object_node::toStrings() {
    if (!isArray() && (valueFlag >> 2) != JNOType::JNOString) throw std::bad_cast();
    return (std::vector<jstring>*)handle;
}
std::vector<jbool>* jno_object_node::toBooleans() {
    if (!isArray() && (valueFlag >> 2) != JNOType::JNOBoolean) throw std::bad_cast();
    return (std::vector<jbool>*)handle;
}

jno_object_parser::jno_object_parser() {}
jno_object_parser::~jno_object_parser() {}

int jno_avail_only(jno_evaluated& eval, const char* jno_source, int length, int depth) {}

// big algorithm, please free me.
// divide and conquer method avail
int jno_avail(jstruct& entry, jno_evaluated& eval, const char* jno_source, int length, int depth) {
    int x, y;
    JNOType valueType;
    JNOType current_block_type;
    jno_object_node prototype_node;
    void* pointer;

    // base step
    if (!length) return 0;

    for (x ^= x; x < length && jno_source[x];) {
        // has comment line
        y = x += jno_skip_comment(jno_source + x, length - x);
        if (depth > 0 && jno_source[x] == jno_syntax.jno_array_segments[1]) break;
        x += jno_skip(jno_source + x, length - x);
        // check property name
        if (!jno_is_property(jno_source + y, x - y)) throw std::bad_exception();
        prototype_node = {};
        prototype_node.propertyName.append(jno_source + y, static_cast<size_t>(x - y));
        x += jno_trim(jno_source + x, length - x);  // trim string
        // has comment line
        x += jno_skip_comment(jno_source + x, length - x);
        x += jno_trim(jno_source + x, length - x);  // trim string
        // is block or array
        if (jno_source[x] == jno_syntax.jno_array_segments[0]) {
            ++depth;
            if (jno_is_array(jno_source + x, y, length - x)) {
                y += x++;
                current_block_type = JNOType::Unknown;
                prototype_node.valueFlag = Node_ArrayFlag;
                for (; x < y;) {
                    x += jno_skip_comment(jno_source + x, y - x);
                    // next index
                    if (jno_source[x] == jno_syntax.jno_obstacle)
                        ++x;
                    else {
                        x += jno_format(jno_source + x, &pointer, valueType);
                        if (valueType != JNOType::Unknown) {
                            if (current_block_type == JNOType::Unknown) {
                                current_block_type = valueType;

                                switch (current_block_type) {
                                    case JNOType::JNOString:
                                        prototype_node.set_native_memory((void*)jalloc<std::vector<jstring>>());
                                        break;
                                    case JNOType::JNOBoolean:
                                        prototype_node.set_native_memory((void*)jalloc<std::vector<jbool>>());
                                        break;
                                    case JNOType::JNOReal:
                                        prototype_node.set_native_memory((void*)jalloc<std::vector<jreal>>());
                                        break;
                                    case JNOType::JNONumber:
                                        prototype_node.set_native_memory((void*)jalloc<std::vector<jnumber>>());
                                        break;
                                }

                            } else if (current_block_type != valueType && current_block_type == JNOType::JNOReal &&
                                       valueType != JNOType::JNONumber) {
                                throw std::runtime_error("Multi type is found.");
                            }

                            switch (current_block_type) {
                                case JNOType::JNOString: {
                                    auto ref = (jstring*)pointer;
                                    ((std::vector<jstring>*)prototype_node.handle)->emplace_back(*ref);
                                    jfree(ref);
                                    break;
                                }
                                case JNOType::JNOBoolean: {
                                    auto ref = (jbool*)pointer;
                                    ((std::vector<jbool>*)prototype_node.handle)->emplace_back(*ref);
                                    jfree(ref);
                                    break;
                                }
                                case JNOType::JNOReal: {
                                    jreal* ref;
                                    if (valueType == JNOType::JNONumber) {
                                        ref = new jreal(static_cast<jreal>(*(jnumber*)pointer));
                                        jfree((jnumber*)pointer);
                                    } else
                                        ref = (jreal*)pointer;

                                    ((std::vector<jreal>*)prototype_node.handle)->emplace_back(*ref);
                                    jfree(ref);
                                    break;
                                }
                                case JNOType::JNONumber: {
                                    auto ref = (jnumber*)pointer;
                                    ((std::vector<jnumber>*)prototype_node.handle)->emplace_back(*ref);
                                    jfree(ref);
                                    break;
                                }
                            }
                        }
                    }
                    x += jno_skip_comment(jno_source + x, y - x);
                }

                // shrink to fit
                switch (current_block_type) {
                    case JNOType::JNOString: {
                        ((std::vector<jstring>*)prototype_node.handle)->shrink_to_fit();
                        break;
                    }
                    case JNOType::JNOBoolean: {
                        ((std::vector<jbool>*)prototype_node.handle)->shrink_to_fit();
                        break;
                    }
                    case JNOType::JNOReal: {
                        ((std::vector<jreal>*)prototype_node.handle)->shrink_to_fit();
                        break;
                    }
                    case JNOType::JNONumber: {
                        ((std::vector<jnumber>*)prototype_node.handle)->shrink_to_fit();
                        break;
                    }
                }

                prototype_node.valueFlag |= (current_block_type) << 2;
            } else {  // get the next node
                jstruct* _nodes = jalloc<jstruct>();
                ++x;
                x += jno_avail(*_nodes, eval, jno_source + x, length - x, depth);
                prototype_node.valueFlag = Node_StructFlag;
                prototype_node.set_native_memory(_nodes);
                x += jno_skip_comment(jno_source + x, length - x);
            }
            --depth;
            if (jno_source[x] != jno_syntax.jno_array_segments[1]) {
                throw std::bad_exception();
            }
            ++x;
        } else {  // get also value
            x += jno_format(jno_source + x, &pointer, valueType);
            prototype_node.set_native_memory(pointer);
            pointer = nullptr;
            prototype_node.valueFlag = Node_ValueFlag | valueType << 2;
        }

        entry.insert(std::make_pair(y = jno_string_to_hash(prototype_node.propertyName.c_str()), prototype_node));
        /*
        #if defined(QDEBUG) || defined(DEBUG)
                // get iter
                auto _curIter = entry.find(j);
                if (_dbgLastNode) _dbgLastNode->nextNode = &_curIter->second;
                _curIter->second.prevNode = _dbgLastNode;
                _dbgLastNode = &_curIter->second;
        #endif
        */
        x += jno_skip_comment(jno_source + x, length - x);
    }

    return x;
}
void jno_object_parser::deserialize_from(const char* filename) {
    long length;
    char* buffer;

    std::ifstream file;

    file.open(filename);

    if (!file) throw std::runtime_error("error open file");

    length = file.seekg(0, std::ios::end).tellg();
    file.seekg(0, std::ios::beg);

    if ((buffer = (char*)malloc(length)) == nullptr) throw std::bad_alloc();

    memset(buffer, 0, length);
    length = file.read(buffer, length).gcount();
    file.close();
    deserialize((char*)buffer, length);
    free(buffer);
}
void jno_object_parser::deserialize(const jstring& source) { deserialize(source.data(), source.size()); }
void jno_object_parser::deserialize(const char* source, int len) {
    // FIXME: CLEAR FUNCTION IS UPGRADE
    entry.clear();  // clears alls
    jno_avail(entry, source, len);
}
jstring jno_object_parser::serialize() {
    jstring data;
    throw std::runtime_error("no complete");
    return data;
}
jno_object_node* jno_object_parser::at(const jstring& name) {
    jno_object_node* node = nullptr;
    int id = jno_string_to_hash(name.c_str());

    auto iter = this->entry.find(id);

    if (iter != std::end(this->entry)) node = &iter->second;

    return node;
}

jno_object_node* jno_object_parser::tree(const jstring& nodename) { return at(nodename); }

jno_object_parser::jstruct& jno_object_parser::get_struct() { return entry; }

jno_object_node* jno_object_parser::find_node(const jstring& nodePath) {
    jno_object_node* node = nullptr;
    decltype(this->entry)* entry = &this->entry;
    decltype(entry->begin()) iter;
    int l = 0, r;
    int hash;
    // get splits
    do {
        if ((r = nodePath.find(jno_syntax.jno_nodePathBreaker, l)) == ~0) r = static_cast<int>(nodePath.length());
        hash = jno_string_to_hash(nodePath.c_str() + l, r - l);
        iter = entry->find(hash);
        if (iter != end(*entry)) {
            if (iter->second.isStruct())
                entry = decltype(entry)(iter->second._bits);
            else {
                if (r == nodePath.length()) node = &iter->second;  // get the next section
                break;
            }
        }
        l = ++r;
        r = nodePath.length();

    } while (l < r);
    return node;
}

jbool jno_object_parser::contains(const jstring& nodePath) { return find_node(nodePath) != nullptr; }

jno_object_node& operator<<(jno_object_node& root, const jstring& nodename) { return *root.tree(nodename); }
jno_object_node& operator<<(jno_object_parser& root, const jstring& nodename) { return *root.tree(nodename); }

}  // namespace jno
