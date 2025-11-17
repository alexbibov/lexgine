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
private:
	static const char s_token_separators[];
	static const char s_separating_tokens[];
public:
	CommandRegistry();
	void addNamespace(std::string const& namespace_name, std::string const& namespace_summary);
	void addCommand(CommandSpec const& command_spec, CommandExecFn const& command_op);
	void addCommand(std::string const& namespace_name, CommandSpec const& command_spec, CommandExecFn const& command_op);
	void setQuery(std::string const& query);
	void append(char c);
	void backspace();
	std::vector<std::string> autocompleteSuggestions(uint16_t max_allowed_distance) const;
	std::optional<CommandExecutionSchema> invokeQuery() const;

private:
	struct NamespaceSpec
	{
		std::string name;
		std::string summary;
		ConsoleTokenAutocomplete autocompleter;  //!< context autocompleter for command names completion
		std::unordered_map<std::string, Command> commands;

		NamespaceSpec(std::string const& namespace_name, std::string const& namespace_summary)
			: name{ namespace_name }
			, summary{ namespace_summary }
		{

		}
		std::pair<std::string_view, uint16_t> findMostLikelyCommandName(std::string_view const& requested_command_id) const;
	};

	enum class TokenContext
	{
		_namespace,
		command,
		argument_name,
		argument_value,
		_operator,
		expecting_new_token,
		unknown
	};

	struct QueryContext
	{
		std::variant<std::monostate, NamespaceSpec*, CommandSpec*, ArgSpec*> spec;
		TokenContext context;
	};


private:
	static std::vector<std::string_view> tokenize(std::string_view const& s);
	static std::vector<std::pair<std::string_view, TokenContext>> parseTokensContext(std::vector<std::string_view> const& tokens);
	std::optional<CommandExecutionSchema> parse(std::vector<std::pair<std::string_view, TokenContext>> const& contextualized_tokens) const;

	std::pair<std::string_view, uint16_t> findMostLikelyNamespaceName(std::string_view const& requested_namespace_id) const;

private:
	std::unordered_map<std::string, NamespaceSpec> m_namespaces;
	ConsoleTokenAutocomplete m_autocompleter;  //!< context autocompleter for namespace names completion
	std::string m_current_query;
	std::vector<std::string_view> m_query_tokens;
	std::vector<std::pair<std::string_view, TokenContext>> m_contextualized_query_tokens;
	std::stack<QueryContext> m_current_context;
};

}

#endif