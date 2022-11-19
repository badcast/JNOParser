#include <cstdlib>
#include <limits>

#include "jnoparser"

#define method

#define MACROPUT(base) #base

namespace jno {

static const struct {
    char jno_dot = '.';
    char jno_obstacle = ',';
    char jno_nodePathBreaker = '/';
    char jno_commentLine[3] = "//";
    char jno_left_seperator = '\\';
    char jno_eol_segment = '\n';
    char jno_format_string = '\"';
    char jno_true_string[5] = MACROPUT(true);
    char jno_false_string[6] = MACROPUT(false);
    char jno_null_string[5] = "null";
    char jno_array_segments[2]{'{', '}'};
    char jno_unknown_string[6]{"{...}"};
    char jno_trim_segments[5]{32, '\t', '\n', '\r', '\v'};
    char jno_valid_property_name[3] = {'A', 'z', '_'};
} jno_syntax;

// TODO: Get status messages
// static const struct { const char* msg_multitype_cast = ""; } jno_error_messages;

enum { Node_ValueFlag = 1, Node_ArrayFlag = 2, Node_StructFlag = 3 };

// NOTE: storage description
/*
    index  |types
    -------|---------------|
    0       bools
    1       numbers
    2       reals
    3       strings
    4       properties
*/

struct jno_storage {
    // up members: meta-info
    jnumber numBools, numNumbers, numReals, numStrings, numProperties, arrayBools, arrayNumbers, arrayReals, arrayStrings;
};

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
        jnumber sz;

        sz ^= sz;

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
    std::free(pointer);
    // RoninEngine::Runtime::GC::gc_unalloc(pointer);
}

inline void jfree(void* pointer) { std::free(pointer); }

// method for get type from pointer (storage required)
method JNOType jno_get_type(const void* pointer, jno_storage* pstorage) {
    const jnumber* alpha = reinterpret_cast<jnumber*>(pstorage);
    const void* delta = pstorage + 1;
    int type;
    if (pointer) {
        type = JNOType::Unknown;
        for (; pointer < delta; ++alpha) {
            // get type
            ++type;
            // set next pointer
            delta += static_cast<std::uint32_t>(*alpha >> 32);  // high (bytes)
        }
    } else
        // ops: Type is null, var is empty
        type = JNOType::Null;

    return static_cast<JNOType>(type);
}
// method for check valid a unsigned number
method inline bool jno_is_unsigned_jnumber(const char character) { return std::isdigit(character); }

// method for check valid a signed number
method inline bool jno_is_jnumber(const char character) { return jno_is_unsigned_jnumber(character) || character == '-'; }

