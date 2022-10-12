#pragma once

#include <string>
#include <map>
#include <vector>
#include <fstream>
#include <optional>
#include <format>
#include <format>
#include <tuple>
#include <iostream>

namespace json_utils {
	// ECMA-404 2nd edition / December 2017

	constexpr size_t default_buffer_size = 256;

	class json_tables {
	public:
		enum class json_tokens {
			LEFTBRACKET,
			LEFTBRACE,
			RIGHTBRACKET,
			RIGHTBRACE,
			COLON,
			COMMA,

			TRUE_TOKEN,
			FALSE_TOKEN,
			NULL_TOKEN,

			STRING_TOKEN,
			NUMBER_TOKEN,

			EOFTOKEN,

			UNKNOWN
		};

	protected:
		static std::map<json_tables::json_tokens, std::string> spelling_;
		static std::map<int, json_tables::json_tokens> char_lookup_;
		static std::map<std::string, json_tables::json_tokens> reserved_;
		static std::map<int, char> valid_escapes_;

	};

	class file_input_buffer {
	public:
		file_input_buffer(const std::string& file_name, size_t size = default_buffer_size)
			: size_(size)
			, source_(size + 1, '\0')
			, file_(file_name, std::ios::binary) {

			reset();
		}

		~file_input_buffer() {
			file_.close();
		}

		file_input_buffer(const file_input_buffer&) = delete;
		file_input_buffer& operator=(const file_input_buffer&) = delete;

		bool empty() const { return (source_[pos_] == '\0'); }

		int pos() { return pos_; }

		int next_char() {
			if (empty()) {
				reset();
				if (!file_.read(source_.data(), size_)) { // eof or count < size
					size_t count = static_cast<size_t>(file_.gcount());
					if (count < size_) {
						source_[count] = -1;
						source_[count + 1] = 0;
					}
				}
			}

			return source_[pos_++];
		}

		void restart() {
			file_.seekg(0, std::ios::beg);
			reset();
		}

	private:
		void reset() {
			pos_ = 0;
			source_[0] = '\0';
		}

		size_t size_;
		int pos_{};

		std::string source_;
		std::ifstream file_{};
	};

	class string_input_buffer {
	public:
		string_input_buffer(const std::string& source, size_t)
			: source_(source)
			, pos_{ 0 } {

			source_.push_back(-1);
			source_.push_back(0);
		}

		~string_input_buffer() {
		}

		string_input_buffer(const string_input_buffer&) = delete;
		string_input_buffer& operator=(const string_input_buffer&) = delete;

		bool empty() const { return (source_[pos_] == '\0'); }

		int pos() { return pos_; }

		int next_char() {
			if (empty())
				return std::char_traits<char>::eof();

			return source_[pos_++];
		}

		void restart() {
			pos_ = 0;
		}

	private:
		int pos_{};

		std::string source_;
	};

	template <typename BASE>
	class json_lexer : public BASE, public json_tables {
	public:
		json_lexer(const std::string& file_name, size_t size = default_buffer_size);
		~json_lexer();

		json_lexer(const json_lexer&) = delete;
		json_lexer& operator=(const json_lexer&) = delete;

		json_tables::json_tokens next_token() { token_ = scan(); return token_; }
		json_tables::json_tokens token() { return token_; }
		std::string value() const { return lex_val_; }

		static std::string spelling(json_tables::json_tokens tok) {
			return spelling_[tok];
		}

		size_t lineno() const {
			return lineno_;
		}

		void restart() {
			error_cout_ = 0;
			BASE::restart();
			lookahead_ = BASE::next_char();
			next_token();
		}

		int error_cout() const {
			return error_cout_;
		}

	private:

		void skip_ws();
		json_tables::json_tokens scan();

		int lookahead_{ std::char_traits<char>::eof() };
		std::string lex_val_;
		json_tables::json_tokens token_;
		size_t lineno_{};

