#pragma once

#include <stdio>
#include <memory>
#include <map>
#include <string>
#include <stdint.h>


namespace JNOParser {
	static const struct {
		char dot = '.';
		char obstacle = ',';
		char space = ' ';
		char nodePathBreaker = '/';
		char commentLine[3] = "//";
		char eof_segment = '\n';
		char format_string = '\"';
		char true_string[5]{ "true" };
		char false_string[6]{ "false" };
		char array_segments[2]{ '{', '}' };
		char trim_segments[5]{ ' ', '\t', '\n', '\r' , '\v' };
	} syntax;

	enum ObjectSerializedFormat {
		Normal,
		WithOut_EOF,
		WithOut_Tabs
	};

	constexpr int Node_ValueFlag = 1;
	constexpr int Node_ArrayFlag = 2;
	constexpr int Node_StructFlag = 3;

	class ObjectParser;

	enum DataType
	{
		Unknown = 0,
		String = 1,
		Boolean = 2,
		Real = 3,
		Number = 4
	};

	class ObjectNode {
		friend class ObjectParser;
		std::string propertyName;
		std::uint8_t valueFlag;
		void* value = 0;
		mutable int* uses = NULL;

		void*& setMemory(void* mem);
		int decrementMemory();
		int incrementMemory();

#ifdef _DEBUG
		ObjectNode* prevNode = NULL;
		ObjectNode* nextNode = NULL;
#endif

	public:
		ObjectNode() = default;
		ObjectNode(const ObjectNode&);

		ObjectNode& operator=(const ObjectNode&);

		~ObjectNode();

		inline bool	isArray();
		inline bool	isStruct();
		inline bool	isValue();
		inline bool	isNumber();
		inline bool	isReal();
		inline bool	isString();
		inline bool	isBoolean();

		//Property name it is Node
		std::string& getPropertyName();

		std::int64_t& toNumber();
		double& toReal();
		std::string& toString();
		bool& toBoolean();

		vector<std::int64_t>* toNumbers();
		vector<double>* toReals();
		vector<std::string>* toStrings();
		vector<bool>* toBooleans();

	};

	class ObjectParser
	{
	public:
		using _StructType = std::map<int, ObjectNode>;
	private:
		_StructType entry;
		int avail(ObjectParser::_StructType& entry, const char* source, int len, int levels = 0);


	public:
		ObjectParser();
		ObjectParser(const ObjectParser&) = delete;
		~ObjectParser() = default;

		void Deserialize(const char* filename);
		void Deserialize(const std::string& source);
		void Deserialize(const char* source, int len);
		std::string Serialize();

		//Find node
		ObjectNode* GetNode(const std::string& name);

		_StructType& GetContainer();
		_StructType& GetContainer(ObjectNode* node);

		//Find node from child 
		//example, key="First/Second/Triple" -> Node
		//for has a node, HasNode
		ObjectNode* FindNode(const std::string& nodePath);

		bool ContainsNode(const std::string& nodePath);

		void Clear();
	};

	int stringToHash(const char* str, int len = INT_MAX);

}