// method for check is real ?
method jbool jno_is_jreal(const char* content, int* getLength) {
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

// method for check is bool ?
method inline jbool jno_is_jbool(const char* content, int* getLength) {
    if (!strncmp(content, jno_syntax.jno_true_string, *getLength = sizeof(jno_syntax.jno_true_string) - 1)) return true;
    if (!strncmp(content, jno_syntax.jno_false_string, *getLength = sizeof(jno_syntax.jno_false_string) - 1)) return true;
    *getLength ^= *getLength;
    return false;
}

// method for get format from raw content, also to write in mem pointer
method int jno_get_format(const char* content, void** mem, JNOType& out) {
    int offset;
    size_t i;

    offset ^= offset;  // set to zero

    // string type
    if (content == nullptr || *content == '\0')
        out = JNOType::Null;
    else if (*content == jno_syntax.jno_format_string) {
        ++offset;
        for (; content[offset] != jno_syntax.jno_format_string; ++offset)
            if (content[offset] == jno_syntax.jno_left_seperator && content[offset + 1] == jno_syntax.jno_format_string)
                ++offset;
        if (--offset) {
            if (mem) {
                jstring str;
                str.reserve(offset);
                for (i ^= i; i < offset; ++i) {
                    if (content[i + 1] == jno_syntax.jno_left_seperator) ++i;
                    str.push_back(content[i + 1]);
                }
                *mem = static_cast<void*>(jalloc(jstring(str)));
            }
        }

        offset += 2;
        out = JNOType::JNOString;
    } else if (jno_is_jreal(content, &offset)) {
        if (mem) *mem = jalloc<jreal>(static_cast<jreal>(atof(content)));
        out = JNOType::JNOReal;
    } else if (jno_is_jnumber(*content)) {
        if (mem) *mem = jalloc<jnumber>(atoll(content));
        out = JNOType::JNONumber;
    } else if (jno_is_jbool(content, &offset)) {
        if (mem) *mem = jalloc<jbool>(offset == sizeof(jno_syntax.jno_true_string) - 1);
        out = JNOType::JNOBoolean;
    } else  // another type
        out = JNOType::Unknown;

    return offset;
}

// method for check. has type in value
method inline jbool jno_has_datatype(const char* content) {
    JNOType type;
    jno_get_format(const_cast<char*>(content), nullptr, type);
    return type != JNOType::Unknown;
}

// method for trim
method int jno_trim(const char* content, int contentLength = std::numeric_limits<int>::max()) {
    int x, y;
    x ^= x;
    if (content != nullptr)
        for (; x < contentLength && *content;) {
            for (y ^= y; y < static_cast<int>(sizeof(jno_syntax.jno_trim_segments));)
                if (*content == jno_syntax.jno_trim_segments[y])
                    break;
                else
                    ++y;
            if (y != sizeof(jno_syntax.jno_trim_segments)) {
                ++x;
                ++content;
            } else
                break;
        }
    return x;
}

method int jno_skip(const char* c, int len = std::numeric_limits<int>::max()) {
    int x, y;
    char skipping[sizeof(jno_syntax.jno_trim_segments) + sizeof(jno_syntax.jno_array_segments)];
    *skipping ^= *skipping;
    strncpy(skipping, jno_syntax.jno_trim_segments, sizeof(jno_syntax.jno_trim_segments));
    strncpy(skipping + sizeof jno_syntax.jno_trim_segments, jno_syntax.jno_array_segments,
            sizeof(jno_syntax.jno_array_segments));
    for (x ^= x; c[x] && x <= len;) {
        for (y ^= y; y < sizeof(skipping);)
            if (c[x] == skipping[y])
                break;
            else
                ++y;
        if (y == sizeof(skipping))
            ++x;
        else
            break;
    }
    return x;
}

method inline jbool jno_valid_property_name(const char* abstractContent, int len) {
    int x;
    for (x ^= x; x < len; ++x, ++abstractContent)
        if (!((*abstractContent >= *jno_syntax.jno_valid_property_name &&
               *abstractContent <= jno_syntax.jno_valid_property_name[1]) ||
              (x && jno_is_unsigned_jnumber(*abstractContent)) || *abstractContent == jno_syntax.jno_valid_property_name[2]))
            return false;
    return len != 0;
}

method inline jbool jno_is_comment_line(const char* c, int len) {
    return len > 0 && !strncmp(c, jno_syntax.jno_commentLine, std::min(2, len));
}

method inline int jno_has_eol(const char* c, int len = std::numeric_limits<int>::max()) {
    int x;
    for (x ^= x; *c && x < len && *c != jno_syntax.jno_eol_segment; ++x, ++c)
        ;
    return x;
}

method inline int jno_skip_comment(const char* c, int len) {
    int x = jno_trim(c, len);
    while (jno_is_comment_line(c + x, len - x)) {
        x += jno_has_eol(c + x, len - x);
        x += jno_trim(c + x, len - x);
    }
    return x;
}

method inline jbool jno_is_space(const char c) {
    const char* left = jno_syntax.jno_trim_segments;
    const char* right = left + sizeof(jno_syntax.jno_trim_segments);
    for (; left < right; ++left)
        if (*left == c) return true;
    return false;
}

method jbool jno_is_array(const char* content, int& endpoint, int contentLength = std::numeric_limits<int>::max()) {
    int x;
    jbool result = false;

    if (*content == *jno_syntax.jno_array_segments) {
        x ^= x;
        ++x;
        x += jno_trim(content + x);

        x += jno_skip_comment(content + x, contentLength - x);
        x += jno_trim(content + x);

        if (jno_has_datatype(content + x) || content[x] == jno_syntax.jno_array_segments[1]) {
            for (; content[x] && x < contentLength; ++x) {
                if (content[x] == jno_syntax.jno_array_segments[0])
                    break;
                else if (content[x] == jno_syntax.jno_array_segments[1]) {
                    endpoint = x;
                    result = true;
                    break;
                }
            }
        }
    }
    return result;
}

method inline int jno_string_to_hash_fast(const char* content, int contentLength) {
    int x = 1, y;
    y ^= y;
    while (*(content) && y++ < contentLength) x *= *content;
    return x;
}

// JNO Object Node

jno_object_node::jno_object_node(jno_object_parser* head, void* handle) {
    this->head = head;
    this->handle = handle;
}

method JNOType jno_object_node::type() { jno_get_type(handle, static_cast<jno_storage*>(this->head->_storage)); }

method jno_object_node* jno_object_node::tree(const jstring& child) { return nullptr; }

method jbool jno_object_node::has_tree() const { throw std::runtime_error("This is method not implemented"); }

method const jstring jno_object_node::name() const { return jstring(static_cast<char*>(handle)); }

method int jno_avail_only(jno_evaluated& eval, const char* source, int length, int depth) {
    int x, y, z;
    JNOType valueType;
    JNOType current_block_type;
    const char* pointer = source;

    // base step
    if (!length) return 0;

    for (x ^= x; x < length;) {
        // has comment line
        y = x += jno_skip_comment(pointer + x, length - x);
        if (depth > 0 && pointer[x] == jno_syntax.jno_array_segments[1]) break;
        x += jno_skip(pointer + x, length - x);

        // check property name
        if (!jno_valid_property_name(pointer + y, x - y)) throw std::bad_exception();

        // property name
        // prototype_node.propertyName.append(jno_source + y, static_cast<size_t>(x - y));

        //  has comment line
        x += jno_skip_comment(pointer + x, length - x);
        x += jno_trim(pointer + x, length - x);  // trim string
        // is block or array
        if (pointer[x] == *jno_syntax.jno_array_segments) {
            if (jno_is_array(pointer + x, y, length - x)) {
                y += x++;
                current_block_type = JNOType::Unknown;
                // prototype_node.flags = Node_ArrayFlag;
                for (; x < y;) {
                    x += jno_skip_comment(pointer + x, y - x);
                    // next index
                    if (pointer[x] == jno_syntax.jno_obstacle)
                        ++x;
                    else {
                        x += z = jno_get_format(pointer + x, nullptr, valueType);

                        // JNO current value Type

                        if (valueType != JNOType::Unknown) {
                            if (current_block_type == JNOType::Unknown) {
                                current_block_type = valueType;
                                switch (current_block_type) {
                                    case JNOType::JNOString:
                                        ++eval.jarrstrings;
                                        break;
                                    case JNOType::JNOBoolean:
                                        ++eval.jarrbools;
                                        break;
                                    case JNOType::JNOReal:
                                        ++eval.jarrreals;
                                        break;
                                    case JNOType::JNONumber:
                                        ++eval.jarrnumbers;
                                        break;
                                }
                            }
                            // convert jno numbers as JNOReal
                            else if (current_block_type != valueType)
                                throw std::runtime_error("Multi type is found.");

                            switch (current_block_type) {
                                case JNOType::JNOString:
                                    eval.jarrstrings_total_bytes += z - 1;
                                    // prototype_node.set_native_memory((void*)jalloc<std::vector<jstring>>());
                                    break;
                                case JNOType::JNOBoolean:
                                    eval.jarrbools_total_bytes += z;
                                    // prototype_node.set_native_memory((void*)jalloc<std::vector<jbool>>());
                                    break;
                                case JNOType::JNOReal:
                                    eval.jarrreals_total_bytes += z;
                                    // prototype_node.set_native_memory((void*)jalloc<std::vector<jreal>>());
                                    break;
                                case JNOType::JNONumber:
                                    eval.jarrnumbers_total_bytes += z;
                                    // prototype_node.set_native_memory((void*)jalloc<std::vector<jnumber>>());
                                    break;
                            }
                        }
                    }
                    x += jno_skip_comment(pointer + x, y - x);
                }

            } else {  // get the next node
                ++x;
                x += jno_avail_only(eval, pointer + x, length - x, depth + 1);
                x += jno_skip_comment(pointer + x, length - x);
            }

            if (pointer[x] != jno_syntax.jno_array_segments[1]) {
                throw std::bad_exception();
            }
            ++x;
        } else {  // get also value
            x += z = jno_get_format(pointer + x, nullptr, valueType);

            switch (valueType) {
                case JNOType::JNOString:
                    ++eval.jstrings;
                    eval.jarrnumbers_total_bytes += z - 1;
                    break;
                case JNOType::JNOBoolean:
                    ++eval.jbools;
                    eval.jarrbools_total_bytes += z;
                    break;
                case JNOType::JNOReal:
                    ++eval.jreals;
                    eval.jarrreals_total_bytes += z;
                    break;
                case JNOType::JNONumber:
                    ++eval.jnumbers;
                    eval.jarrnumbers_total_bytes += z;
                    break;
            }

            // prototype_node.set_native_memory(pointer);
            // pointer = nullptr;
            // prototype_node.flags = Node_ValueFlag | valueType << 2;
        }

        // entry.insert(std::make_pair(y = jno_string_to_hash(prototype_node.propertyName.c_str()), prototype_node));

        /*
        #if defined(QDEBUG) || defined(DEBUG)
                // get iter
                auto _curIter = entry.find(j);
                if (_dbgLastNode) _dbgLastNode->nextNode = &_curIter->second;
                _curIter->second.prevNode = _dbgLastNode;
                _dbgLastNode = &_curIter->second;
        #endif
        */
        x += jno_skip_comment(pointer + x, length - x);
    }

    return x;
}

// big algorithm, please free me.
// divide and conquer method avail
method int jno_avail(jstruct& entry, jno_evaluated& eval, const char* source, int length, int depth) {
    int x, y;
    JNOType valueType;
    JNOType current_block_type;
    struct property_node {
        jstring propertyName;
        void* data;
    };

    const char* pointer = source;

    // base step
    if (!length) return 0;

    for (x ^= x; x < length;) {
        // has comment line
        y = x += jno_skip_comment(pointer + x, length - x);
        if (depth > 0 && pointer[x] == jno_syntax.jno_array_segments[1]) break;
        x += jno_skip(pointer + x, length - x);
        // check property name
        if (!jno_valid_property_name(pointer + y, x - y)) throw std::bad_exception();
        prototype_node = {};
        prototype_node.propertyName.append(pointer + y, static_cast<size_t>(x - y)); // set property name
        // has comment line
        x += jno_skip_comment(pointer + x, length - x);
        x += jno_trim(pointer + x, length - x);  // trim string
        // is block or array
        if (pointer[x] == *jno_syntax.jno_array_segments) {
            if (jno_is_array(pointer + x, y, length - x)) {
                y += x++;
                current_block_type = JNOType::Unknown;
                // prototype_node.flags = Node_ArrayFlag;
                for (; x < y;) {
                    x += jno_skip_comment(pointer + x, y - x);
                    // next index
                    if (pointer[x] == jno_syntax.jno_obstacle)
                        ++x;
                    else {
                        void* memory;
                        x += jno_get_format(pointer + x, &memory, valueType);
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
                    x += jno_skip_comment(pointer + x, y - x);
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

                prototype_node.flags |= (current_block_type) << 2;
            } else {  // get the next node
                jstruct* _nodes = jalloc<jstruct>();
                ++x;
                x += jno_avail(*_nodes, eval, pointer + x, length - x, depth + 1);
                // prototype_node.flags = Node_StructFlag;
                prototype_node.set_native_memory(_nodes);
                x += jno_skip_comment(pointer + x, length - x);
            }
            if (pointer[x] != jno_syntax.jno_array_segments[1]) {
                throw std::bad_exception();
            }
            ++x;
        } else {  // get also value
            x += jno_get_format(pointer + x, &memory, valueType);
            prototype_node.set_native_memory(memory);
            memory = nullptr;
            // prototype_node.flags = Node_ValueFlag | valueType << 2;
        }

        entry.insert(std::make_pair(y = jno_string_to_hash_fast(prototype_node.propertyName.c_str()), prototype_node));
        /*
        #if defined(QDEBUG) || defined(DEBUG)
                // get iter
                auto _curIter = entry.find(j);
                if ur/target/direct(_dbgLastNode) _dbgLastNode->nextNode = &_curIter->second;
                _curIter->second.prevNode = _dbgLastNode;
ur/target/direct                _dbgLastNode = &_curIter->second;
        #endif
            */
        x += jno_skip_comment(pointer + x, length - x);
    }

    return x;
}

method void jno_object_parser::deserialize_from(const char* filename) {
    long length;
    char* buffer;
    std::ifstream file;

    // try open file
    file.open(filename);

    // has error from open
    if (!file) throw std::runtime_error("error open file");

    // read length
    length = file.seekg(0, std::ios::end).tellg();
    file.seekg(0, std::ios::beg);

    // check buffer
    if ((buffer = (char*)malloc(length)) == nullptr) throw std::bad_alloc();
    // set buffer to zero
    std::memset(buffer, 0, length);
    // read
    length = file.read(buffer, length).gcount();
    // close file
    file.close();
    // deserialize
    deserialize((char*)buffer, length);
    // free buffer
    free(buffer);
}
method void jno_object_parser::deserialize(const jstring& source) { deserialize(source.data(), source.size()); }
method void jno_object_parser::deserialize(const char* source, int len) {
    // FIXME: CLEAR FUNCTION IS UPGRADE
    jno_evaluated eval = {};
    entry.clear();  // clears alls
    jno_avail_only(eval, source, len, 0);
}
method jstring jno_object_parser::serialize(JNOSerializeFormat format) {
    jstring data;
    throw std::runtime_error("no complete");

    if (format == JNOBeautify) {
        data += "//@Just Node Object Version: 1.0.0\n";
    }

    jstruct* entry;

    // Write structure block

    return data;
}
method jno_object_node* jno_object_parser::find_node(const jstring& name) {
    jno_object_node* node;
    int hash = jno_string_to_hash_fast(name.c_str());

    auto iter = this->entry.find(hash);

    if (iter != std::end(this->entry))
        node = &iter->second;
    else
        node = nullptr;
    return node;
}

method jno_object_node* jno_object_parser::tree(const jstring& nodename) { return at(nodename); }

method jstruct& jno_object_parser::get_struct() { return entry; }

method jno_object_node* jno_object_parser::at(const jstring& nodePath) {
    jno_object_node* node = nullptr;
    std::add_pointer<decltype(this->entry)>::type entry = &this->entry;
    decltype(entry->begin()) iter;
    int alpha, delta;
    int hash;

    // set l to 0 (zero value).
    // NOTE: Alternative answer for new method.
    alpha ^= alpha;

    // get splits
    do {
        if ((delta = nodePath.find(jno_syntax.jno_nodePathBreaker, alpha)) == ~0) delta = static_cast<int>(nodePath.length());
        hash = jno_string_to_hash_fast(nodePath.c_str() + alpha, delta - alpha);
        iter = entry->find(hash);
        if (iter != end(*entry)) {
            if (iter->second.has_tree())
                entry = nullptr;  // decltype(entry)(iter->second._bits);
            else {
                if (delta == nodePath.length()) node = &iter->second;  // get the next section
                break;
            }
        }
        alpha = ++delta;
        delta = nodePath.length();

    } while (alpha < delta);
    return node;
}

method jbool jno_object_parser::contains(const jstring& nodePath) { return find_node(nodePath) != nullptr; }

method jno_object_node& operator<<(jno_object_node& root, const jstring& nodename) { return *root.tree(nodename); }
method jno_object_node& operator<<(jno_object_parser& root, const jstring& nodename) { return *root.tree(nodename); }

method std::ostream& operator<<(std::ostream& out, const jno_object_node& node) {
    switch (node.type()) {
        case JNOType::JNONumber:
            out << node.value<jnumber>();
            break;
        case JNOType::JNOBoolean:
            out << node.value<jbool>();
            break;
        case JNOType::JNOReal:
            out << node.value<jreal>();
            break;
        case JNOType::JNOString:
            out << node.value<jstring>();
            break;
        case JNOType::Unknown:
            out << jno_syntax.jno_unknown_string;
            break;
        case JNOType::Null:
            out << jno_syntax.jno_null_string;
            break;
    }

    return out;
}

method std::ostream& operator<<(std::ostream& out, const jno_object_parser& parser) {
    if (false)
        out << jno_syntax.jno_unknown_string;
    else
        out << parser.serialize();
    return out;
}

}  // namespace jno

#undef method
#undef MACROPUT