		int error_cout_{};
	};

	template <typename BASE>
	json_lexer<BASE>::json_lexer(const std::string& file_name, size_t size)
		: BASE(file_name, size) {

		lookahead_ = BASE::next_char();
		next_token();
	}

	template <typename BASE>
	json_lexer<BASE>::~json_lexer() {

	}

	template <typename BASE>
	void json_lexer<BASE>::skip_ws() {
		while (std::isspace(lookahead_)) {

			if (lookahead_ == '\n')
				lineno_++;

			lookahead_ = BASE::next_char();
		}
	}

	template <typename BASE>
	json_tables::json_tokens json_lexer<BASE>::scan() {

		if (lookahead_ == std::char_traits<char>::eof())
			return json_tables::json_tokens::EOFTOKEN;

		skip_ws();

		if (std::isalpha(lookahead_)) { // true, false, null
			lex_val_.clear();
			do {
				lex_val_.push_back((char)lookahead_);
				lookahead_ = BASE::next_char();
			} while (std::isalpha(lookahead_));

			if (reserved_.contains(lex_val_))
				return reserved_[lex_val_];

			return json_tables::json_tokens::UNKNOWN;
		}

		if (lookahead_ == '"') { // string

			lex_val_.clear();
			lookahead_ = BASE::next_char();

			while (lookahead_ != '"') {
				if (lookahead_ < 0x1f) {
					// TODO: signal invalid char
					error_cout_++;
					lookahead_ = BASE::next_char(); // skip
				}
				else if (lookahead_ == '\\') { // esc sequence
					lookahead_ = BASE::next_char();

					if (lookahead_ == 'u') { // u

						unsigned int esc = 0;

						lookahead_ = BASE::next_char();

						for (auto i = 0; i < 4; i++) {
							if (std::isxdigit(lookahead_)) {

								if (lookahead_ <= '9')
									esc = esc * 16 + lookahead_ - '0';
								else if (lookahead_ <= 'F')
									esc = esc * 16 + lookahead_ - 'A' + 10;
								else
									esc = esc * 16 + lookahead_ - 'a' + 10;
							}
							else {
								// TODO: signal invalid hex digit
								error_cout_++;
								// skipped
							}

							lookahead_ = BASE::next_char();
						}

						// based on  http://en.wikipedia.org/wiki/UTF-8
						if (esc <= 0x7f) {
							lex_val_.push_back((char)esc);
						}
						else if (esc <= 0x7FF) {
							lex_val_.push_back((char)(0x80 | (0x3f & esc)));
							lex_val_.push_back((char)(0xC0 | (0x1f & (esc >> 6))));
						}
						else if (esc <= 0xFFFF) {
							lex_val_.push_back((char)(0x80 | (0x3f & esc)));
							lex_val_.push_back((char)(0x80 | (0x3f & (esc >> 6))));
							lex_val_.push_back((char)(0xE0 | (0xf & (esc >> 12))));
						}
						else if (esc <= 0x10FFFF) {
							lex_val_.push_back((char)(0x80 | (0x3f & esc)));
							lex_val_.push_back((char)(0x80 | (0x3f & (esc >> 6))));
							lex_val_.push_back((char)(0x80 | (0x3f & (esc >> 12))));
							lex_val_.push_back((char)(0xF0 | (0x7 & (esc >> 18))));
						}

					}
					else if (valid_escapes_.contains(lookahead_)) {
						lex_val_.push_back(valid_escapes_[lookahead_]);
						lookahead_ = BASE::next_char();
					}
					else {
						// TODO: signal invalid escape char
						error_cout_++;
						lookahead_ = BASE::next_char(); // skip
					}
				}
				else {
					lex_val_.push_back((char)lookahead_);	
					lookahead_ = BASE::next_char();
				}
			}

			lookahead_ = BASE::next_char();	// skip last ["]

			return json_tables::json_tokens::STRING_TOKEN;
		}

		if (std::isdigit(lookahead_) || lookahead_ == '-') { // number

			lex_val_.clear();

			if (lookahead_ == '-') {
				lex_val_.push_back((char)lookahead_);

				lookahead_ = BASE::next_char();
			}

			if (lookahead_ == '0') {
				lex_val_.push_back((char)lookahead_);

				lookahead_ = BASE::next_char();
			}
			else if (lookahead_ >= '1' && lookahead_ <= '9') {
				while (isdigit(lookahead_)) {
					lex_val_.push_back((char)lookahead_);

					lookahead_ = BASE::next_char();
				}
			}
			else {
				// TODO: signal malformed number
				error_cout_++;
				lookahead_ = BASE::next_char(); // skip
			}

			if (lookahead_ == '.') {
				lex_val_.push_back((char)lookahead_);

				lookahead_ = BASE::next_char();
				while (isdigit(lookahead_)) {
					lex_val_.push_back((char)lookahead_);

					lookahead_ = BASE::next_char();
				}
			}

			if (lookahead_ == 'e' || lookahead_ == 'E') {
				lex_val_.push_back((char)lookahead_);

				lookahead_ = BASE::next_char();

				if (lookahead_ == '-' || lookahead_ == '+') {
					lex_val_.push_back((char)lookahead_);

					lookahead_ = BASE::next_char();
				}

				while (isdigit(lookahead_)) {
					lex_val_.push_back((char)lookahead_);

					lookahead_ = BASE::next_char();
				}
			}

			return json_tables::json_tokens::NUMBER_TOKEN;
		}

		if (char_lookup_.contains(lookahead_)) // reserved tokens
		{
			auto tok = char_lookup_[lookahead_];

			lex_val_ = (char)lookahead_;
			lookahead_ = BASE::next_char();

			return tok;
		}

		lex_val_ = (char)lookahead_;

		return json_tables::json_tokens::UNKNOWN;
	}

