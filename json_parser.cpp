#include "json_parser.h"
#include <iomanip>


namespace json_utils {

	std::map<json_tables::json_tokens, std::string> json_tables::spelling_{
	{ json_tables::json_tokens::LEFTBRACKET, "[" },
	{ json_tables::json_tokens::LEFTBRACE, "{" },
	{ json_tables::json_tokens::RIGHTBRACKET, "]" },
	{ json_tables::json_tokens::RIGHTBRACE,"}" },
	{ json_tables::json_tokens::COLON, ":" },
	{ json_tables::json_tokens::COMMA, "," },

	{ json_tables::json_tokens::TRUE_TOKEN, "true" },
	{ json_tables::json_tokens::FALSE_TOKEN, "false" },
	{ json_tables::json_tokens::NULL_TOKEN, "null" },

	{ json_tables::json_tokens::STRING_TOKEN, "<string>" },
	{ json_tables::json_tokens::NUMBER_TOKEN, "<jnumber" },


	{ json_tables::json_tokens::EOFTOKEN, "<eof>" },

	{ json_tables::json_tokens::UNKNOWN, "??" }
	};

	std::map<int, json_tables::json_tokens> json_tables::char_lookup_
	{
		{ '[', json_tables::json_tokens::LEFTBRACKET },
		{ '{', json_tables::json_tokens::LEFTBRACE },
		{ ']', json_tables::json_tokens::RIGHTBRACKET },
		{ '}', json_tables::json_tokens::RIGHTBRACE },
		{ ':', json_tables::json_tokens::COLON },
		{ ',', json_tables::json_tokens::COMMA },

		{ std::char_traits<char>::eof(), json_tables::json_tokens::EOFTOKEN }
	};

	std::map<std::string, json_tables::json_tokens> json_tables::reserved_{
		{ "true",	json_tables::json_tokens::TRUE_TOKEN },
		{ "false",	json_tables::json_tokens::FALSE_TOKEN },
		{ "null",	json_tables::json_tokens::NULL_TOKEN }
	};

	std::map<int, char> json_tables::valid_escapes_{
		{ '"',	'"' },
		{ '\\',	'\\' },
		{ '/',	'/' },
		{ 'b',	'\b' },
		{ 'f',	'\f' },
		{ 'n',	'\n' },
		{ 'r',	'\r' },
		{ 't',	'\t' }
	};
	
	void json_value::write_escaped(std::ostream& out, const std::string& s) {
		out << '"';
		for (auto c = s.cbegin(); c != s.cend(); c++) {
			switch (*c) {
			case '"': 
				out << "\\\""; 
				break;
			case '\\':
				out << "\\\\";
				break;
			case '/':
				out << "\\/";
				break;
			case '\b': 
				out << "\\b"; 
				break;
			case '\f': 
				out << "\\f"; 
				break;
			case '\n': 
				out << "\\n"; 
				break;
			case '\r': 
				out << "\\r"; 
				break;
			case '\t': 
				out << "\\t"; 
				break;
			default:
				if ('\x00' <= *c && *c <= '\x1f') 
					out << "\\u" << std::hex << std::setw(4) << std::setfill('0') << static_cast<int>(*c);
				else 
					out << *c;
			}
		}

		out << '"';
	}

	std::vector<std::string> json_value::keys() const {

		std::vector<std::string> result;

		if (!object_impl.has_value())
			return result;

		auto values = object_impl.value();
		for (auto& [key, value] : values) {
			result.push_back(key);
		}

		return result;
	}
	
	bool json_value::is_assignable_from(json_value_type t) {
		if (type_ == json_value_type::JSON_NULL) {
			type_ = t;
			switch (type_) {
			case json_value_type::JSON_OBJECT:
				object_impl = obj_impl_type{};
				break;

			case json_value_type::JSON_ARRAY:
				array_impl = arr_impl_type{};
				break;

			case json_value_type::JSON_NUMBER:
				number_impl = 0.0;
				break;

			case json_value_type::JSON_STRING:
				string_impl = "";
				break;

			case json_value_type::JSON_BOOLEAN:
				boolean_impl = false;
				break;

			default:
				throw;
			}
		}

		return type_ == t;
	}

	std::ostream& json_value::print(std::ostream& os, const std::string& whitespace) const {
		switch (type_) {
		case json_value_type::JSON_OBJECT:
		{
			os << "{\n";
			auto values = object_impl.value();
			size_t i = 0;
			for (auto& [key, value] : values) {
				os << whitespace << "  " << "\"" << key << "\": ";
				value.print(os, whitespace + "  ");
				if (i < values.size() - 1)
					os << ",";

				os << "\n";
				i++;
			}

			return os << whitespace << "}";
		}
		break;

		case json_value_type::JSON_ARRAY:
		{
			os << "[\n";
			auto a = array_impl.value();
			for (size_t i = 0; i < a.size(); i++) {
				auto value = a[i];
				os << whitespace << "  ";
				value.print(os, whitespace + "  ");
				if (i < a.size() - 1) {
					os << ",";
				}

				os << "\n";
			}
		}
		return os << whitespace << "]";

		case json_value_type::JSON_NUMBER:
		{
			double d = number_impl.value();
			if (d - std::floor(d) != 0.0)
				return os << std::format("{}", number_impl.value()); 

			return os << std::format("{}", (long long)d);
		}
		break;

		case json_value_type::JSON_STRING:
			write_escaped(os, string_impl.value());
			return os;

		case json_value_type::JSON_BOOLEAN:
			return os << (boolean_impl.value() ? "true" : "false");

		case json_value_type::JSON_NULL:
			return os << "null";

		default:
			return os;
		}
	}

	void json_value::nullify() {
		object_impl.reset();
		array_impl.reset();
		number_impl.reset();
		string_impl.reset();
		boolean_impl.reset();
		type_ = json_value_type::JSON_NULL;
	}

	bool json_value::save_to(const std::string& filename) {
		std::ofstream out(filename);
		if (!out)
			return false;
		out << *this;

		return out.good();
	}

	std::ostream& operator<<(std::ostream& os, const json_value& value) {
		return value.print(os);
	}

	bool parse_string(const std::string& source, json_value& value) {
		json_string_lexer slexer{ source };
		json_string_parser sparser{ slexer };

		return sparser.json(value);
	}

	bool parse_file(const std::string& filename, json_value& value) {
		json_file_lexer slexer{ filename };
		json_file_parser sparser{ slexer };

		return sparser.json(value);
	}
}