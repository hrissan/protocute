#include "proto.hpp"
#include "protocute.hpp"

#include <iostream>
#include <fstream>
#include <string>
#include <complex>
#include <unistd.h>
#include <sstream>

static std::string boast = "// This file is generated by https://github.com/hrissan/protocute\n\n";

struct Namespace;

struct Namespace {
	enum Type { PACKAGE, MESSAGE, ENUM, BUILTIN };
	Type type = PACKAGE;
	std::string fullname;
	std::map<std::string, std::unique_ptr<Namespace>> member;
	
	std::string enum_default_value;
	bool has_sign = false;
};

struct CodeGenerator {
	const std::string only_name;
	const std::string cpp_out_path;
	std::ofstream hpp;
	std::stringstream cpp;
	
	std::vector<std::string> current_package;
	
	Namespace root_namespace;
	size_t total_messages = 0;
	std::vector<Namespace *> current_namespace;
	size_t indent = 0; // signle indent for a::b::c::d
	
	struct EnumGenerator {
		CodeGenerator & cg;
		explicit EnumGenerator(CodeGenerator & cg):cg(cg){
		}
		void operator()(GField const& s) {}
		void operator()(GEnum const& s);
		void operator()(GOption const& s) {}
		void operator()(GMessage const& s) {}
		void operator()(GEmptyStatement const& s) {}
	};
	struct MessageGenerator {
		CodeGenerator & cg;
		explicit MessageGenerator(CodeGenerator & cg):cg(cg){
		}
		void operator()(GField const& s) {}
		void operator()(GEnum const& s) {}
		void operator()(GOption const& s) {}
		void operator()(GMessage const& s);
		void operator()(GEmptyStatement const& s) {}
	};
	struct FieldGenerator {
		CodeGenerator & cg;
		size_t total_count = 0;
		explicit FieldGenerator(CodeGenerator & cg):cg(cg){
		}
		void operator()(GField const& s);
		void operator()(GEnum const& s) {}
		void operator()(GOption const& s) {}
		void operator()(GMessage const& s) {}
		void operator()(GEmptyStatement const& s) {}
	};
	struct FieldGeneratorRead {
		CodeGenerator & cg;
		explicit FieldGeneratorRead(CodeGenerator & cg):cg(cg){
		}
		void operator()(GField const& s);
		void operator()(GEnum const& s) {}
		void operator()(GOption const& s) {}
		void operator()(GMessage const& s) {}
		void operator()(GEmptyStatement const& s) {}
	};
	struct FieldGeneratorWrite {
		CodeGenerator & cg;
		explicit FieldGeneratorWrite(CodeGenerator & cg):cg(cg){
		}
		void operator()(GField const& s);
		void operator()(GEnum const& s) {}
		void operator()(GOption const& s) {}
		void operator()(GMessage const& s) {}
		void operator()(GEmptyStatement const& s) {}
	};
	void close_package(){
		if(current_package.empty())
			return;
		hpp << std::string(current_package.size(), '}') << " // ";
		std::string fullname;
		for(const auto & s : current_package){
			if(!fullname.empty())
				fullname += "::";
			fullname += s;
		}
		hpp << fullname << "\n\n";
		current_package.clear();
		current_namespace = {&root_namespace};
		indent = 0;
	}
	void add_builtin(const std::string & fullname, const std::string & cppname, const std::string & def, bool has_sign = false){
		auto cit = root_namespace.member.find(fullname);
		if(cit != root_namespace.member.end())
			throw std::runtime_error("Builtin name collision");
		cit = root_namespace.member.emplace(fullname, std::make_unique<Namespace>()).first;
		cit->second->type = Namespace::BUILTIN;
		cit->second->fullname = cppname;
		cit->second->enum_default_value = def;
		cit->second->has_sign = has_sign;
	}
	explicit CodeGenerator(const std::string & only_name, const std::string & cpp_out_path):only_name(only_name), cpp_out_path(cpp_out_path), hpp(cpp_out_path + "/" + only_name + ".hpp") {
		hpp << boast;
		hpp << "#include <string>\n";
		hpp << "#include <vector>\n";
		hpp << "#include <cstdint>\n\n";
		
		current_namespace = {&root_namespace};
		add_builtin("double", "double", "0");
		add_builtin("float", "float", "0");
		add_builtin("int32", "int32_t", "0", true);
		add_builtin("sint32", "int32_t", "0", true);
		add_builtin("sfixed32", "int32_t", "0", true);
		add_builtin("uint32", "uint32_t", "0");
		add_builtin("fixed32", "uint32_t", "0");
		add_builtin("int64", "int64_t", "0", true);
		add_builtin("sint64", "int64_t", "0", true);
		add_builtin("sfixed64", "int64_t", "0", true);
		add_builtin("uint64", "uint64_t", "0");
		add_builtin("fixed64", "uint64_t", "0");
		add_builtin("bool", "bool", "false");
		add_builtin("string", "std::string", "std::string{}");
		add_builtin("bytes", "std::string", "std::string{}");
	}
	