	enum class json_value_type {
		JSON_OBJECT,
		JSON_ARRAY,
		JSON_NUMBER,
		JSON_STRING,
		JSON_BOOLEAN,
		JSON_NULL
	};

	class json_value {
	public:
		using obj_impl_type = std::map<std::string, json_value>;
		using arr_impl_type = std::vector<json_value>;

		json_value()
			: type_(json_value_type::JSON_NULL) {
		}

		json_value_type type() const {
			return type_;
		}

		// --------------------------------------------------------
		// null
		bool is_null() const {
			return type_ == json_value_type::JSON_NULL;
		}
		
		void nullify();

		// --------------------------------------------------------
		// object
		explicit json_value(const obj_impl_type& values)
			: type_(json_value_type::JSON_OBJECT) {
			object_impl = values;
		}

		bool is_object() const {
			return type_ == json_value_type::JSON_OBJECT && object_impl.has_value();
		}

		std::vector<std::string> keys() const;

		const obj_impl_type& get_object() const {
			if (!is_object())
				throw;

			return object_impl.value();
		}

		const json_value& operator[](const std::string& name) const {
			if (!is_object())
				throw;

			auto it = object_impl.value().find(name);
			if (!contains_key(name))
				throw;

			return object_impl.value().at(name);
		}

		json_value& operator[](const std::string& name) {
			if (!contains_key(name))
				throw;

			return object_impl.value().at(name);
		}

		bool contains_key(const std::string& name) const {
			if (!is_object())
				return false;

			return object_impl.value().contains(name);
		}

		void add_pair(const std::string& name, const json_value& value) {
			if (!is_assignable_from(json_value_type::JSON_OBJECT))
				throw;

			object_impl.value()[name] = value;
		}

		template <typename T>
		void add_pair(const std::string& name, const T& value) {
			add_pair(name, json_value{ value });
		}

