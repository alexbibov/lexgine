#include <algorithm>
#include <stdexcept>
#include <unordered_set>
#include <queue>
#include "console_command_autocomplete.h"

#include "console_command.h"


namespace lexgine::interaction::console
{

namespace value_parsers
{

ArgParserFn bool_parser = 
[](std::string_view const& token) -> ParserOutput
	{
		std::string s{ token };
		std::transform(s.begin(), s.end(), s.begin(),
			[](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
		if (s == "true" || s == "1" || s == "on" || s == "yes")  return { true, "" };
		if (s == "false" || s == "0" || s == "off" || s == "no") return { false, "" };
		return { std::nullopt, "expected boolean value" };
	};

ArgParserFn int_parser =
[](std::string_view const& token) -> ParserOutput
	{
		try
		{
			size_t pos{ 0 };
			std::string s{ token };
			int value = std::stoi(s, &pos);
			if (pos != s.size())
			{
				throw std::invalid_argument{ "integer has unknown tail" };
			}
			return { value, "" };
		}
		catch (std::out_of_range const&)
		{
			return { std::nullopt, "out of range value" };
		}
		catch (std::invalid_argument const&)
		{
			return { std::nullopt, "expected integer value" };
		}
		catch (std::exception const& e)
		{
			return { std::nullopt, e.what() };
		}
	};

ArgParserFn float_parser =
[](std::string_view const& token) -> ParserOutput
	{
		try
		{
			size_t pos{ 0 };
			std::string s{ token };
			float value = std::stof(s, &pos);
			if (pos != s.size())
			{
				throw std::invalid_argument{ "floating point unknown tail" };
			}
			return { value, "" };
		}
		catch (std::out_of_range const&)
		{
			return { std::nullopt, "out of range value" };
		}
		catch (std::invalid_argument const&)
		{
			return { std::nullopt, "expected floating point value" };
		}
		catch (std::exception const& e)
		{
			return { std::nullopt, e.what() };
		}
	};

ArgParserFn string_parser =
[](std::string_view const& token) -> ParserOutput
	{
		return { std::string{token}, "" };
	};

}

std::pair<std::string_view, uint16_t> CommandSpec::findMostLikelyArgName(std::string_view const& requested_arg_name) const
{
	uint16_t d = requested_arg_name.length();
	std::string_view suggestion{};
	for (auto const& arg_spec : args)
	{
		uint16_t new_d = ConsoleTokenAutocomplete::computeDLDistance(requested_arg_name, arg_spec.name);
		if (new_d < d)
		{
			suggestion = arg_spec.name;
			d = new_d;
		}
	}
	return { suggestion, d };
}

void CommandSpec::initAutocompleter()
{
	autocompleter.clearTokenPool();
	for (const ArgSpec& arg_spec : args)
	{
		autocompleter.addToken(arg_spec.name);
	}
}

CommandRegistry::CommandRegistry()
{
	addNamespace("global", "global namespace");
}

void CommandRegistry::addNamespace(std::string const& namespace_name, std::string const& namespace_summary)
{
	if (m_namespaces.count(namespace_name))
	{
		LEXGINE_LOG_ERROR(this, std::format("Attempt to register namespace '{}', which already exists", namespace_name));
	}
	m_namespaces.emplace(
		std::piecewise_construct, 
		std::forward_as_tuple(namespace_name), 
		std::forward_as_tuple(namespace_name, namespace_summary)
	);
	m_autocompleter.addToken(namespace_name);
}

void CommandRegistry::addCommand(CommandSpec const& command_spec, CommandExecFn const& command_op)
{
	auto& namespace_spec = m_namespaces.at("global");
	namespace_spec.commands[command_spec.name] = {.spec = command_spec, .exec = command_op};
	namespace_spec.autocompleter.addToken(command_spec.name);
}

void CommandRegistry::addCommand(std::string const& namespace_name, CommandSpec const& command_spec, CommandExecFn const& command_op)
{
	if (m_namespaces.count(namespace_name) == 0)
	{
		LEXGINE_LOG_ERROR(this, std::format("Attempt to add command '{}' to non-existing namespace '{}'",
			command_spec.name, namespace_name));
		return;
	}
	if (m_namespaces.at(namespace_name).commands.count(command_spec.name))
	{
		LEXGINE_LOG_ERROR(this, std::format("Error registering command {} to namespace {}: the command with this name already exists in the namespace",
			command_spec.name, namespace_name));
		return;
	}
	auto& namespace_spec = m_namespaces.at(namespace_name);
	namespace_spec.commands[command_spec.name] = { .spec = command_spec, .exec = command_op };
	namespace_spec.autocompleter.addToken(command_spec.name);
}

void CommandRegistry::setQuery(std::string const& query)
{
	m_current_query = query;
	tokenize();
	if (!m_query_tokens.empty())
	{
		auto [namespace_token, command_token_position_hint] = extractNamespaceToken();
		auto [command_token, command_token_position] = extractCommandToken(namespace_token, command_token_position_hint);

		auto namespace_iter = namespace_token.count > 0 ? m_namespaces.find(extractToken(namespace_token)) : m_namespaces.find("global");
		if (command_token.count > 0)
		{
			std::string_view command_id = extractToken(command_token);
			if (namespace_iter != m_namespaces.end())
			{
				NamespaceSpec const& namespace_spec = namespace_iter->second;

				auto command_iter = namespace_spec.commands.find(command_id);
				if (command_iter != namespace_spec.commands.end())
				{
					// Found command and namespace, auto-complete command arguments
					Token const& id = m_query_tokens.back();
					std::string_view current_input = id.tag == TokenTag::idendifier && id.start_index > command_token.start_index ? extractToken(id) : std::string_view{};
					m_autocompletion_context = { .autocompleter = command_iter->second.spec.autocompleter, .current_input = current_input };
				}
				else
				{
					// Found namespace, but cannot find command. Suggest command names
					m_autocompletion_context = { .autocompleter = namespace_iter->second.autocompleter, .current_input = command_id };
				}
			}
			else
			{
				// Unable to find namespace, but already started entering command identifier. Suggestion context is undefined
				m_autocompletion_context = std::nullopt;
				return;
			}
		}
		else
		{
			// No command id available yet
			if (namespace_iter != m_namespaces.end())
			{
				// Found namespace identifier
				m_autocompletion_context = { .autocompleter = namespace_iter->second.autocompleter, .current_input = std::string_view{} };
			}
			else
			{
				m_autocompletion_context = { .autocompleter = m_autocompleter, .current_input = extractToken(namespace_token) };
			}
		}
	}
}

std::vector<std::string> CommandRegistry::autocompleteSuggestions(uint16_t max_allowed_distance) const
{
	if (m_autocompletion_context.has_value())
	{
		if (m_autocompletion_context->current_input.empty())
		{
			return m_autocompletion_context->autocompleter.allTokens();
		}

		ConsoleTokenAutocomplete& autocompleter = m_autocompletion_context->autocompleter;
		autocompleter.setQuery(m_autocompletion_context->current_input);
		std::vector<std::pair<std::string, uint16_t>> weighted_suggestions = autocompleter.suggestions();
		std::vector<std::string> rv{};
		for (auto p = weighted_suggestions.begin(); p != weighted_suggestions.end() && p->second <= max_allowed_distance; ++p)
		{
			rv.push_back(p->first);
		}
		return rv;
	}
	return {};
}

std::optional<CommandExecutionSchema> CommandRegistry::invokeQuery() const
{
	return parse();
}

char CommandRegistry::s_separators[] = { ' ', '\t', '\r', '\n' };
std::string CommandRegistry::s_dereference = ".";
std::string CommandRegistry::s_assignment = "=";
std::string CommandRegistry::s_comma = ",";
std::string CommandRegistry::s_operators[] = { CommandRegistry::s_dereference, CommandRegistry::s_assignment, CommandRegistry::s_comma };

std::pair<std::string_view, uint16_t> CommandRegistry::NamespaceSpec::findMostLikelyCommandName(std::string_view const& requested_command_id) const
{
	uint16_t d = requested_command_id.length();
	std::string_view suggestion{};
	for (auto [name, spec] : commands)
	{
		uint16_t new_d = ConsoleTokenAutocomplete::computeDLDistance(requested_command_id, name);
		if (new_d < d)
		{
			suggestion = name;
			d = new_d;
		}
	}
	return { suggestion, d };
}

std::string_view CommandRegistry::extractToken(Token const& token) const
{
	return { m_current_query.data() + token.start_index, token.count };
}

void CommandRegistry::tokenize()
{
	m_query_tokens.clear();
	Token current_token{.start_index = 0, .count = 0};
	auto is_operator = [](std::string const& str, size_t start_position) -> int
		{
			for (std::string& op : s_operators)
			{
				if (start_position + op.length() <= str.size()
					&& op == std::string_view{ str.data() + start_position, op.length() })
				{
					return op.length();
				}
			}
			return -1;
		};

	{
		size_t i = 0;
		while (i < m_current_query.size())
		{
			if (isSeparator(m_current_query[i]))
			{
				if (current_token.count)
				{
					m_query_tokens.push_back(current_token);
					current_token = { .start_index = i + 1 };
				}
				else
				{
					++current_token.start_index;
				}
				++i;
				continue;
			}
			{
				int operator_token_length = is_operator(m_current_query, i);
				if (operator_token_length != -1)
				{
					if (current_token.count)
					{
						m_query_tokens.push_back(current_token);
						current_token = { .start_index = i + operator_token_length };
					}
					else
					{
						current_token.start_index += operator_token_length;
					}
					m_query_tokens.push_back({ .start_index = i, .count = static_cast<size_t>(operator_token_length) });
					i += operator_token_length;
					continue;
				}
			}
			++i;
			++current_token.count;
		}
		if (current_token.count)
		{
			m_query_tokens.push_back(current_token);
		}
	}
	
	for (size_t i = 0; i < m_query_tokens.size(); ++i)
	{
		Token& t = m_query_tokens[i];
		if (isOperatorToken(t))
		{
			t.tag = TokenTag::operation;
		}
	}
}

std::pair<CommandRegistry::Token, size_t> CommandRegistry::extractNamespaceToken() const
{
	Token namespace_token{ .start_index = 0 };
	size_t c = 0;
	if (extractToken(m_query_tokens[0]) == s_dereference)
	{
		namespace_token.start_index = 1;
		c = 1;
	}
	while (c + 1 < m_query_tokens.size()
		&& m_query_tokens[c].tag == TokenTag::idendifier
		&& extractToken(m_query_tokens[c + 1]) == s_dereference)
	{
		namespace_token.count += m_query_tokens[c].start_index + m_query_tokens[c].count - namespace_token.start_index;
		c += 2;
	}
	return { namespace_token, c };
}

std::pair<CommandRegistry::Token, size_t> CommandRegistry::extractCommandToken(Token const& namespace_token, size_t token_position_hint) const
{
	size_t start_index_hint = namespace_token.start_index + namespace_token.count;
	auto p = std::find_if(
		m_query_tokens.begin() + token_position_hint,
		m_query_tokens.end(),
		[start_index_hint](Token const& t)
		{
			return t.start_index >= start_index_hint && t.tag == TokenTag::idendifier;
		}
	);
	return p != m_query_tokens.end() 
		? std::pair{ *p, static_cast<size_t>(p - m_query_tokens.begin()) } 
	    : std::pair{ Token{}, token_position_hint };
}

std::optional<CommandExecutionSchema> CommandRegistry::parse() const
{
	// Short grammar description
	//
	// [namespace. | .]command value [, value]+ [, name = value]* 
	// namespace ::= ^[A-Za-z_]+\d*(?:\.[A-Za-z_]+\d*)*$
	// command ::= valid_identifier
	// name ::= valid_identifier
	// value ::= boolean_value | integer_value | fp_value | string
	// 
	// valid_identifier := ^[A-Za-z_]+\d*
	// boolean_value ::= true | false | 0 | 1 | on | off | yes | no (not case-sensitive where applies)
	// integer_value ::= \d+
	// fp_value ::= ^[+-]?(?:\d+\.\d*|\.\d+)(?:[eE][+-]?\d+)?[fF]?$
	// string ::= <ascii string>

	if (m_query_tokens.empty())
	{
		return std::nullopt;
	}

	auto [namespace_token, command_token_position_hint] = extractNamespaceToken();
	std::string namespace_id = "global";
	if (namespace_token.count > 0)
	{
		namespace_id = extractToken(namespace_token);
	}

	auto namespace_spec_it = m_namespaces.find(namespace_id);
	if (namespace_spec_it == m_namespaces.end())
	{
		auto [suggestion, distance] = findMostLikelyNamespaceName(namespace_id);
		std::string err_msg = distance <= 3
			? std::format("Invalid namespace identifier '{}'. Namespace is not defined. Did you mean {}?", namespace_id, suggestion)
			: std::format("Invalid namespace identifier '{}'. Namespace is not defined.", namespace_id);
		LEXGINE_LOG_ERROR(this, err_msg);
		return std::nullopt;
	}
	NamespaceSpec const& namespace_spec = namespace_spec_it->second;
	
	// Try fetching target command
	auto [command_token, command_token_position] = extractCommandToken(namespace_token, command_token_position_hint);
	if (command_token.count == 0)
	{
		LEXGINE_LOG_ERROR(this,
			std::format("No command id specifier after namespace '{}'", namespace_id)
		);
		return std::nullopt;
	}

	size_t c = command_token_position + 1;
	std::string_view command_id = extractToken(command_token);
	auto command_iter = namespace_spec.commands.find(command_id);
	if (command_iter == namespace_spec.commands.end())
	{
		auto [suggestion, distance] = namespace_spec.findMostLikelyCommandName(command_id);
		std::string err_msg = distance <= 3
			? std::format("Command with identifier '{}' is not found in namespace '{}'. Did you mean {}?",
				command_id, namespace_id, suggestion)
			: std::format("Command with identifier '{}' is not found in namespace '{}'.",
				command_id, namespace_id);
		LEXGINE_LOG_ERROR(this, err_msg);
		return std::nullopt;
	}
	Command const& command = command_iter->second;

	CommandExecutionSchema rv{ .exec = command.exec };
	CommandSpec const& command_spec = command.spec;
	std::vector<ArgSpec> const& command_args = command_spec.args;
	size_t command_arg_count = command_args.size();
	std::unordered_map<std::string_view, size_t> arg_names_lut{}; arg_names_lut.reserve(command_arg_count);
	for (size_t i = 0; i < command_arg_count; ++i)
	{
		const ArgSpec& arg_spec = command_args[i];
		arg_names_lut[arg_spec.name] = i;
	}
	std::vector<bool> set_arguments(command_arg_count, false);
	{
		// parse command arguments

		// parse value arguments
		size_t command_argument_counter = 0;
		while (c < m_query_tokens.size() && m_query_tokens[c].tag == TokenTag::idendifier)
		{
			// Look-ahead to check if we began parsing named arguments
			if (c + 1 < m_query_tokens.size() && extractToken(m_query_tokens[c + 1]) == s_assignment)
			{
				break;
			}
			if (command_argument_counter == arg_names_lut.size())
			{
				LEXGINE_LOG_ERROR(
					this,
					std::format("Unable to invoke command '{}', the command does not accept {} arguments",
						command_id, command_argument_counter)
				);
				return std::nullopt;
			}

			std::string_view const& value_str = extractToken(m_query_tokens[c]);
			if (command_argument_counter + 1 < arg_names_lut.size())
			{
				if (c + 1 >= m_query_tokens.size() || extractToken(m_query_tokens[c + 1]) != s_comma)
				{
					LEXGINE_LOG_ERROR(
						this,
						std::format("Expected ',' after {}", value_str)
					);
					return std::nullopt;
				}
				++c;    // take comma between the arguments into account
			}

			ArgSpec const& arg_spec = command_args[command_argument_counter];
			auto value_parsing_result = arg_spec.parser(value_str);
			if (!value_parsing_result.value)
			{
				LEXGINE_LOG_ERROR(
					this,
					std::format("Unable to invoke command '{}', positional argument #{} value parsing error: {}",
						command_id, command_argument_counter + 1, value_parsing_result.error_message)
				);
				return std::nullopt;
			}
			rv.args[arg_spec.name] = *value_parsing_result.value;
			set_arguments[arg_names_lut[arg_spec.name]] = true;
			++command_argument_counter;
			++c;
		}

		// parse trailing named arguments
		while (c + 2 < m_query_tokens.size()
			&& m_query_tokens[c].tag == TokenTag::idendifier
			&& m_query_tokens[c + 1].tag == TokenTag::operation
			&& m_query_tokens[c + 2].tag == TokenTag::idendifier)
		{
			Token const& argument_token = m_query_tokens[c];
			std::string_view argument_id = extractToken(argument_token);
			if (extractToken(m_query_tokens[c + 1]) != s_assignment)
			{
				LEXGINE_LOG_ERROR(
					this,
					std::format("Invalid named argument invocation syntax invoking command '{}'. Argument name {} is followed by {}, when operator '=' is expected",
						command_id, argument_id, extractToken(m_query_tokens[c + 1]))
				);
				return std::nullopt;
			}

			if (arg_names_lut.find(argument_id) == arg_names_lut.end())
			{
				auto [suggestion, distance] = command_spec.findMostLikelyArgName(argument_id);
				std::string err_msg = distance <= 3
					? std::format("Error invoking command '{}'. The command does not have argument '{}'. Did you mean '{}'?",
						command_id, argument_id, suggestion)
					: std::format("Error invoking command '{}'. The command does not have argument '{}'.",
						command_id, argument_id);
				LEXGINE_LOG_ERROR(this, err_msg);
				return std::nullopt;
			}

			std::string_view const& value_token = extractToken(m_query_tokens[c + 2]);
			size_t arg_index = arg_names_lut[argument_id];
			const ArgSpec& arg_spec = command_spec.args[arg_index];
			auto value_parsing_result = arg_spec.parser(value_token);
			if (!value_parsing_result.value)
			{
				LEXGINE_LOG_ERROR(
					this,
					std::format("Error invoking command '{}' when parsing named argument '{}': {}",
						command_id, argument_id, value_parsing_result.error_message)
				);
				return std::nullopt;
			}
			if (set_arguments[arg_index])
			{
				LEXGINE_LOG_ERROR(
					this,
					std::format("Error invoking command '{}'. Argument '{}' has already been assigned a value",
						command_id, arg_spec.name)
				);
				return std::nullopt;
			}
			rv.args[arg_spec.name] = *value_parsing_result.value;
			set_arguments[arg_index] = true;

			++command_arg_count;
			c += 3;
		}

		if(c < m_query_tokens.size())
		{
			while (c < m_query_tokens.size())
			{
				LEXGINE_LOG_ERROR(this,
					std::format("Syntax error trying to invoke command {}. Unexpected token {}.", 
						command_id, extractToken(m_query_tokens[c])));
				++c;
			}
            return std::nullopt;
		}

		// populate default values for the arguments when applicable
		for (size_t i = 0; i < set_arguments.size(); ++i)
		{
			if (!set_arguments[i])
			{
				const ArgSpec& arg_spec = command_spec.args[i];
				if (arg_spec._default)
				{
					rv.args[arg_spec.name] = *arg_spec._default;
				}
				else
				{
					LEXGINE_LOG_ERROR(
						this,
						std::format("Error invoking command '{}': required argument '{}' was not assigned a value",
							command_id, arg_spec.name)
					);
					return std::nullopt;
				}
			}
		}
	}

	return rv;
}

bool CommandRegistry::isSeparator(char c)
{
	for (char e : s_separators)
	{
		if (e == c)
		{
			return true;
		}
	}
	return false;
}

bool CommandRegistry::isOperatorToken(Token const& t) const
{
	for (const auto& op : s_operators)
	{
		if (op == extractToken(t))
		{
			return true;
		}
	}
	return false;
}

std::pair<std::string_view, uint16_t> CommandRegistry::findMostLikelyNamespaceName(std::string_view const& requested_namespace_id) const
{
	uint16_t d = requested_namespace_id.length();
	std::string_view suggestion{};
	for (auto [name, spec] : m_namespaces)
	{
		uint16_t new_d = ConsoleTokenAutocomplete::computeDLDistance(requested_namespace_id, name);
		if (new_d < d)
		{
			suggestion = name;
			d = new_d;
		}
	}
	return { suggestion, d };
}



}