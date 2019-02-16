#include <boost/config/warning_disable.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/qi_hold.hpp>
#include <boost/spirit/include/phoenix.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_object.hpp>
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/fusion/include/std_pair.hpp>
#include <boost/fusion/include/io.hpp>
#include <boost/core/demangle.hpp>
#include <boost/variant/recursive_variant.hpp>

#include <iostream>
#include <string>
#include <complex>

namespace qi = boost::spirit::qi;
namespace ascii = boost::spirit::ascii;
namespace phx    = boost::phoenix;

namespace fusion = boost::fusion;

using qi::lexeme;
using ascii::char_;

namespace spirit = boost::spirit;

    template <typename Expr, typename Iterator = spirit::unused_type>
    struct attribute_of_parser
    {
        typedef typename spirit::result_of::compile<
            spirit::qi::domain, Expr
        >::type parser_expression_type;

        typedef typename spirit::traits::attribute_of<
            parser_expression_type, spirit::unused_type, Iterator
        >::type type;
    };

    template <typename T>
    void display_attribute_of_parser(T const &)
    {
        // Report invalid expression error as early as possible.
        // If you got an error_invalid_expression error message here,
        // then the expression (expr) is not a valid spirit qi expression.
        BOOST_SPIRIT_ASSERT_MATCH(spirit::qi::domain, T);

        typedef typename attribute_of_parser<T>::type type;
        std::cout << typeid(type).name() << std::endl;
    }

    template <typename T>
    void display_attribute_of_parser(std::ostream& os, T const &)
    {
        // Report invalid expression error as early as possible.
        // If you got an error_invalid_expression error message here,
        // then the expression (expr) is not a valid spirit qi expression.
        BOOST_SPIRIT_ASSERT_MATCH(spirit::qi::domain, T);

        typedef typename attribute_of_parser<T>::type type;
//        os << typeid(type).name() << std::endl;
        os << "{ " << boost::core::demangle(typeid(type).name()) << " }" << std::endl;
    }

typedef std::string::iterator Iterator;

typedef std::pair<int, std::string> LiteralType;

std::ostream& operator<<(std::ostream& stream, const LiteralType& val) {
    return stream << "LiteralType{" << val.first << "}";
 }

struct GImport {
	boost::optional<int> kind;
    std::string filename;
};

std::ostream& operator<<(std::ostream& stream, const GImport& val) {
    return stream << "GImport{" << (val.kind ? val.kind.get() : 0) << "|" << val.filename << "}";
 }