	void operator()(GImport const& s) {}
	void operator()(std::string const& s)
	{
		close_package();
		if(!parse_ident_split(s, current_package))
			throw std::runtime_error("Error parsing package name {" + s + "}");
		current_package.insert(current_package.begin(), "test");
		for(const auto & s : current_package){
			hpp << "namespace " << s << " { ";
			auto cn = current_namespace.back();
			auto cit = cn->member.find(s);
			if(cit == cn->member.end())
				cit = cn->member.emplace(s, std::make_unique<Namespace>()).first;
			cit->second->fullname = cn->fullname + "::" + s;
			if(cit->second->type != Namespace::PACKAGE)
				throw std::runtime_error("Package name collision with non-package");
			current_namespace.push_back(cit->second.get());
		}
		hpp << "\n\n";
		indent = 1;
	}
	void operator()(GOption const& s)
	{
	}
	void operator()(GEnum const& s){
		auto cn = current_namespace.back();
		auto cit = cn->member.find(s.name);
		if(cit != cn->member.end())
			throw std::runtime_error("Enum name collision");
		cit = cn->member.emplace(s.name, std::make_unique<Namespace>()).first;
		cit->second->fullname = cn->fullname + "::" + s.name;
		cit->second->type = Namespace::ENUM;
		hpp << std::string(indent, '\t') << "enum " << s.name << " {";
		bool first_field = true;
		for(const auto & f : s.fields)
			if(f.type() == typeid(GEnumField)){
				const auto & ef = boost::get<GEnumField>(f);
				if(cit->second->enum_default_value.empty())
					cit->second->enum_default_value = ef.name;
				if(!first_field)
					hpp << ",";
				first_field = false;
				hpp << "\n" << std::string(indent + 1, '\t') << ef.name << " = " << ef.value;
			}
//			else if(f.type() == typeid(GOption))
//				stream << boost::get<GOption>(f) << "|";
		hpp << "\n" << std::string(indent, '\t') << "};\n\n";
	}
	void operator()(GMessage const& s){
		hpp << std::string(indent, '\t') << "struct " << s.name << " { \n";
		auto cn = current_namespace.back();
		auto cit = cn->member.find(s.name);
		if(cit != cn->member.end())
			throw std::runtime_error("Message name collision");
		cit = cn->member.emplace(s.name, std::make_unique<Namespace>()).first;
		cit->second->fullname = cn->fullname + "::" + s.name;
		cit->second->type = Namespace::MESSAGE;
		total_messages += 1;
		current_namespace.push_back(cit->second.get());
		indent += 1;

		EnumGenerator eg(*this);
		for(const auto & f : s.fields)
			boost::apply_visitor(eg, f);
		MessageGenerator mg(*this);
		for(const auto & f : s.fields)
			boost::apply_visitor(mg, f);
		FieldGenerator fg(*this);
		for(const auto & f : s.fields)
			boost::apply_visitor(fg, f);
		
		cpp << "\tvoid read(" << cit->second->fullname << " & v, iterator s, iterator e){\n";
		cpp << "\t\twhile(s != e){\n";
		cpp << "\t\t\tauto m = read_varint(&s, e);\n";
		cpp << "\t\t\tauto field_type = static_cast<unsigned>(m & 7);\n";
		if(fg.total_count != 0)
			cpp << "\t\t\tauto field_number = static_cast<unsigned>(m >> 3);\n";
		cpp << "\t\t\t";

		FieldGeneratorRead fgr(*this);
		for(const auto & f : s.fields)
			boost::apply_visitor(fgr, f);
		if(fg.total_count != 0)
			cpp << "\n\t\t\t\t";
		cpp << "skip_by_type(field_type, &s, e);\n";
		cpp << "\t\t}\n\n";
		cpp << "\t}\n\n";
		cpp << "\tstd::string write(const " << cit->second->fullname << " & v){\n";
		cpp << "\t\tstd::string s;\n";
		FieldGeneratorWrite fgw(*this);
		for(const auto & f : s.fields)
			boost::apply_visitor(fgw, f);
		cpp << "\t\treturn s;\n";
		cpp << "\t}\n\n";

		current_namespace.pop_back();
		indent -= 1;
		hpp << std::string(indent, '\t') << "};\n\n";
	}
	void operator()(GEmptyStatement const& s) {}
	Namespace * resolve_type(const std::string & name){
		std::string str = name;
		if(str.empty())
			throw std::runtime_error("Type cannot be empty");
		size_t pos = current_namespace.size();
		if(str.at(0) == '.'){
			pos = 1;
			str.erase(str.begin());
		}
		std::vector<std::string> full_ident;
		if(!parse_ident_split(str, full_ident) || full_ident.empty())
			throw std::runtime_error("Error parsing full identifer name {" + str + "}");
		Namespace * found_cn = nullptr;
		for(size_t i = pos; i-- > 0; ){
			auto cn = current_namespace.at(i);
			auto cit = cn->member.find(full_ident.at(0));
			if(cit != cn->member.end()){
				found_cn = cit->second.get();
				full_ident.erase(full_ident.begin());
				while(!full_ident.empty()){
					auto cit = found_cn->member.find(full_ident.at(0));
					if(cit == found_cn->member.end())
						throw std::runtime_error("Type not found - " + name);
					found_cn = cit->second.get();
					full_ident.erase(full_ident.begin());
				}
				break;
			}
		}
		if(!found_cn)
			throw std::runtime_error("Type not found - " + name);
		if(found_cn->type == Namespace::PACKAGE)
			throw std::runtime_error("Type specified is a package - " + name);
		return found_cn;
	}
	void generate_field_read(GField const& s){
		auto found_cn = resolve_type(s.type);
		std::string ass = "\t\t\t\tv." + s.name + " = ";
		std::string ass2 = "";
		if( s.kind == GFieldKind::REPEATED){
			ass = "\t\t\t\tv." + s.name + ".push_back(";
			ass2 = ")";
			if( found_cn->type == Namespace::ENUM || found_cn->fullname == "bool" || s.type == "uint32" || s.type == "int32" || s.type == "uint64" || s.type == "int64"){
				cpp << "if(field_number == " << s.number << " && field_type == 2)\n";
				cpp << "\t\t\t\tread_packed_varint(v." << s.name << ", &s, e);\n\t\t\telse ";
			}else if( s.type == "sint32" || s.type == "sint64"){
				cpp << "if(field_number == " << s.number << " && field_type == 2)\n";
				cpp << "\t\t\t\tread_packed_s_varint(v." << s.name << ", &s, e);\n\t\t\telse ";
			}else if( s.type == "sfixed32" || s.type == "fixed32" || s.type == "float" || s.type == "sfixed64" || s.type == "fixed64" || s.type == "double"){
				cpp << "if(field_number == " << s.number << " && field_type == 2)\n";
				cpp << "\t\t\t\tread_packed_fixed(v." << s.name << ", &s, e);\n\t\t\telse ";
			}
		}
		cpp << "if(field_number == " << s.number << " && field_type == ";
		if(s.type == "sfixed32")
			cpp << "5)\n" << ass << "read_fixed<int32_t>(&s, e)" << ass2 << ";\n\t\t\t";
		else if(s.type == "fixed32")
			cpp << "5)\n" << ass << "read_fixed<uint32_t>(&s, e)" << ass2 << ";\n\t\t\t";
		else if(s.type == "sint32")
			cpp << "0)\n" << ass << "static_cast<int32_t>(zagzig(read_varint(&s, e)))" << ass2 << ";\n\t\t\t";
		else if(s.type == "sfixed64")
			cpp << "1)\n" << ass << "read_fixed<int64_t>(&s, e)" << ass2 << ";\n\t\t\t";
		else if(s.type == "fixed64")
			cpp << "1)\n" << ass << "read_fixed<uint64_t>(&s, e)" << ass2 << ";\n\t\t\t";
		else if(s.type == "sint64")
			cpp << "0)\n" << ass << "zagzig(read_varint(&s, e))" << ass2 << ";\n\t\t\t";
		else if(found_cn->fullname == "uint32_t")
			cpp << "0)\n" << ass << "read_varint_t<uint32_t>(&s, e)" << ass2 << ";\n\t\t\t";
		else if(found_cn->fullname == "uint64_t")
			cpp << "0)\n" << ass << "read_varint(&s, e)" << ass2 << ";\n\t\t\t";
		else if(found_cn->fullname == "int32_t")
			cpp << "0)\n" << ass << "read_varint_t<int32_t>(&s, e)" << ass2 << ";\n\t\t\t";
		else if(found_cn->fullname == "int64_t")
			cpp << "0)\n" << ass << "read_varint_t<int64_t>(&s, e)" << ass2 << ";\t\t\t\t";
		else if(found_cn->fullname == "bool")
			cpp << "0)\n" << ass << "read_varint(&s, e) != 0" << ass2 << ";\n\t\t\t";
		else if(found_cn->fullname == "float")
			cpp << "5)\n" << ass << "read_fixed<float>(&s, e)" << ass2 << ";\n\t\t\t";
		else if(found_cn->fullname == "double")
			cpp << "1)\n" << ass << "read_fixed<double>(&s, e)" << ass2 << ";\n\t\t\t";
		else if(found_cn->type == Namespace::ENUM)
			cpp << "0)\n" << ass << "read_varint_t<" << found_cn->fullname << ">(&s, e)" << ass2 << ";\n\t\t\t";
		else if(found_cn->fullname == "std::string")
			cpp << "2)\n" << ass << "read_string(&s, e)" << ass2 << ";\n\t\t\t";
		else {
			if( s.kind == GFieldKind::REPEATED){
				cpp << "2){\n\t\t\t\t" << s.name << ".resize(" << s.name << ".size() + 1);\n";
				cpp << "\t\t\t\tread_message(v." << s.name << ".back(), &s, e);\n\t\t\t}";
			}else{
				cpp << "\t\t\t\tread_message(v." << s.name << ", &s, e);\n\t\t\t";
			}
		}
		cpp << "else ";
	}
	void generate_field_write(GField const& s){
		auto found_cn = resolve_type(s.type);
		bool option_packed = false;
		for(const auto & op : s.options)
			if(op.option_name == "packed" && op.constant == "true")
				option_packed = true;
		size_t n_tabs = 2;
		std::string ref = "v." + s.name;
		if( s.kind == GFieldKind::REPEATED){
			if(option_packed){
				if( found_cn->type == Namespace::ENUM || found_cn->fullname == "bool" || s.type == "uint32" || s.type == "int32" || s.type == "uint64" || s.type == "int64"){
					cpp << "\t\t\twrite_packed_varint(" << s.number << ", v." << s.name << ", s);\n";
					return;
				}
				if( s.type == "sint32" || s.type == "sint64"){
					cpp << "\t\t\twrite_packed_s_varint(" << s.number << ", v." << s.name << ", s);\n";
					return;
				}
				if( s.type == "sfixed32" || s.type == "fixed32" || s.type == "float" || s.type == "sfixed64" || s.type == "fixed64" || s.type == "double"){
					cpp << "\t\t\twrite_packed_fixed(" << s.number << ", v." << s.name << ", s);\n";
					return;
				}
				throw std::runtime_error("Only primitive types can be packed");
			}
			cpp << "\t\tfor(const auto & vv : v." << s.name <<")\n";
			ref = "vv";
			n_tabs += 1;
		}else if( s.kind != GFieldKind::REQUIRED && !found_cn->enum_default_value.empty() ){
			if(found_cn->enum_default_value == "false"){
				cpp << std::string(n_tabs, '\t') << "if("<< ref << ")\n";
			}else if(found_cn->enum_default_value == "std::string{}"){
				cpp << std::string(n_tabs, '\t') << "if(!" << ref << ".empty())\n";
			}else if(found_cn->type == Namespace::ENUM){
				cpp << std::string(n_tabs, '\t') << "if(" << ref << " != " << found_cn->fullname << "::" << found_cn->enum_default_value << ")\n";
			}else {
				cpp << std::string(n_tabs, '\t') << "if(" << ref << " != " << found_cn->enum_default_value << ")\n";
			}
			n_tabs += 1;
		}
		if(s.type == "sint32")
			cpp << std::string(n_tabs, '\t') << "write_field_varint(" << s.number << ", zigzag(" << ref << "), s);\n";
		else if(s.type == "sfixed32" || s.type == "fixed32")
			cpp << std::string(n_tabs, '\t') << "write_field_fixed32(" << s.number << ", &" << ref << ", s);\n";
		else if(s.type == "sint64")
			cpp << std::string(n_tabs, '\t') << "write_field_varint(" << s.number << ", zigzag(" << ref << "), s);\n";
		else if(s.type == "sfixed64" || s.type == "fixed64")
			cpp << std::string(n_tabs, '\t') << "write_field_fixed64(" << s.number << ", &" << ref << ", s);\n";
		else if(found_cn->fullname == "int32_t")
			cpp << std::string(n_tabs, '\t') << "write_field_varint(" << s.number << ", static_cast<uint64_t>(static_cast<int64_t>(" << ref << ")), s);\n";
		else if(found_cn->fullname == "int64_t")
			cpp << std::string(n_tabs, '\t') << "write_field_varint(" << s.number << ", static_cast<uint64_t>(" << ref << "), s);\n";
		else if(found_cn->fullname == "uint32_t" || found_cn->fullname == "uint64_t")
			cpp << std::string(n_tabs, '\t') << "write_field_varint(" << s.number << ", " << ref << ", s);\n";
		else if(found_cn->fullname == "bool")
			cpp << std::string(n_tabs, '\t') << "write_field_varint(" << s.number << ", " << ref << " ? 1 : 0, s);\n";
		else if(found_cn->fullname == "float")
			cpp << std::string(n_tabs, '\t') << "write_field_fixed32(" << s.number << ", &" << ref << ", s);\n";
		else if(found_cn->fullname == "double")
			cpp << std::string(n_tabs, '\t') << "write_field_fixed64(" << s.number << ", &" << ref << ", s);\n";
		else if(found_cn->type == Namespace::ENUM)
			cpp << std::string(n_tabs, '\t') << "write_field_varint(" << s.number << ", static_cast<uint64_t>(" << ref << "), s);\n";
		else if(found_cn->fullname == "std::string")
			cpp << std::string(n_tabs, '\t') << "write_field_string(" << s.number << ", " << ref << ", s);\n";
		else
			cpp << std::string(n_tabs, '\t') << "write_field_string(" << s.number << ", write(" << ref << "), s);\n";
	}
	void generate_field(GField const& s){
		hpp << std::string(indent, '\t');
		if(s.kind == GFieldKind::REPEATED){
			hpp << "std::vector<";
		}
		auto found_cn = resolve_type(s.type);
		if(found_cn->type == Namespace::BUILTIN)
			hpp << found_cn->fullname;
		else
			hpp << s.type; // We hope C++ resolves in the same as we do
		if(s.kind == GFieldKind::REPEATED){
			hpp << ">";
		}
		hpp << " " << s.name;
		if(s.kind != GFieldKind::REPEATED && !found_cn->enum_default_value.empty() && found_cn->enum_default_value != "std::string{}")
			hpp << " = " << found_cn->enum_default_value;
		hpp << ";\n";
	}
	void gen_defs(const Namespace & na){
		if(na.type == Namespace::MESSAGE){
			hpp << "\tvoid read(" << na.fullname << " & v, iterator s, iterator e);\n";
			hpp << "\tstd::string write(const " << na.fullname << " & v);\n\n";
		}
		for(const auto & m : na.member){
			gen_defs(*m.second.get());
		}
	}
	void finish_header(){
		close_package();
		
		std::ofstream real_cpp(cpp_out_path + "/" + only_name + ".cpp");

		real_cpp << boast;
		
		real_cpp << "#include \"" << only_name << ".hpp\"\n";

		if( total_messages != 0){
			hpp << "namespace protocute {\n\n";
			hpp << "\ttypedef std::string::const_iterator iterator;\n\n";

			gen_defs(root_namespace);
			hpp << "} // namespace protocute\n\n";
			
			real_cpp << "#include \"protocute.hpp\"\n\n";
			real_cpp << "namespace protocute {\n\n";

 			real_cpp << cpp.str();
			
			real_cpp << "} // namespace protocute\n\n";
		}
	}
};

