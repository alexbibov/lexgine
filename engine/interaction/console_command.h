#ifndef LEXGINE_INTERACTION_CONSOLE_CONSOLE_COMMAND_H
#define LEXGINE_INTERACTION_CONSOLE_CONSOLE_COMMAND_H

#include <variant>
#include <string>
#include <optional>
#include <vector>
#include <stack>
#include <unordered_map>
#include <functional>

#include "engine/core/entity.h"
#include "engine/core/class_names.h"
#include "console_command_autocomplete.h"

namespace lexgine::interaction::console
{

using ArgValue = std::variant<bool, int, float, std::string>;
using ArgMap = std::unordered_map<std::string, ArgValue>;

struct ParserOutput
{
	std::optional<ArgValue> value;
	std::string error_message;
};
using ArgParserFn = std::function<ParserOutput(std::string_view const& token)>;
namespace value_parsers
{
	extern ArgParserFn bool_parser;
	extern ArgParserFn int_parser;
	extern ArgParserFn float_parser;
	extern ArgParserFn string_parser;
}

struct ArgSpec
{
	std::string name;                  // Argument name
	std::string description;           // Argument description
	ArgParserFn parser;                // Argument value parser
	std::optional<ArgValue> _default;  // Default argument value
};

struct CommandSpec
{
	std::string name;
	std::string summary;
	std::vector<ArgSpec> args;
	ConsoleTokenAutocomplete autocompleter;  //!< context auto-completion for argument names when using name=value argument assignment syntax
	std::vector<std::string> autocompleteSuggestions(size_t max_count) const;
	std::pair<std::string_view, uint16_t> findMostLikelyArgName(std::string_view const& requested_arg_name) const;
	void initAutocompleter();
};

struct CommandExecResult
{
	bool succeeded;
	std::string msg;
};
using CommandExecFn = std::function<CommandExecResult(ArgMap const& args)>;

struct Command
{
	CommandSpec spec;
	CommandExecFn exec;
	CommandExecResult operator()(ArgMap const& args) const { return exec(args); }
};

struct CommandExecutionSchema
{
	CommandExecFn exec;
	ArgMap args;
};

class CommandRegistry : public core::NamedEntity<core::class_names::ConsoleCommandRegistry>
{
public:
	CommandRegistry();
	void addNamespace(std::string const& namespace_name, std::string const& namespace_summary);
	void addCommand(CommandSpec const& command_spec, CommandExecFn const& command_op);
	void addCommand(std::string const& namespace_name, CommandSpec const& command_spec, CommandExecFn const& command_op);
	void setQuery(std::string const& query);
	void append(char c);
	void backspace();
	std::string_view getCurrentQuery() const { return m_current_query; }
	std::vector<std::string> autocompleteSuggestions(size_t max_count) const;
	std::optional<CommandExecutionSchema> invokeQuery() const;

private:
	static char s_separators[];
	static char s_operators[];

private:
	struct LookupHasher
	{
		using is_transparent = int;

		[[nodiscard]] size_t operator()(std::string_view const& value) const
		{
			return std::hash<std::string_view>{}(value);
		}

		[[nodiscard]] size_t operator()(char const* value) const
		{
			return std::hash<std::string_view>{}(value);
		}
	};

	struct NamespaceSpec
	{
		std::string name;
		std::string summary;
		ConsoleTokenAutocomplete autocompleter;  //!< context autocompleter for command names completion
		std::unordered_map<std::string, Command, LookupHasher, std::equal_to<>> commands;

		NamespaceSpec(std::string const& namespace_name, std::string const& namespace_summary)
			: name{ namespace_name }
			, summary{ namespace_summary }
		{

		}
		std::vector<std::string> autocompleteSuggestions(size_t max_count) const;
		std::pair<std::string_view, uint16_t> findMostLikelyCommandName(std::string_view const& requested_command_name) const;
	};

	enum class TokenTag
	{
		operation,
		idendifier,
		value,
		unknown
	};

	struct Token
	{
		size_t start_index{ };
		size_t count{ 0 };
		TokenTag tag{ TokenTag::unknown };
		operator bool() const
		{
			return count > 0 && tag != TokenTag::unknown;
		}
	};

	using QueryContext = std::variant<NamespaceSpec*, Command*>;

private:
	std::string_view extractToken(Token const& token) const;
	void tokenize(size_t start_position);
	Token extractCommandIdToken() const;
	Token extractCommandNameTokenFromCommandIdToken(Token const& command_id) const;
	Token extractNamespaceNameTokenFromCommandIdToken(Token const& command_id) const;
	std::optional<CommandExecutionSchema> parse() const;
	static bool isSeparator(char c);
	static bool isOperator(char c);
	bool isOperator(Token const& t) const;
	std::pair<std::string_view, uint16_t> findMostLikelyNamespaceName(std::string_view const& requested_namespace_name) const;

private:
	std::unordered_map<std::string, NamespaceSpec, LookupHasher, std::equal_to<>> m_namespaces;
	ConsoleTokenAutocomplete m_autocompleter;  //!< context autocompleter for namespace names completion
	std::string m_current_query;
	std::vector<Token> m_query_tokens;
	std::stack<QueryContext> m_current_autompletion_context;
};

}

#endif