std::ostream& operator<<(std::ostream& stream, const std::vector<std::string> & val) {
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

std::ostream& operator<<(std::ostream& stream, const GOption& val) {
    return stream << "GOption{" << val.option_name << "|" << val.constant << "}";
 }

BOOST_FUSION_ADAPT_STRUCT(GOption,
	(std::string, option_name)
	(std::string, constant)
)


struct GField {
	int kind = 0;
    std::string type;
    std::string name;
    std::string number;
    std::vector<GOption> options;
};

std::ostream& operator<<(std::ostream& stream, const GField& val) {
    return stream << "GField{" << val.kind << "|" << val.type << "|" << val.name << "|" << val.number << "|" << val.options.size() << "}";
 }

BOOST_FUSION_ADAPT_STRUCT(GField,
	(int, kind)
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
	typedef boost::variant<GEnumField, GOption, GEmptyStatement> GEnumFieldVariant;
	std::string name;
	std::vector<GEnumFieldVariant> fields;
};

std::ostream& operator<<(std::ostream& stream, const GEnumField& val) {
    return stream << "GEnumField{" << val.name << "|" << val.value << "|" << val.options.size() << "}";
 }

std::ostream& operator<<(std::ostream& stream, const GEnum& val) {
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

struct GMessage;

typedef boost::variant<boost::recursive_wrapper<GMessage>, GField, GEnum, GOption, GEmptyStatement> GMessageFieldVariant;

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
	void operator()(GMessage const& s) const;
	void operator()(GEmptyStatement const& s) const
	{}
};

std::ostream& operator<<(std::ostream& stream, const GMessage& val) {
	stream << "GMessage{" << val.name << "|";
	for(const auto & s : val.fields){
		boost::apply_visitor(PrintVisitor{stream}, s);
 		stream << "|";
	}
    return stream << "}";
 }

void PrintVisitor::operator()(GMessage const& s) const
{
	stream << s;
}

BOOST_FUSION_ADAPT_STRUCT(GMessage,
	(std::string, name)
	(std::vector<GMessageFieldVariant>, fields)
)

struct skip_grammar : qi::grammar<Iterator> {
    qi::rule<Iterator> start;
    skip_grammar() : skip_grammar::base_type(start) {
        single_line_comment = "//" >> *(char_ - spirit::eol) >> (spirit::eol|spirit::eoi);
        block_comment = ("/*" >> *(block_comment | char_ - "*/")) > "*/";

        start = single_line_comment | block_comment | qi::blank;
    }
    qi::rule<Iterator> block_comment, single_line_comment, skipper;
};


template<class R>
void test_parse(std::string str, R & r, std::string must_be){
	typename R::attr_type data{};
	Iterator first = str.begin();
    bool ok = parse(first, str.end(), r, data);
    if (ok) {
        std::cout << str << " -> " << data;
		if (first != str.end()) {
			std::cout << " --| " << std::string(first, str.end());
		}
    } else {
        std::cout << str << " x ";
    }
    std::cout << std::endl;
	if(std::string(str.begin(), first) != must_be)
		std::cout << "====BAD, must be | " << must_be << std::endl;
}

template<class R>
void test_phrase_parse(std::string str, R & r, std::string must_be){
	typename R::attr_type data{};
	Iterator first = str.begin();
    skip_grammar ws;
	qi::on_error<qi::fail>(r,
					phx::ref(std::cout)
					   << "Error! Expecting "
					   << qi::_4
					   << " here: '"
					   << phx::construct<std::string>(qi::_3, qi::_2)
					   << "'\n"
				);
	bool ok = phrase_parse(first, str.end(), r, ws, data);
    if (ok) {
        std::cout << "{" << str << "} -> " << data;
		if (first != str.end()) {
			std::cout << " --|" << std::string(first, str.end()) << "|--";
		}
    } else {
        std::cout << "{" << str << "} x";
    }
    std::cout << std::endl;
	if(std::string(str.begin(), first) != must_be)
		std::cout << "====BAD, actual {" << std::string(str.begin(), first) << "}" << std::endl;
}

qi::rule<Iterator, std::string()> g_decimalLit = char_("1-9") >> *qi::digit;
qi::rule<Iterator, std::string()> g_octalLit = char_("0") >> *char_("0-7");
qi::rule<Iterator, std::string()> g_hexLit = qi::hold[char_("0") >> (char_("x") | char_("X")) >> +qi::xdigit];
qi::rule<Iterator, std::string()> g_intLit = g_decimalLit | g_hexLit | g_octalLit;

qi::rule<Iterator, std::string()> g_exponent = qi::hold[char_("eE") >> (-char_("+-")) >> +qi::digit];
qi::rule<Iterator, std::string()> g_floatLit = qi::hold[(+qi::digit >> char_(".") >> *qi::digit >> qi::hold[-g_exponent])] | qi::hold[(+qi::digit >> g_exponent)] | qi::hold[(char_(".") >> +qi::digit >> -g_exponent)] | (char_("i") >> char_("n") >> char_("f")) | (char_("n") >> char_("a") >> char_("n"));

qi::rule<Iterator, std::string()> g_boolLit = (char_("f") >> char_("a") >> char_("l") >> char_("s") >> char_("e")) | (char_("t") >> char_("r") >> char_("u") >> char_("e"));
qi::rule<Iterator, std::string()> g_hexEscape = char_("\\") >> char_("xX") >> qi::xdigit >> qi::xdigit;
qi::rule<Iterator, std::string()> g_octalEscape = char_("\\") >> char_("0-7") >> char_("0-7") >> char_("0-7");
qi::rule<Iterator, std::string()> g_charEscape = char_("\\") >> char_("abfnrtv\\\"\'");
qi::rule<Iterator, std::string()> g_charValue1 = qi::hold[g_hexEscape] | qi::hold[g_octalEscape] | qi::hold[g_charEscape] | (qi::char_ - (qi::char_("\"\n\\") | qi::char_('\0')));
qi::rule<Iterator, std::string()> g_charValue2 = qi::hold[g_hexEscape] | qi::hold[g_octalEscape] | qi::hold[g_charEscape] | (qi::char_ - (qi::char_("\'\n\\") | qi::char_('\0')));

qi::rule<Iterator, std::string()> g_strLit = qi::hold[(char_("\"") >> *g_charValue1 >> char_("\""))] | qi::hold[(char_("\'") >> *g_charValue2 >> char_("\'"))];

qi::rule<Iterator, std::string()> g_ident = qi::alpha >> *(qi::alpha | qi::digit | char_("_"));
qi::rule<Iterator, std::string()> g_fullIdent = g_ident >> *qi::hold[(qi::char_(".") >> g_ident)];
qi::rule<Iterator, std::string()> g_messageEnumType = -qi::char_(".") >> g_fullIdent;

qi::rule<Iterator, std::string(), skip_grammar> g_syntax = qi::lit("syntax") > qi::lit('=') > g_strLit > qi::lit(';');
qi::rule<Iterator, GImport(), skip_grammar> g_import = qi::lit("import") > -((qi::lit("public") >> qi::attr(1)) | (qi::lit("weak") >> qi::attr(2))) > g_strLit > qi::lit(';');
qi::rule<Iterator, std::string(), skip_grammar> g_package = qi::lit("package") > g_fullIdent > qi::lit(';');

qi::rule<Iterator, std::string(), skip_grammar> g_optionName = qi::hold[g_ident | (qi::char_("(") >> g_fullIdent >> qi::char_(")"))] >> qi::hold[-(qi::char_(".") >> g_ident)];

qi::rule<Iterator, std::string(), skip_grammar> g_constant = g_fullIdent | (-qi::char_("-+") >> g_intLit)  | (-qi::char_("-+") >> g_floatLit) | g_strLit | g_boolLit;

qi::rule<Iterator, GOption(), skip_grammar> g_option = qi::lit("option") > g_optionName > qi::lit('=') > g_constant > qi::lit(';');

qi::rule<Iterator, GOption(), skip_grammar> g_fieldOption = g_optionName > qi::lit('=') > g_constant;
qi::rule<Iterator, std::vector<GOption>(), skip_grammar> g_fieldOptions = g_fieldOption >> *(qi::lit(",") >> g_fieldOption);
qi::rule<Iterator, int(), skip_grammar> g_fieldLabel = (qi::lit("required") >> qi::attr(1)) | (qi::lit("optional") >> qi::attr(2)) | (qi::lit("repeated") >> qi::attr(3));
qi::rule<Iterator, GField(), skip_grammar> g_field = g_fieldLabel > g_fullIdent > g_ident > qi::lit('=') > g_intLit > ((qi::lit("[") > g_fieldOptions > qi::lit("]")) | qi::attr(std::vector<GOption>{})) > qi::lit(';');

qi::rule<Iterator, GEnumField(), skip_grammar> g_enumField = g_ident > qi::lit('=') > g_intLit > ((qi::lit("[") > g_fieldOptions > qi::lit("]")) | qi::attr(std::vector<GOption>{})) > qi::lit(';');
qi::rule<Iterator, GEnum(), skip_grammar> g_enum = qi::lit("enum") > g_ident > qi::lit('{') > *(g_option | g_enumField | (qi::lit(";") >> qi::attr(GEmptyStatement{}))) > qi::lit('}');

// Those ideas below were all bad ideas, we'll just skip them.
// TODO - group
// TODO - oneof
// TODO - map
// TODO - extensions
// TODO - ranges
// TODO - range
// TODO - reserved
// TODO - extend
// TODO - service

int main()
{
	qi::rule<Iterator, GMessage(), skip_grammar> g_message;

	g_message = qi::lit("message") > g_ident > qi::lit('{') > *(g_field | g_enum | g_message | g_option | (qi::lit(";") >> qi::attr(GEmptyStatement{}))) > qi::lit('}');

	g_ident.name("Identifier");
	g_fullIdent.name("Fully quialified identifier");
	test_parse("123", g_intLit, "123");
	test_parse("12A", g_intLit, "12");
	test_parse("0", g_intLit, "0");
	test_parse("0x123", g_intLit, "0x123");
	test_parse("0X12Az", g_intLit, "0X12A");
	test_parse("0xZ", g_intLit, "0");
	test_parse("0123", g_intLit, "0123");
	test_parse("0012A", g_intLit, "0012");
	test_parse("0.14e+13", g_floatLit, "0.14e+13");
	test_parse(".14e-0", g_floatLit, ".14e-0");
	test_parse("14.", g_floatLit, "14.");
	test_parse("truee", g_boolLit, "true");
	test_parse("false", g_boolLit, "false");
	test_parse("truka", g_boolLit, "");
    test_parse("\"abcde\"", g_strLit, "\"abcde\"");
    test_parse("\"abc", g_strLit, "");
    test_parse("\'ab\\x65c\'", g_strLit, "\'ab\\x65c\'");
    test_parse("\'ab\\x6Uc\'", g_strLit, "");
    test_parse("\'ab\\n\\777\"\';", g_strLit, "\'ab\\n\\777\"\'");
	test_parse("Arka.", g_ident, "Arka");
	test_parse("1ku", g_ident, "");
	test_parse("s_10+", g_ident, "s_10");
	test_parse("Arka.", g_fullIdent, "Arka");
	test_parse("a.ku.1ku", g_fullIdent, "a.ku");
	test_parse(".swe", g_fullIdent, "");
	test_parse("Arka.", g_messageEnumType, "Arka");
	test_parse("a.ku.1ku", g_messageEnumType, "a.ku");
	test_parse(".sw_e.n.1", g_messageEnumType, ".sw_e.n");
    test_phrase_parse("import \"hren\";z", g_import, "import \"hren\";");
    test_phrase_parse("import public \"hren\" ;", g_import, "import public \"hren\" ;");
    test_phrase_parse(" import weak \"hren\" ; q", g_import, " import weak \"hren\" ; ");
    test_phrase_parse("import wea \"hren\";", g_import, "");
    test_phrase_parse("syntax = \"proto2\" ; q", g_syntax, "syntax = \"proto2\" ; ");
    test_phrase_parse(" syntax=\'proto5\' ; q", g_syntax, " syntax=\'proto5\' ; ");
    test_phrase_parse("syntax = \"proto2 ; q", g_syntax, "");
    test_phrase_parse(" syntax=proto3\";  ", g_syntax, "");

    test_phrase_parse("package hren ;", g_package, "package hren ;");
    test_phrase_parse("package hren.vam ;", g_package, "package hren.vam ;");
    test_phrase_parse("package zlo.va.1 ;", g_package, "");
    test_phrase_parse("package .hren ;", g_package, "");
    test_phrase_parse("package 1;", g_package, "");

    test_phrase_parse("abc1", g_optionName, "abc1");
    test_phrase_parse("abc.def", g_optionName, "abc");
    test_phrase_parse("( hr )", g_optionName, "( hr )");
    test_phrase_parse("( hr.as ).as", g_optionName, "( hr.as ).as");
    test_phrase_parse("( hr.as.1 )", g_optionName, "");

    test_phrase_parse("option abc1 = \"zz\";", g_option, "option abc1 = \"zz\";");
    test_phrase_parse("option (x.a) = -5;", g_option, "option (x.a) = -5;");

    test_phrase_parse("optional foo.bar nested_message = 2;", g_field, "optional foo.bar nested_message = 2;");
    test_phrase_parse("repeated int32 samples = 4 [packed=true];", g_field, "repeated int32 samples = 4 [packed=true];");

    test_phrase_parse("STARTED = 1;", g_enumField, "STARTED = 1;");
    test_phrase_parse("RUNNING = 2 [(custom_option) = \"hello world\"];", g_enumField, "RUNNING = 2 [(custom_option) = \"hello world\"];");

    test_phrase_parse("enum EnumAllowingAlias {}", g_enum, "enum EnumAllowingAlias {}");
    test_phrase_parse("enum EnumAllowingAlias {UNKNOWN = 0; option allow_alias = true; ; STARTED = 1;};", g_enum, "enum EnumAllowingAlias {UNKNOWN = 0; option allow_alias = true; ; STARTED = 1;}");

    test_phrase_parse("message Outer { required int64 ival = 1; enum EnumAllowingAlias {A = 0; B = 1;} };", g_message, "message Outer { required int64 ival = 1; enum EnumAllowingAlias {A = 0; B = 1;} }");
/*	test2(example1);
    test2("syntax = \"proto2\" ; \"aha\"     zlo ");
    test2(" syntax= \"proto3\"; \"aha\" ");
    test2("01.");
    test2("1");
    test2("01");
    test2("01.");
	test2("12.13");
	test2("12.13e+");
	test2("12.13e+0");
	test2("12.e+");
	test2("12.e+0");
	test2("12e+");
	test2("12e+0");
	test2("1.e0x12");
	test2("0");
	test2("e-012");
	test2("0a");
	test2("00a");
	test2("e+00x1a");
	test2("012.3s");
    test2("inf0x12");
    test2("in0x12");
    test2(".012e");*/

/*    std::cout
        << "Give me an employee of the form :"
        << "employee{age, \"surname\", \"forename\", salary } \n";
    std::cout << "Type [q or Q] to quit\n\n";

    using boost::spirit::ascii::space;
    typedef std::string::const_iterator iterator_type;
    typedef client::employee_parser<iterator_type> employee_parser;

    employee_parser g; // Our grammar
    std::string str;
    while (getline(std::cin, str))
    {
        if (str.empty() || str[0] == 'q' || str[0] == 'Q')
            break;

        client::employee emp;
        std::string::const_iterator iter = str.begin();
        std::string::const_iterator end = str.end();
        bool r = phrase_parse(iter, end, g, space, emp);

        if (r && iter == end)
        {
            std::cout << boost::fusion::tuple_open('[');
            std::cout << boost::fusion::tuple_close(']');
            std::cout << boost::fusion::tuple_delimiter(", ");

            std::cout << "-------------------------\n";
            std::cout << "Parsing succeeded\n";
            std::cout << "got: " << boost::fusion::as_vector(emp) << std::endl;
            std::cout << "\n-------------------------\n";
        }
        else
        {
            std::cout << "-------------------------\n";
            std::cout << "Parsing failed\n";
            std::cout << "-------------------------\n";
        }
    }

    std::cout << "Bye... :-) \n\n";*/
    return 0;
}

/*namespace client
{

    ///////////////////////////////////////////////////////////////////////////
    //  Our employee struct
    ///////////////////////////////////////////////////////////////////////////
    //[tutorial_employee_struct
    struct employee
    {
        int age;
        std::string surname;
        std::string forename;
        double salary;
    };
    //]
}

// We need to tell fusion about our employee struct
// to make it a first-class fusion citizen. This has to
// be in global scope.

//[tutorial_employee_adapt_struct
BOOST_FUSION_ADAPT_STRUCT(
    client::employee,
    (int, age)
    (std::string, surname)
    (std::string, forename)
    (double, salary)
)
//]

namespace client
{
    ///////////////////////////////////////////////////////////////////////////////
    //  Our employee parser
    ///////////////////////////////////////////////////////////////////////////////
    //[tutorial_employee_parser
    template <typename Iterator>
    struct employee_parser : qi::grammar<Iterator, employee(), ascii::space_type>
    {
        employee_parser() : employee_parser::base_type(start)
        {
            using qi::int_;
            using qi::lit;
            using qi::double_;
            using qi::lexeme;
            using ascii::char_;

            quoted_string %= lexeme['"' >> +(char_ - '"') >> '"'];

            start %=
                lit("employee")
                >> '{'
                >>  int_ >> ','
                >>  quoted_string >> ','
                >>  quoted_string >> ','
                >>  double_
                >>  '}'
                ;
        }
		
        qi::rule<Iterator, std::string(), ascii::space_type> quoted_string;
        qi::rule<Iterator, employee(), ascii::space_type> start;
    };
    //]
}*/
