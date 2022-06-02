// This file is generated by https://github.com/hrissan/protocute

#include <string>
#include <vector>
#include <cstdint>
#include <optional>
#include <variant>

namespace test123 { 

enum class Side {
	LEFT = 0,
	RIGHT = 1,
	UNKNOWN_SIDE = 99
};

struct Types { 
	int32_t value1 = 0;
	std::vector<uint64_t> value2;
	Side side = ::test123::Side::LEFT;
	std::string str;
	bool boo = false;
	std::string bite;
	float num1 = 0;
	std::vector<double> num2;
};

struct Point { 
	int64_t x = 0;
	int64_t y = 0;
	std::optional<int64_t> z;
};

struct Something { 
	std::variant<std::monostate,
		std::string,
		std::string,
		Point
	> position;
	enum position_num {
		i_name = 1,
		i_name_alt = 2,
		i_pos = 3
	};
};

} // namespace test123

namespace protocute {

typedef const char * iterator;

void read(::test123::Point & v, iterator s, iterator e);
std::string write(const ::test123::Point & v);

void read(::test123::Something & v, iterator s, iterator e);
std::string write(const ::test123::Something & v);

void read(::test123::Types & v, iterator s, iterator e);
std::string write(const ::test123::Types & v);

} // namespace protocute