		// --------------------------------------------------------
		// array
		explicit json_value(const arr_impl_type& values)
			: type_(json_value_type::JSON_ARRAY) {
			array_impl = values;
		}

		bool is_array() const {
			return type_ == json_value_type::JSON_ARRAY && array_impl.has_value();
		}

		const arr_impl_type& get_array() const {
			if (!is_array())
				throw;

			return array_impl.value();
		}

		size_t get_array_size() const {
			if (!is_array() || !array_impl.has_value())
				return 0;

			return array_impl.value().size();
		}

		json_value& operator[](unsigned int index) {
			if (!is_array() || index >= array_impl.value().size())
				throw;

			return array_impl.value()[index];
		}

		const json_value& operator[](unsigned int index) const {
			if (!is_array() || index >= array_impl.value().size())
				return {};

			return array_impl.value()[index];
		}

		void append_element(const json_value& value) {
			if (!is_assignable_from(json_value_type::JSON_ARRAY))
				throw;

			array_impl.value().push_back(value);
		}

		template <typename T>
		void append_element(const T& value) {
			append_element(json_value{ value });
		}

		// --------------------------------------------------------
		// number
		explicit json_value(double value)
			: type_(json_value_type::JSON_NUMBER) {

			number_impl = value;
		}

		explicit json_value(int value)
			: type_(json_value_type::JSON_NUMBER) {

			number_impl = (double)value;
		}

		bool is_number() const {
			return type_ == json_value_type::JSON_NUMBER;
		}

		void operator=(double d) {
			if (!is_assignable_from(json_value_type::JSON_NUMBER))
				throw;

			number_impl = d;
		}

		void operator=(int d) {
			if (!is_assignable_from(json_value_type::JSON_NUMBER))
				throw;

			number_impl = (double)d;
		}

		double num_val() const {
			if (!is_number())
				throw;

			return number_impl.value();
		}

		// --------------------------------------------------------
		// string
		explicit json_value(const std::string& value)
			: type_(json_value_type::JSON_STRING) {

			string_impl = value;
		}

		explicit json_value(const char* value)
			: type_(json_value_type::JSON_STRING) {

			string_impl = std::string{ value };
		}

		bool is_string() const {
			return type_ == json_value_type::JSON_STRING;
		}

		void operator=(const std::string& s) {
			if (!is_assignable_from(json_value_type::JSON_STRING))
				throw;

			string_impl = s;
		}

		void operator=(const char* s) {
			if (!is_assignable_from(json_value_type::JSON_STRING))
				throw;

			string_impl = s;
		}

		std::string str_val() const {
			if (!is_string())
				throw;

			return string_impl.value();
		}

		// --------------------------------------------------------
		// boolean
		explicit json_value(bool value)
			: type_(json_value_type::JSON_BOOLEAN) {

			boolean_impl = value;
		}

		bool is_boolean() const {
			return type_ == json_value_type::JSON_BOOLEAN;
		}

		void operator=(bool b) {
			if (!is_assignable_from(json_value_type::JSON_BOOLEAN))
				throw;

			boolean_impl = b;
		}

		double bool_val() const {
			if (!is_boolean())
				throw;

			return boolean_impl.value();
		}

		// --------------------------------------------------------

		friend std::ostream& operator<<(std::ostream& os, const json_value& value);

		bool save_to(const std::string& filename);

	private:
		bool is_assignable_from(json_value_type t);

		static void write_escaped(std::ostream& out, const std::string& s);
		std::ostream& print(std::ostream& os, const std::string& whitespace = "") const;

		std::optional<obj_impl_type> object_impl;
		std::optional<arr_impl_type> array_impl;
		std::optional<double> number_impl;
		std::optional<std::string> string_impl;
		std::optional<bool> boolean_impl;

		json_value_type type_{ json_value_type::JSON_NULL };
	};

	template <typename LEXER>
	class json_parser {
	public:

		json_parser(LEXER& lexer) : lexer_(lexer) {

		}

