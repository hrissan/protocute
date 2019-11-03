#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/variant/recursive_variant.hpp>
#include <boost/variant.hpp>
#include <boost/optional.hpp>

#include <iostream>
#include <string>
#include <vector>

struct GImport {
	boost::optional<int> kind;
    std::string filename;
};

inline std::ostream& operator<<(std::ostream& stream, const GImport& val) {
    return stream << "GImport{" << (val.kind ? val.kind.get() : 0) << "|" << val.filename << "}";
 }

inline std::ostream& operator<<(std::ostream& stream, const std::vector<std::string> & val) {
	stream << "v<s>{";
	for(const auto & s : val)
		stream << s << "|";
    return stream << "}";
 }

BOOST_FUSION_ADAPT_STRUCT(GImport,
	(boost::optional<int>, kind)
	(std::string, filename)
)

struct GOption {
    std::string option_name;
    std::string constant;
};

inline std::ostream& operator<<(std::ostream& stream, const GOption& val) {
    return stream << "GOption{" << val.option_name << "|" << val.constant << "}";
 }

BOOST_FUSION_ADAPT_STRUCT(GOption,
	(std::string, option_name)
	(std::string, constant)
)

enum class GFieldKind { OPTIONAL, REQUIRED, REPEATED, ONEOF };
// ONEOF is not parsed from file, but assigned in code as a shortcut
// when generating field inside oneof group

struct GField {
	GFieldKind kind = GFieldKind::OPTIONAL;
    std::string type;
    std::string name;
    std::string number;
    std::vector<GOption> options;
    
    bool is_true_optional()const{
		for(const auto & op : options)
			if(op.option_name == "protocute.optional" && op.constant == "true")
				return true;
		return false;
	}
    bool is_packed()const{
		for(const auto & op : options)
			if(op.option_name == "packed" && op.constant == "true")
				return true;
		return false;
	}
//	std::string get_default()const{
//		for(const auto & op : options)
//			if(op.option_name == "default")
//				return op.constant;
//		return std::string{};
//	}
};

inline std::ostream& operator<<(std::ostream& stream, const GField& val) {
    return stream << "GField{" << int(val.kind) << "|" << val.type << "|" << val.name << "|" << val.number << "|" << val.options.size() << "}";
 }

BOOST_FUSION_ADAPT_STRUCT(GField,
	(GFieldKind, kind)
	(std::string, type)
	(std::string, name)
	(std::string, number)
	(std::vector<GOption>, options)
)

struct GEnumField {
	std::string name;
	std::string value;
	std::vector<GOption> options;
};

struct GEmptyStatement {};

struct GEnum {
	typedef boost::variant<GEnumField, GOption> GEnumFieldVariant;
	std::string name;
	std::vector<GEnumFieldVariant> fields;
};

inline std::ostream& operator<<(std::ostream& stream, const GEnumField& val) {
    return stream << "GEnumField{" << val.name << "|" << val.value << "|" << val.options.size() << "}";
 }

inline std::ostream& operator<<(std::ostream& stream, const GEnum& val) {
	stream << "GEnum{" << val.name << "|";
	for(const auto & s : val.fields)
		if(s.type() == typeid(GEnumField))
			stream << boost::get<GEnumField>(s) << "|";
		else if(s.type() == typeid(GOption))
			stream << boost::get<GOption>(s) << "|";
    return stream << "}";
 }

BOOST_FUSION_ADAPT_STRUCT(GEnumField,
	(std::string, name)
	(std::string, value)
	(std::vector<GOption>, options)
)
BOOST_FUSION_ADAPT_STRUCT(GEnum,
	(std::string, name)
	(std::vector<GEnum::GEnumFieldVariant>, fields)
)

struct GOneOfField {
	std::string type;
	std::string name;
	std::string number;
	std::vector<GOption> options;
};


inline std::ostream& operator<<(std::ostream& stream, const GOneOfField& val) {
	return stream << "GOneOfField{" << val.type << "|" << val.name << "|" << val.number << "|" << val.options.size() << "}";
}

BOOST_FUSION_ADAPT_STRUCT(GOneOfField,
	(std::string, type)
	(std::string, name)
	(std::string, number)
	(std::vector<GOption>, options)
)

struct GOneOf {
	std::string name;
	std::vector<GOneOfField> fields;
};

inline std::ostream& operator<<(std::ostream& stream, const GOneOf& val) {
	return stream << "GOneOf{" << val.name << "|" << val.fields.size() << "}";
}

BOOST_FUSION_ADAPT_STRUCT(GOneOf,
	(std::string, name)
	(std::vector<GOneOfField>, fields)
)

struct GMessage;

typedef boost::variant<boost::recursive_wrapper<GMessage>, GField, GEnum, GOption, GOneOf, GEmptyStatement> GMessageFieldVariant;

struct GMessage {
	std::string name;
	std::vector<GMessageFieldVariant> fields;
};

struct PrintVisitor {
	std::ostream& stream;
	explicit PrintVisitor(std::ostream& stream):stream(stream) {}
	void operator()(GField const& s) const
	{
		stream << s;
	}
	void operator()(GEnum const& s) const
	{
		stream << s;
	}
	void operator()(GOption const& s) const
	{
		stream << s;
	}
	void operator()(GOneOf const& s) const
	{
		stream << s;
	}
	void operator()(GMessage const& s) const;
	void operator()(GEmptyStatement const& s) const
	{}
};

inline std::ostream& operator<<(std::ostream& stream, const GMessage& val) {
	stream << "GMessage{" << val.name << "|";
	for(const auto & s : val.fields){
		boost::apply_visitor(PrintVisitor{stream}, s);
 		stream << "|";
	}
    return stream << "}";
 }

inline void PrintVisitor::operator()(GMessage const& s) const
{
	stream << s;
}

BOOST_FUSION_ADAPT_STRUCT(GMessage,
	(std::string, name)
	(std::vector<GMessageFieldVariant>, fields)
)

// std::string - package
struct GProtoFile {
	typedef boost::variant<GImport, std::string, GOption, GEnum, GMessage, GEmptyStatement> GProtoFileFieldVariant;
	std::string syntax;
	std::vector<GProtoFileFieldVariant> fields;
};


inline std::ostream& operator<<(std::ostream& stream, const GProtoFile& val) {
	stream << "GProtoFile{" << val.syntax << "|";
	for(const auto & s : val.fields)
		if(s.type() == typeid(GImport))
			stream << boost::get<GImport>(s) << "|";
		else if(s.type() == typeid(std::string))
			stream << boost::get<std::string>(s) << "|";
		else if(s.type() == typeid(GOption))
			stream << boost::get<GOption>(s) << "|";
		else if(s.type() == typeid(GEnum))
			stream << boost::get<GEnum>(s) << "|";
		else if(s.type() == typeid(GMessage))
			stream << boost::get<GMessage>(s) << "|";
    return stream << "}";
 }

BOOST_FUSION_ADAPT_STRUCT(GProtoFile,
	(std::string, syntax)
	(std::vector<GProtoFile::GProtoFileFieldVariant>, fields)
)
bool parse_proto(const std::string & str, GProtoFile & result);
bool parse_ident_split(const std::string & str, std::vector<std::string> & result);

void test_rules();
