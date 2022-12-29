#include <cstdlib>
#include <vector>
#include <tuple>
#include <climits>

// import header
#include "justparser"

#if __unix__ || __linux__
#include <unistd.h>
#elif WIN32
#include <windows.h>
#endif

#define method

#define MACROPUT(base) #base

namespace just {

    typedef std::vector<int> jtree_t;

    static const struct {
        char just_dot = '.';
        char just_obstacle = ',';
        char just_nodePathBreaker = '/';
        char just_commentLine[3] = "//";
        char just_left_seperator = '\\';
        char just_eol_segment = '\n';
        char just_format_string = '\"';
        char just_true_string[5] = MACROPUT(true);
        char just_false_string[6] = MACROPUT(false);
        char just_null_string[5] = MACROPUT(null);
        char just_unknown_string[6] { "{...}" };
        char just_block_segments[2] { '{', '}' };
        char just_trim_segments[5] { 32, '\t', '\n', '\r', '\v' };
        char just_valid_property_name[3] = { 'A', 'z', '_' };
    } just_syntax;

    // TODO: Get status messages
    // static const struct { const char* msg_multitype_cast = ""; } just_error_msg;

    enum { Node_ValueFlag = 1,
        Node_ArrayFlag = 2,
        Node_StructFlag = 3 };

    template <typename T>
    struct get_type {
        static constexpr JustType type = JustType::Unknown;
    };

    template <>
    struct get_type<void> {
        static constexpr JustType type = JustType::Null;
    };

    template <>
    struct get_type<int> {
        static constexpr JustType type = JustType::JustNumber;
    };

    template <>
    struct get_type<float> {
        static constexpr JustType type = JustType::JustReal;
    };

    template <>
    struct get_type<double> {
        static constexpr JustType type = JustType::JustReal;
    };

    template <>
    struct get_type<bool> {
        static constexpr JustType type = JustType::JustBoolean;
    };

    template <>
    struct get_type<jstring> {
        static constexpr JustType type = JustType::JustString;
    };

    template <>
    struct get_type<const char*> {
        static constexpr JustType type = JustType::JustString;
    };

    method inline int just_type_size(const JustType type)
    {
        switch (type) {
        case JustType::JustBoolean:
            return sizeof(jbool);
        case JustType::JustNumber:
            return sizeof(jnumber);
        case JustType::JustReal:
            return sizeof(jreal);
        }
        return 0;
    }
    // NOTE: storage description
    /*
        index  |types
        -------|----------------
        0       bools
        1       numbers
        2       reals
        3       strings
        4       properties

        ------------------------
        ISSUE:
        - Organize structure malloc (realloc)
        - Tree for node
            - How to get answer ? First use unsigned int as pointer in linear.

    */

    struct just_storage {
        // up members  : meta-info
        // down members: size-info
        std::uint8_t just_allocated;
        jnumber numBools, numNumbers, numReals, numStrings, arrayBools, arrayNumbers, arrayReals, arrayStrings, numProperties;
    };

    struct just_evaluated {
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

