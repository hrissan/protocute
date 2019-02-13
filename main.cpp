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

#include <iostream>
#include <string>
#include <complex>

namespace qi = boost::spirit::qi;
namespace ascii = boost::spirit::ascii;
namespace phx    = boost::phoenix;

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

struct convert_f {
    LiteralType operator()(const std::string & text) const {
    	return LiteralType{1, text};
    }
};

boost::phoenix::function<convert_f> convert;

void test2(std::string str){
	qi::rule<Iterator, std::string()> g_decimalLit = char_("1-9") >> *qi::digit;
	qi::rule<Iterator, std::string()> g_octalLit = char_("0") >> *char_("0-7");
	qi::rule<Iterator, std::string()> g_hexLit = char_("0") >> (char_("x") | char_("X")) >> +qi::xdigit;
	qi::rule<Iterator, std::string()> g_intLit = g_decimalLit | g_octalLit | g_hexLit;

	qi::rule<Iterator, LiteralType()> g_decimalLit2 = qi::as_string[char_("1-9") >> *qi::digit][ qi::_val = phx::construct<LiteralType>(1, qi::_1)];
	qi::rule<Iterator, LiteralType()> g_octalLit2 = qi::as_string[char_("0") >> *char_("0-7")][ qi::_val = phx::construct<LiteralType>(2, qi::_1)];
	qi::rule<Iterator, LiteralType()> g_hexLit2 = qi::as_string[char_("0") >> (char_("x") | char_("X")) >> +qi::xdigit][ qi::_val = phx::construct<LiteralType>(3, qi::_1)];
	qi::rule<Iterator, LiteralType()> g_intLit2 = g_decimalLit2 | g_hexLit2 | g_octalLit2;
//	qi::rule<Iterator, std::string()> g_exponent = (char_("e") | char_("E")) >> ((char_("+") >> +qi::digit) | (char_("-") >> +qi::digit) | +qi::digit);
	qi::rule<Iterator, std::string()> g_exponent = char_("eE") >> (-char_("+-")) >> +qi::digit;
	qi::rule<Iterator, LiteralType()> g_floatLit2 = qi::as_string[qi::hold[(+qi::digit >> char_(".") >> *qi::digit >> qi::hold[-g_exponent])] | qi::hold[(+qi::digit >> g_exponent)] | qi::hold[(char_(".") >> +qi::digit >> -g_exponent)] | (char_("i") >> char_("n") >> char_("f")) | (char_("n") >> char_("a") >> char_("n"))][ qi::_val = phx::construct<LiteralType>(4, qi::_1)];

	LiteralType data;
	Iterator first = str.begin();
    bool ok = parse(first, str.end(), g_floatLit2, data);
    if (ok) {
        std::cout << "Parse success " << str << " -> {" << data.first << "|" << data.second << "}";
		std::cout << std::endl;
    } else {
        std::cout << "Parse failed" << std::endl;
    }

    if (first != str.end()) {
        std::cout << "Remaining unparsed: '" << std::string(first, str.end()) << "'\n";
    }
}

void test1(std::string str){
	qi::rule<Iterator, std::string()> g_ident = lexeme[ ascii::alpha >> *(ascii::alnum) ]; //  | char_('_')
	qi::rule<Iterator, std::vector<std::string>()> g_fullIdent = g_ident >> *(qi::lit(".") >> g_ident);
//	qi::rule<Iterator, std::pair<boost::optional<std::string>, std::vector<std::string>>()> g_name = -char_(".") >> g_fullIdent;

//	display_attribute_of_parser(g_ident);
//	display_attribute_of_parser(qi::matches[-char_(".")] >> g_ident);
	qi::rule<Iterator, std::pair<bool, std::vector<std::string>>()> g_fullName = -qi::matches[char_(".")] >> g_fullIdent;

	std::pair<bool, std::vector<std::string>> data;
//	std::vector<std::string> data;
	Iterator first = str.begin();
    bool ok = parse(first, str.end(), g_fullName, data);
    if (ok) {
        std::cout << "Parse success {" << int(data.first) << "}";
    	for(auto && s : data.second)
        	std::cout << "{" << s << "}";
		std::cout << std::endl;
    } else {
        std::cout << "Parse failed" << std::endl;
    }

    if (first != str.end()) {
        std::cout << "Remaining unparsed: '" << std::string(first, str.end()) << "'\n";
    }
}

int main()
{
	test2("1");
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
	test1("Hr.en123");
	test1("Hren.*123");
	test1(".Zopa .Hren.*123");
	test1("1Hren");

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