void CodeGenerator::EnumGenerator::operator()(GEnum const& s){
	cg.operator()(s);
}

void CodeGenerator::MessageGenerator::operator()(GMessage const& s) {
	cg.operator()(s);
}

void CodeGenerator::FieldGenerator::operator()(GField const& s){
	total_count += 1;
	cg.generate_field(s);
}

void CodeGenerator::FieldGeneratorRead::operator()(GField const& s){
	cg.generate_field_read(s);
}
void CodeGenerator::FieldGeneratorWrite::operator()(GField const& s){
	cg.generate_field_write(s);
}

int generate(std::string pf, std::vector<std::string> import_paths, std::string cpp_out_path)
{
	auto only_name = pf.substr(0, pf.size() - 6);
	auto pos = only_name.find_last_of("/");
	if(pos != std::string::npos)
		only_name = only_name.substr(pos + 1);

	GProtoFile result;

	std::ifstream t(pf);
	std::string str(std::istreambuf_iterator<char>{t},
                 std::istreambuf_iterator<char>{});
	if(!parse_proto(str, result)){
		throw std::runtime_error("Proto file parse failed (probably contains feature protocute does not support yet)");
	}

	CodeGenerator generator(only_name, cpp_out_path); // , import_paths
	
	for(const auto & s : result.fields){
		boost::apply_visitor(generator, s);
	}
	
	generator.finish_header();
/*	test_rules();

	std::vector<std::string> spl;
	bool a1 = parse_ident_split("a.bcd.ef", spl);
	bool a2 = parse_ident_split("bcd.ef1-", spl);
	bool a3 = parse_ident_split("bcd", spl);
	GProtoFile result;

//	bool ok = parse_proto("syntax = \"proto2\";\npackage hw.trezor.messages.common;; enum EnumAllowingAlias {A=1; B=2;} message Outer{}", result);
//	std::cout << int(ok) << " " << result << std::endl;
	
	std::ifstream t("test.proto");
	std::string str(std::istreambuf_iterator<char>{t},
                 std::istreambuf_iterator<char>{});

//	result = GProtoFile{};
	bool ok = parse_proto(str, result);
	std::cout << int(ok) << " " << result << std::endl;

	CodeGenerator generator(cwd, "test");
	
	for(const auto & s : result.fields){
		boost::apply_visitor(generator, s);
	}
	
	generator.finish_header();

//	std::cout << str << std::endl;*/
	return 0;
}