        const jnumber calcBytes() const
        {
            jnumber sz;

            // set "sz" to zero
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

    method inline int get_page_size()
    {
        int _PAGE_SIZE;
#if __unix__ || __linux__
        _PAGE_SIZE = sysconf(_SC_PAGE_SIZE);
#elif WIN32
        SYSTEM_INFO sysInfo;
        GetSystemInfo(&sysInfo);
        _PAGE_SIZE = static_cast<int>(sysInfo.dwPageSize);
#else
        _PAGE_SIZE = 4096;
#endif
        return _PAGE_SIZE;
    }

    // method for create and init new storage.
    method inline just_storage* storage_new_init()
    {
        auto pgSize = get_page_size();
        pgSize = sizeof(just_storage) > pgSize ? sizeof(just_storage) : pgSize;
        void* ptr = std::malloc(pgSize);
        if (!ptr)
            throw std::bad_alloc();
        return static_cast<just_storage*>(std::memset(ptr, 0, pgSize));
    }

    method std::uint32_t storage_calc_size(const just_storage* pstorage, JustType type)
    {
        std::uint32_t calcSize;

        if (!pstorage)
            throw std::runtime_error("Memory is undefined");

        if (type > JustType::Null) {
            // move pointer to ...
            const jnumber* alpha = reinterpret_cast<const jnumber*>(static_cast<const void*>(pstorage) + sizeof(pstorage->just_allocated)) + static_cast<int>(type);
            calcSize = (*alpha) >> 32;
        } else {
            // set to zero
            calcSize ^= calcSize;
        }

        return calcSize;
    }

    method jvariant storage_alloc_get(just_storage** pstore, JustType type, int allocSize = Null)
    {
#define store (*pstore)
#define _addNumber(plot) ()

        if (pstore == nullptr || *pstore == nullptr)
            throw std::bad_alloc();

        if ((*pstore)->just_allocated) {
            throw std::runtime_error("storage state an optimized");
        }

        // Storage Meta-info
        // Low bytes: count
        // High bytes: size
        // Check
        auto calc = storage_calc_size(store, type);

        void* _vault = store + 1;
        void* newAllocated;
        int vaultSize;
        if (allocSize <= 0)
            allocSize = just_type_size(type);

        newAllocated = std::realloc(_vault, allocSize);

        if (!newAllocated)
            return newAllocated; // bad alloc
        _vault = newAllocated;
        switch (type) {
        case JustType::JustBoolean:
            _vault = std::realloc(_vault, allocSize);
            ++store->numBools;
        case JustType::JustNumber:
            ++store->numNumbers;
        case JustType::JustReal:
            ++store->numReals;
        case JustType::JustString:
            ++store->numStrings;
        case JustType::Unknown:
        case JustType::Null:
            throw std::bad_cast();
        }
#undef store
    }

    method jtree_t* storage_alloc_tree(just_storage** pstore)
    {
        if (pstore == nullptr || *pstore == nullptr)
            throw std::bad_alloc();

        if ((*pstore)->just_allocated) {
            throw std::runtime_error("storage state an optimized");
        }
        return nullptr;
    }

    method jvariant storage_alloc_array(just_storage** pstore, JustType arrayType)
    {
        jvariant variant;

        return variant;
    }

    method bool storage_optimize(just_storage** pstorage)
    {
        if(pstorage->
        if ((*pstorage)->just_allocated)
            return true;

        // TODO: optimize here
    }

    // method for get type from pointer (storage required)
    method JustType just_get_type(const void* pointer, just_storage* pstorage)
    {
        const jnumber* alpha = static_cast<jnumber*>(static_cast<void*>(pstorage) + sizeof(pstorage->just_allocated));
        const void* delta = pstorage + 1;
        int type;
        if (pointer) {
            type = JustType::Unknown;
            for (; alpha < delta; ++alpha) {
                // get type
                ++type;
                // set next pointer
                delta += static_cast<std::uint32_t>(*alpha >> 32); // high (bytes)
            }
        } else
            // ops: Type is null, var is empty
            type = JustType::Null;

        return static_cast<JustType>(type);
    }

    // method for fast get hash from string
    method inline int just_string_to_hash_fast(const char* content, int contentLength = INT32_MAX)
    {
        int x, y;
        y = (x = x == x) ^ x; // init a x, y variable
        while (*(content) && y++ < contentLength)
            x *= *content;
        return x;
    }

    // method for check valid a unsigned number
    method inline bool just_is_unsigned_jnumber(const char character) { return std::isdigit(character); }

    // method for check valid a signed number
    method inline bool just_is_jnumber(const char character) { return just_is_unsigned_jnumber(character) || character == '-'; }

    // method for check is real ?
    method jbool just_is_jreal(const char* content, int* getLength)
    {
        bool real = false;
        for (;;) {
            if (!just_is_jnumber(content[*getLength])) {
                if (real)
                    break;

                real = (content[*getLength] == just_syntax.just_dot);

                if (!real)
                    break;
            }
            ++*getLength;
        }
        return real;
    }

    // method for check is bool ?
    method inline jbool just_is_jbool(const char* content, int* getLength)
    {
        if (!strncmp(content, just_syntax.just_true_string, *getLength = sizeof(just_syntax.just_true_string) - 1))
            return true;
        if (!strncmp(content, just_syntax.just_false_string, *getLength = sizeof(just_syntax.just_false_string) - 1))
            return true;
        *getLength ^= *getLength;
        return false;
    }

    // method for get format from raw content, also to write in storage pointer
    method int just_get_format(const char* content, just_storage** storage, JustType& containType, jvariant* outValue = nullptr)
    {
        int x;
        int offset;
        void* mem;

        offset ^= offset; // set to zero

        // Null type
        if (content == nullptr || *content == '\0') {
            containType = JustType::Null;
        } else if (just_is_jbool(content, &offset)) { // Bool type -----------------------------------------------------------------------------
            containType = JustType::JustBoolean;
            if (storage) {
                jbool conv = (offset == sizeof(just_syntax.just_true_string) - 1);
                // Copy to
                mem = std::memcpy(storage_alloc_get(storage, containType), &conv, just_type_size(containType));
                if (outValue)
                    *outValue = mem;
            }

        } else if (just_is_jnumber(*content)) { // Number type ---------------------------------------------------------------------------------
            containType = JustType::JustNumber;
            if (storage) {
                jnumber conv = std::atoll(content);
                // Copy to
                mem = std::memcpy(storage_alloc_get(storage, containType), &conv, just_type_size(containType));
                if (outValue)
                    *outValue = mem;
            }
        } else if (just_is_jreal(content, &offset)) { // Real type -----------------------------------------------------------------------------
            containType = JustType::JustReal;
            if (storage) {
                jreal conv = std::atof(content);
                // Copy to
                mem = std::memcpy(storage_alloc_get(storage, containType), &conv, just_type_size(containType));
                if (outValue)
                    *outValue = mem;
            }
        } else if (*content == just_syntax.just_format_string) { // String type ----------------------------------------------------------------

            ++offset;
            for (; content[offset] != just_syntax.just_format_string; ++offset)
                if (content[offset] == just_syntax.just_left_seperator && content[offset + 1] == just_syntax.just_format_string)
                    ++offset;
            containType = JustType::JustString;
            if (--offset) {
                if (storage) {
                    // TODO: Go support next for string. '\n' '\r' .etc.
                    jstring stringSyntax;
                    stringSyntax.reserve(offset);
                    for (x ^= x; x < offset; ++x) {
                        if (content[x + 1] == just_syntax.just_left_seperator)
                            ++x;
                        stringSyntax.push_back(content[x + 1]);
                    }

                    // Copy to
                    mem = std::memcpy(storage_alloc_get(storage, containType, stringSyntax.size()), stringSyntax.data(), stringSyntax.size());
                    if (outValue)
                        *outValue = mem;
                }
            }
            offset += 2;
        } else // another type
            containType = JustType::Unknown;

        return offset;
    }

    // method for check. has type in value
    method inline jbool just_has_datatype(const char* content)
    {
        JustType type;
        just_get_format(const_cast<char*>(content), nullptr, type);
        return type != JustType::Unknown && type < JustType::Null;
    }

    // method for trim tabs, space, EOL (etc) to skip.
    method int just_trim(const char* content, int contentLength = INT_MAX)
    {
        int x, y;
        x ^= x;
        if (content != nullptr)
            for (; x < contentLength && *content;) {
                for (y ^= y; y < static_cast<int>(sizeof(just_syntax.just_trim_segments));)
                    if (*content == just_syntax.just_trim_segments[y])
                        break;
                    else
                        ++y;
                if (y != sizeof(just_syntax.just_trim_segments)) {
                    ++x;
                    ++content;
                } else
                    break;
            }
        return x;
    }

    method int just_skip(const char* c, int len = INT_MAX)
    {
        int x, y;
        char skipping[sizeof(just_syntax.just_trim_segments) + sizeof(just_syntax.just_block_segments)];
        *skipping ^= *skipping;
        strncpy(skipping, just_syntax.just_trim_segments, sizeof(just_syntax.just_trim_segments));
        strncpy(skipping + sizeof just_syntax.just_trim_segments, just_syntax.just_block_segments, sizeof(just_syntax.just_block_segments));
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

    method inline jbool just_valid_property_name(const char* abstractContent, int len)
    {
        int x;
        for (x ^= x; x < len; ++x, ++abstractContent)
            if (!((*abstractContent >= *just_syntax.just_valid_property_name && *abstractContent <= just_syntax.just_valid_property_name[1]) || (x && just_is_unsigned_jnumber(*abstractContent)) || *abstractContent == just_syntax.just_valid_property_name[2]))
                return false;
        return len != 0;
    }

    method inline jbool just_is_comment_line(const char* c, int len) { return len > 0 && !std::strncmp(c, just_syntax.just_commentLine, std::min(2, len)); }

    method inline int just_has_eol(const char* pointer, int len = INT_MAX)
    {
        const char* chars = pointer;
        const char* endChars = pointer + len;
        while (*chars && chars < endChars && *chars != just_syntax.just_eol_segment)
            ++chars;
        return static_cast<int>(chars - pointer);
    }

    // method for skip comment's
    method int just_autoskip_comment(const char* src, int len)
    {
        int offset, skipped;
        const char* pointer = src;
        pointer += offset = just_trim(pointer, len - offset);
        while (just_is_comment_line(pointer, len - offset)) {
            // skip to EOL
            pointer += skipped = just_has_eol(pointer, len - offset);
            offset += skipped;
            // trimming
            pointer += skipped = just_trim(pointer, len - offset);
            offset += skipped;
        }
        return offset;
    }

    method inline jbool just_is_space(const char c)
    {
        const char* left = just_syntax.just_trim_segments;
        const char* right = left + sizeof(just_syntax.just_trim_segments);
        for (; left < right; ++left)
            if (*left == c)
                return true;
        return false;
    }

    method jbool just_is_array(const char* content, int& endpoint, int contentLength = INT_MAX)
    {
        int x;
        jbool result = false;

        if (*content == *just_syntax.just_block_segments) {
            x = x == x;
            x += just_trim(content + x);

            x += just_autoskip_comment(content + x, contentLength - x);
            x += just_trim(content + x);

            if (just_has_datatype(content + x) || content[x] == just_syntax.just_block_segments[1]) {
                for (; content[x] && x < contentLength; ++x) {
                    if (content[x] == just_syntax.just_block_segments[0])
                        break;
                    else if (content[x] == just_syntax.just_block_segments[1]) {
                        endpoint = x;
                        result = true;
                        break;
                    }
                }
            }
        }
        return result;
    }

    // Just Object Node

    just_object_node::just_object_node(just_object_parser* owner, void* handle)
    {
        this->owner = owner;
        this->var = handle;
    }

    method JustType just_object_node::type() const { just_get_type(var, static_cast<just_storage*>(this->owner->_storage)); }

    method just_object_node* just_object_node::tree(const jstring& child) { return nullptr; }

    method jbool just_object_node::has_tree() const { throw std::runtime_error("This is method not implemented"); }

    method const jstring just_object_node::name() const { return jstring(static_cast<char*>(var)); }

    method int just_avail_only(just_evaluated& eval, const char* source, int length, int depth)
    {
        int x, y, z;
        JustType valueType;
        JustType current_block_type;
        const char* pointer = source;

        // base step
        if (!length)
            return 0;

        for (x ^= x; x < length;) {
            // has comment line
            y = x += just_autoskip_comment(pointer + x, length - x);
            if (depth > 0 && pointer[x] == just_syntax.just_block_segments[1])
                break;
            x += just_skip(pointer + x, length - x);

            // check property name
            if (!just_valid_property_name(pointer + y, x - y))
                throw std::bad_exception();

            // property name
            // prototype_node.propertyName.append(just_source + y, static_cast<size_t>(x - y));

            //  has comment line
            x += just_autoskip_comment(pointer + x, length - x);
            x += just_trim(pointer + x, length - x); // trim string
            // is block or array
            if (pointer[x] == *just_syntax.just_block_segments) {
                if (just_is_array(pointer + x, y, length - x)) {
                    y += x++;
                    current_block_type = JustType::Unknown;
                    // prototype_node.flags = Node_ArrayFlag;
                    for (; x < y;) {
                        x += just_autoskip_comment(pointer + x, y - x);
                        // next index
                        if (pointer[x] == just_syntax.just_obstacle)
                            ++x;
                        else {
                            x += z = just_get_format(pointer + x, nullptr, valueType);

                            // Just Node Object current value Type

                            if (valueType != JustType::Unknown) {
                                if (current_block_type == JustType::Unknown) {
                                    current_block_type = valueType;
                                    switch (current_block_type) {
                                    case JustType::JustString:
                                        ++eval.jarrstrings;
                                        break;
                                    case JustType::JustBoolean:
                                        ++eval.jarrbools;
                                        break;
                                    case JustType::JustReal:
                                        ++eval.jarrreals;
                                        break;
                                    case JustType::JustNumber:
                                        ++eval.jarrnumbers;
                                        break;
                                    }
                                }
                                // convert just numbers as JustReal
                                else if (current_block_type != valueType)
                                    throw std::runtime_error("Multi type is found.");

                                switch (current_block_type) {
                                case JustType::JustString:
                                    eval.jarrstrings_total_bytes += z - 1;
                                    // prototype_node.set_native_memory((void*)jalloc<std::vector<jstring>>());
                                    break;
                                case JustType::JustBoolean:
                                    eval.jarrbools_total_bytes += z;
                                    // prototype_node.set_native_memory((void*)jalloc<std::vector<jbool>>());
                                    break;
                                case JustType::JustReal:
                                    eval.jarrreals_total_bytes += z;
                                    // prototype_node.set_native_memory((void*)jalloc<std::vector<jreal>>());
                                    break;
                                case JustType::JustNumber:
                                    eval.jarrnumbers_total_bytes += z;
                                    // prototype_node.set_native_memory((void*)jalloc<std::vector<jnumber>>());
                                    break;
                                }
                            }
                        }
                        x += just_autoskip_comment(pointer + x, y - x);
                    }

                } else { // get the next node
                    ++x;
                    x += just_avail_only(eval, pointer + x, length - x, depth + 1);
                    x += just_autoskip_comment(pointer + x, length - x);
                }

                if (pointer[x] != just_syntax.just_block_segments[1]) {
                    throw std::bad_exception();
                }
                ++x;
            } else { // get also value
                x += z = just_get_format(pointer + x, nullptr, valueType);

                switch (valueType) {
                case JustType::JustString:
                    ++eval.jstrings;
                    eval.jarrnumbers_total_bytes += z - 1;
                    break;
                case JustType::JustBoolean:
                    ++eval.jbools;
                    eval.jarrbools_total_bytes += z;
                    break;
                case JustType::JustReal:
                    ++eval.jreals;
                    eval.jarrreals_total_bytes += z;
                    break;
                case JustType::JustNumber:
                    ++eval.jnumbers;
                    eval.jarrnumbers_total_bytes += z;
                    break;
                }

                // prototype_node.set_native_memory(pointer);
                // pointer = nullptr;
                // prototype_node.flags = Node_ValueFlag | valueType << 2;
            }

            // entry.insert(std::make_pair(y = just_string_to_hash(prototype_node.propertyName.c_str()), prototype_node));

            /*
            #if defined(QDEBUG) || defined(DEBUG)
                    // get iter
                    auto _curIter = entry.find(j);
                    if (_dbgLastNode) _dbgLastNode->nextNode = &_curIter->second;
                    _curIter->second.prevNode = _dbgLastNode;
                    _dbgLastNode = &_curIter->second;
            #endif
            */
            x += just_autoskip_comment(pointer + x, length - x);
        }

        return x;
    }

    // big algorithm, please free me.
    // divide and conquer method avail
    method int just_avail(just_storage** storage, just_evaluated& eval, const char* source, int length, int depth)
    {
        int x, y;
        const char* pointer;
        JustType valueType;
        JustType current_block_type;

        std::vector<jtree_t*> _stacks;

        _stacks.emplace_back(storage_alloc_tree(storage)); // push head tree

#define prototype_node (_stacks.back()) // set a macro name
#define push(object) (_stacks.push_back(object))
#define pop() (_stacks.pop_back())

        pointer = source;

        for (x ^= x; x < length;) {
            // skip and trimming
            y = x += just_autoskip_comment(pointer + x, length - x);
            if (depth > 0 && pointer[x] == just_syntax.just_block_segments[1])
                break;
            x += just_skip(pointer + x, length - x);

            // Preparing, check property name
            if (!just_valid_property_name(pointer + y, x - y))
                throw std::bad_exception();

            prototype_node.propertyName.append(pointer + y, static_cast<size_t>(x - y)); // set property name

            // has comment line
            x += just_autoskip_comment(pointer + x, length - x);
            x += just_trim(pointer + x, length - x); // trim string
            // is block or array
            if (pointer[x] == *just_syntax.just_block_segments) {
                if (just_is_array(pointer + x, y, length - x)) {
                    y += x++;
                    current_block_type = JustType::Unknown;
                    // prototype_node.flags = Node_ArrayFlag;

                    // While end of array length
                    while (x < y) {
                        x += just_autoskip_comment(pointer + x, y - x);
                        // next index
                        if (pointer[x] == just_syntax.just_obstacle)
                            ++x;
                        else {
                            jvariant pvalue;
                            x += just_get_format(pointer + x, storage, valueType, &pvalue);
                            if (valueType != JustType::Unknown) {
                                if (current_block_type == JustType::Unknown) {
                                    current_block_type = valueType;

                                    switch (current_block_type) {
                                    case JustType::JustBoolean:
                                        // prototype_node.set_native_memory((void*)jalloc<std::vector<jbool>>());
                                        break;
                                    case JustType::JustNumber:
                                        // prototype_node.set_native_memory((void*)jalloc<std::vector<jnumber>>());
                                        break;
                                    case JustType::JustReal:
                                        // prototype_node.set_native_memory((void*)jalloc<std::vector<jreal>>());
                                        break;
                                    case JustType::JustString:
                                        // prototype_node.set_native_memory((void*)jalloc<std::vector<jstring>>());
                                        break;
                                    }

                                } else if (current_block_type != valueType && current_block_type == JustType::JustReal && valueType != JustType::JustNumber) {
                                    throw std::runtime_error("Multi type is found.");
                                }

                                switch (current_block_type) {
                                case JustType::JustString: {
                                    /*auto ref = (jstring*)memory;
                                    ((std::vector<jstring>*)prototype_node.handle)->emplace_back(*ref);
                                    jfree(ref);*/
                                    break;
                                }
                                case JustType::JustBoolean: {
                                    //                                    auto ref = (jbool*)memory;
                                    //                                    ((std::vector<jbool>*)prototype_node.handle)->emplace_back(*ref);
                                    //                                    jfree(ref);
                                    break;
                                }
                                case JustType::JustReal: {
                                    //                                    jreal* ref;
                                    //                                    if (valueType == JustType::JustNumber) {
                                    //                                        ref = new jreal(static_cast<jreal>(*(jnumber*)memory));
                                    //                                        jfree((jnumber*)pointer);
                                    //                                    } else
                                    //                                        ref = (jreal*)memory;

                                    //                                    ((std::vector<jreal>*)prototype_node.handle)->emplace_back(*ref);
                                    //                                    jfree(ref);
                                    break;
                                }
                                case JustType::JustNumber: {
                                    //                                    auto ref = (jnumber*)memory;
                                    //                                    ((std::vector<jnumber>*)prototype_node.handle)->emplace_back(*ref);
                                    //                                    jfree(ref);
                                    break;
                                }
                                }
                            }
                        }
                        x += just_autoskip_comment(pointer + x, y - x);
                    }

                    prototype_node.flags |= (current_block_type) << 2;
                } else { // get the next node
                    auto _tree = storage_alloc_tree(storage);

                    ++x;
                    x += just_avail(storage, eval, pointer + x, length - x, depth + 1);
                    // prototype_node.flags = Node_StructFlag;
                    prototype_node.set_native_memory(_nodes);
                    x += just_autoskip_comment(pointer + x, length - x);
                }
                if (pointer[x] != just_syntax.just_block_segments[1]) {
                    throw std::bad_exception();
                }
                ++x;
            } else { // get also value
                x += just_get_format(pointer + x, storage, valueType);

                //prototype set memory pointer
                prototype_node.set_native_memory(memory);

                memory = nullptr;
                // prototype_node.flags = Node_ValueFlag | valueType << 2;
            }

            entry.insert(std::make_pair(y = just_string_to_hash_fast(prototype_node.propertyName.c_str()), prototype_node));

            x += just_autoskip_comment(pointer + x, length - x);
        }

#undef prototype_node // undefine a macro name
#undef push
#undef pop

        return x;
    }

    method void just_object_parser::deserialize_from(const char* filename)
    {
        long length;
        char* buffer;
        std::ifstream file;

        // try open file
        file.open(filename);

        // has error from open
        if (!file)
            throw std::runtime_error("error open file");

        // read length
        length = file.seekg(0, std::ios::end).tellg();
        file.seekg(0, std::ios::beg);

        // check buffer
        if ((buffer = (char*)malloc(length)) == nullptr)
            throw std::bad_alloc();
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
    method void just_object_parser::deserialize(const jstring& source) { deserialize(source.data(), source.size()); }

    method void just_object_parser::deserialize(const char* source, int len)
    {
        // FIXME: CLEAR FUNCTION IS UPGRADE
        just_evaluated eval = {};
        entry.clear(); // clears alls

        if (_storage) {
            std::free(_storage);
        }

        // init storage
        _storage = storage_new_init();
        just_avail_only(eval, source, len, 0);
    }
    method jstring just_object_parser::serialize(JustSerializeFormat format) const
    {
        jstring data;
        throw std::runtime_error("no complete");

        if (format == JustBeautify) {
            data += "//@Just Node Object Version: 1.0.0\n";
        }

        // jstruct* entry;

        // Write structure block

        // TODO: Go here write (serialize)

        return data;
    }
    method just_object_node* just_object_parser::find_node(const jstring& name)
    {
        just_object_node* node;
        int hash = just_string_to_hash_fast(name.c_str());

        auto iter = this->entry.find(hash);

        if (iter != std::end(this->entry))
            node = &iter->second;
        else
            node = nullptr;
        return node;
    }

    method just_object_node* just_object_parser::tree(const jstring& nodename) { return at(nodename); }

    method just_object_node* just_object_parser::at(const jstring& nodePath)
    {
        just_object_node* node = nullptr;
        std::add_pointer<decltype(this->entry)>::type entry = &this->entry;
        decltype(entry->begin()) iter;
        int alpha, beta;
        int hash;

        // set l to 0 (zero value).
        // NOTE: Alternative answer for new method.
        alpha ^= alpha;

        // get splits
        do {
            if ((beta = nodePath.find(just_syntax.just_nodePathBreaker, alpha)) == ~0)
                beta = static_cast<int>(nodePath.length());
            hash = just_string_to_hash_fast(nodePath.c_str() + alpha, beta - alpha);
            iter = entry->find(hash);
            if (iter != end(*entry)) {
                if (iter->second.has_tree())
                    entry = nullptr; // decltype(entry)(iter->second._bits);
                else {
                    if (beta == nodePath.length())
                        node = &iter->second; // get the next section
                    break;
                }
            }
            alpha = ++beta;
            beta = nodePath.length();

        } while (alpha < beta);
        return node;
    }

    method jbool just_object_parser::contains(const jstring& nodePath) { return find_node(nodePath) != nullptr; }

    method just_object_node& operator<<(just_object_node& root, const jstring& nodename) { return *root.tree(nodename); }

    method just_object_node& operator<<(just_object_parser& root, const jstring& nodename) { return *root.tree(nodename); }

    method std::ostream& operator<<(std::ostream& out, const just_object_node& node)
    {
        switch (node.type()) {
        case JustType::JustNumber:
            out << node.value<jnumber>();
            break;
        case JustType::JustBoolean:
            out << node.value<jbool>();
            break;
        case JustType::JustReal:
            out << node.value<jreal>();
            break;
        case JustType::JustString:
            out << node.value<jstring>();
            break;
        case JustType::Unknown:
            out << just_syntax.just_unknown_string;
            break;
        case JustType::Null:
            out << just_syntax.just_null_string;
            break;
        }

        return out;
    }

    method std::ostream& operator<<(std::ostream& out, const just_object_parser& parser)
    {
        if (false)
            out << just_syntax.just_unknown_string;
        else
            out << parser.serialize();
        return out;
    }

} // namespace just

#undef method
#undef MACROPUT