		bool json(json_value& value) {

			error_cout_ = 0;
			value.nullify();

			if (lexer_.pos() != 0) {
				lexer_.restart();
			}
			value = jvalue();

			return (lexer_.token() == json_tables::json_tokens::EOFTOKEN);
		}

		json_value jvalue() {
			switch (lexer_.token()) {
			case json_tables::json_tokens::LEFTBRACE:
				return jobject();
			case json_tables::json_tokens::LEFTBRACKET:
				return jarray();
			case json_tables::json_tokens::NUMBER_TOKEN:
				return jnumber();
			case json_tables::json_tokens::STRING_TOKEN:
				return jstring();
			case json_tables::json_tokens::TRUE_TOKEN:
				return jtrue();
			case json_tables::json_tokens::FALSE_TOKEN:
				return jfalse();
			case json_tables::json_tokens::NULL_TOKEN:
				return jnull();
			default:
				parse_error(" unexpected token " + lexer_.spelling(lexer_.token()));

				return json_value();
			}
		}

		json_value jnumber() {
			double value = std::stod(lexer_.value());

			lexer_.next_token();

			return json_value{ value };
		}

		json_value jstring() {
			std::string value = lexer_.value();

			lexer_.next_token();

			return json_value{ value };
		}

		json_value jtrue() {
			lexer_.next_token();

			return json_value{ true };
		}

		json_value jfalse() {
			lexer_.next_token();

			return json_value{ false };
		}

		json_value jnull() {
			lexer_.next_token();

			return json_value();
		}

		json_value jobject() {
			expect(json_tables::json_tokens::LEFTBRACE);

			json_value::obj_impl_type values{};

			if (lexer_.token() == json_tables::json_tokens::RIGHTBRACE) {
				lexer_.next_token();

				return json_value{ values }; // empty
			}

			auto [name, value] = jpair();
			values[name] = value;

			while (lexer_.token() == json_tables::json_tokens::COMMA) {

				lexer_.next_token();
				std::tie(name, value) = jpair();
				values[name] = value;
			}

			expect(json_tables::json_tokens::RIGHTBRACE);

			return json_value{ values };
		}

		json_value jarray() {
			expect(json_tables::json_tokens::LEFTBRACKET);

			json_value::arr_impl_type values{};

			if (lexer_.token() == json_tables::json_tokens::RIGHTBRACKET) {
				lexer_.next_token();

				return json_value{ values }; // empty
			}

			values.push_back(jvalue());

			while (lexer_.token() == json_tables::json_tokens::COMMA) {
				lexer_.next_token();

				values.push_back(jvalue());
			}

			expect(json_tables::json_tokens::RIGHTBRACKET);

			return json_value{ values };
		}

		std::tuple<std::string, json_value> jpair() {

			std::string name = lexer_.value();

			expect(json_tables::json_tokens::STRING_TOKEN);
			expect(json_tables::json_tokens::COLON);

			return { name, jvalue() };
		}

		int error_cout() const {
			return error_cout_;
		}

	private:
		void must_be(json_tables::json_tokens tok) {
			if (lexer_.token() != tok)
				parse_error(" expected " + lexer_.spelling(tok));
		}

		void expect(json_tables::json_tokens tok) {
			must_be(tok);
			lexer_.next_token();
		}

		void parse_error(const std::string& message) {
			error_cout_++;

			std::cout << "Error: line " << lexer_.lineno() << " " << message << std::endl;;
		}
		
		int error_cout_{};

		int indent_{};

		LEXER& lexer_;
	};

	using json_file_lexer = json_lexer<file_input_buffer>;
	using json_string_lexer = json_lexer<string_input_buffer>;

	using json_file_parser = json_parser<json_file_lexer>;
	using json_string_parser = json_parser<json_string_lexer>;

	bool parse_string(const std::string& source, json_value& value);
	bool parse_file(const std::string& filename, json_value& value);
}