//#include "test.hpp"

static std::string normalize_path(std::string str){
	while(!str.empty() && str.back() == '/')
		str.pop_back();
	return str;
}

int main(int argc, const char * argv[])
{
	std::vector<std::string> import_paths;
	std::vector<std::string> protofiles;
	std::string cpp_out_path;
	for(int i = 1; i < argc; ++i){
		if(argv[i] == std::string("-version")){
			std::cout << "protocute 0.1" << std::endl;
			return 0;
		}
		if(argv[i] == std::string("-h") || argv[i] == std::string("--help")){
			std::cout << "protocute [OPTIONS] PROTO_FILES" << std::endl;
			std::cout << "    --version                   Show version info and exit." << std::endl;
			std::cout << "    -h, --help                  Show this text and exit." << std::endl;
			std::cout << "    -IPATH, --proto_path=PATH   Specify the directory in which to search for" << std::endl;
            std::cout << "                                imports.  May be specified multiple times;" << std::endl;
            std::cout << "                                directories will be searched in order.  If not" << std::endl;
            std::cout << "                                given, the current working directory is used." << std::endl;
            std::cout << "    --cpp_out=OUT_DIR           Generate C++ header and source." << std::endl;
			return 0;
		}
		if(argv[i] == std::string("-IPATH") && i < argc - 1)
			import_paths.push_back(normalize_path(argv[i + 1]));
		if(std::string(argv[i]).find("--proto_path=") == 0)
			import_paths.push_back(normalize_path(std::string(argv[i]).substr(13)));
		if(std::string(argv[i]).find("--cpp_out=") == 0)
			cpp_out_path = normalize_path(std::string(argv[i]).substr(10));
		if(argv[i][0] != '-')
			protofiles.push_back(argv[i]);
	}
	if(import_paths.empty()){
		char cwd[PATH_MAX] = {};
		if (!getcwd(cwd, sizeof(cwd))) {
			perror("getcwd() error");
			return 1;
		}
		import_paths.push_back(normalize_path(cwd));
		std::cout << "No import paths specified, using current folder " << import_paths.back() << std::endl;
	}
	if(cpp_out_path.empty()){
		std::cout << "--cpp_out=<OUTDIR> argument is mandatory" << std::endl;
		return 1;
	}
	if(protofiles.empty()){
		std::cout << "No protofile specifed" << std::endl;
		return 1;
	}
	for(auto pf : protofiles){
		if(pf.size() < 6 || pf.substr(pf.size() - 6) != ".proto" ){
			std::cout << "Protobuf file must end with .proto, actual name is " << pf << std::endl;
			return 1;
		}
		generate(pf, import_paths, cpp_out_path);
	}
//	::hw::trezor::messages::common::HDNodeType node, node2;
//	node.depth = 1;
//	node.chain_code = "hren";
//	std::string str = protocute::write(node);
	
//	protocute::read(node2, str.begin(), str.end());
	return 0;
}
