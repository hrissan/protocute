#include <string>
#include <vector>
#include <cstdint>

namespace protocute {
	
typedef std::string::const_iterator iterator;

inline uint64_t read_varint(iterator * s, iterator e){
	size_t read = 0;
	const size_t bits = 64;
	uint64_t result = 0;
	for (size_t shift = 0;; shift += 7) {
		if (*s == e)
			throw std::runtime_error("varint end of input");
		unsigned char byte = *(*s)++;
		++read;
		if (shift + 7 >= bits && byte >= 1 << (bits - shift))
			throw std::runtime_error("varint 64-bit overflow");
		if (byte == 0 && shift != 0)
			throw std::runtime_error("varint non-canonical rep");
		result |= static_cast<uint64_t>(byte & 0x7f) << shift;
		if ((byte & 0x80) == 0)
			break;
	}
	return result;
}
inline size_t read_varint_size_t(iterator * s, iterator e){
	return static_cast<size_t>(read_varint(s, e));
}
inline void write_varint(uint64_t v, std::string & s){
	while (v >= 0x80) {
		s.push_back(static_cast<char>((v & 0x7f) | 0x80));
		v >>= 7;
	}
	s.push_back(static_cast<char>(v));
}

inline iterator skip(iterator * s, iterator e, size_t len){
	if(e - *s > len)
		throw std::runtime_error("protocute skip underflow");
	iterator result = *s;
	*s += len;
	return result;
}

inline std::string read_string(iterator * s, iterator e){
	auto len = read_varint_size_t(s, e);
	auto p = skip(s, e, len);
	std::string str{p, *s};
	return str;
}

inline void write_field_varint(unsigned field_number, uint64_t v, std::string & s){
	write_varint((field_number << 3) | 0, s);
	write_varint(v, s);
}

//inline int64_t read_any_int(unsigned field_type, iterator * s, iterator e){
//	if(field_type == )
//	return 0;
//}
//inline uint64_t read_any_unsigned(unsigned field_type, iterator * s, iterator e){
//	return 0;
//}

//inline void write_field_fixed32(unsigned field_number, uint32_t v, std::string & s){
//	write_varint((field_number << 3) | 0, s);
//	write_varint(v, s);
//}

inline void write_field_string(unsigned field_number, const std::string & v, std::string & s){
	write_varint((field_number << 3) | 2, s);
	write_varint(v.size(), s);
	s += v;
}


inline void skip_by_type(unsigned field_type, iterator * s, iterator e){
	switch (field_type) {
		case 0:
			read_varint(s, e);
			break;
		case 1: // 64-bit fixed
			skip(s, e, 8);
			break;
		case 2:{
			auto len = read_varint_size_t(s, e);
			skip(s, e, len);
			break;
		}
		case 3: // start group
		case 4: // end group
			throw std::runtime_error("groups are not supported");
		case 5:
			skip(s, e, 4);
			break;
		default:
			break;
	}
}

template<typename T>
void read_message(T & v, iterator * s, iterator e) {
	auto len = read_varint_size_t(s, e);
	auto p = skip(s, e, len);
	read(v, p, *s);
}

}
