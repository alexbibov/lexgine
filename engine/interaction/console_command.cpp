#include <algorithm>
#include <stdexcept>
#include <unordered_set>
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

const char CommandRegistry::s_token_separators[] = {'\t', ' ', '=', '\r', '\n'};
const char CommandRegistry::s_separating_tokens[] = { '.', '=', ','};

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
	auto query_tokens = tokenize(query);
	m_contextualized_query_tokens = parseTokensContext(query_tokens);

	size_t c = 0;
	std::string namespace_id{};
	while (c + 1 < m_contextualized_query_tokens.size() + 1
		&& m_contextualized_query_tokens[c].second == TokenContext::_namespace
		&& m_contextualized_query_tokens[c + 1].second == TokenContext::_operator
		)
	{
		namespace_id += m_contextualized_query_tokens[c].first;
		if (c + 2 < m_contextualized_query_tokens.size() && m_contextualized_query_tokens[c + 2].second == TokenContext::_namespace)
		{
			namespace_id += m_contextualized_query_tokens[c + 1].first;
		}
		c += 2;
	}
	m_autocompleter.setQuery(namespace_id);

	if (m_namespaces.count(namespace_id) == 0)
	{
		namespace_id = "global";
	}

	Command* p_command{};
	if (c < m_contextualized_query_tokens.size() && m_contextualized_query_tokens[c].second == TokenContext::command)
	{
		NamespaceSpec& namespace_spec = m_namespaces.at(namespace_id);
		m_current_context.push({ &namespace_spec, TokenContext::_namespace });

		std::string command_id = std::string{ m_contextualized_query_tokens[c].first };
		namespace_spec.autocompleter.setQuery(command_id);
		p_command = namespace_spec.commands.count(command_id)
			? &namespace_spec.commands.at(command_id)
			: nullptr;
		++c;
		m_current_context.push(TokenContext::command);
	}

	if (p_command)
	{
		size_t last_argument_name = c;
		while (c < m_contextualized_query_tokens.size())
		{
			if (m_contextualized_query_tokens[c].second == TokenContext::argument_name)
			{
				m_current_context.push(TokenContext::argument_name);
				last_argument_name = c;
			}
			++c;
		}
		p_command->spec.autocompleter.setQuery(m_contextualized_query_tokens[last_argument_name].first);
	}
}

std::vector<std::string> CommandRegistry::autocompleteSuggestions(uint16_t max_allowed_distance) const
{

}

std::optional<CommandExecutionSchema> CommandRegistry::invokeQuery() const
{

}

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

std::vector<std::string_view> CommandRegistry::tokenize(std::string_view const& s)
{
	static auto is_token = [](char c)
		{
			for (char e : s_separating_tokens)
			{
				if (e == c)
					return true;
			}
			return false;
		};

	static auto is_separator = [](char c)
		{
			for (char e : s_token_separators)
			{
				if (e == c)
					return true;
			}
			return false;
		};



	std::vector<std::string_view> tokens{};
	tokens.reserve(s.size() / 2 + 1);

	size_t c1 = 0, c2 = 0;
	while (c1 < s.size())
	{
		bool isSeparatingToken = false;
		while (c1 < s.size() && ((isSeparatingToken = is_token(s[c1])) || is_separator(s[c1])))
		{
			if (isSeparatingToken)
			{
				tokens.push_back(std::string_view{ s.data() + c1, s.data() + c1 + 1 });
			}
			++c1;
		}
		c2 = c1;
		while (c2 < s.size() && !is_separator(s[c2]) && !is_token(s[c2])) ++c2;
		if (c1 != c2)
		{
			tokens.push_back(std::string_view{ s.data() + c1, s.data() + c2 });
		}
		c1 = c2;
	}

	return tokens;
}

std::vector<std::pair<std::string_view, CommandRegistry::TokenContext>> CommandRegistry::parseTokensContext(std::vector<std::string_view> const& tokens)
{
	std::vector<std::pair<std::string_view, CommandRegistry::TokenContext>> contextualized_tokens{};
	contextualized_tokens.reserve(tokens.size());
	size_t c = 0;
	while (c + 1 < tokens.size() && tokens[c + 1] == ".")
	{
		contextualized_tokens.push_back(std::make_pair(tokens[c], TokenContext::_namespace));
		contextualized_tokens.push_back(std::make_pair(tokens[c + 1], TokenContext::_operator));
		c += 2;
	}

	if (c < tokens.size())
	{
		contextualized_tokens.push_back(std::make_pair(tokens[c], TokenContext::command));
		++c;
	}

	while (c < tokens.size())
	{
		if (c + 1 < tokens.size() && tokens[c + 1] == "=")
		{
			break;
		}
		contextualized_tokens.push_back(std::make_pair(tokens[c], TokenContext::argument_value));
		if (c + 1 < tokens.size())
		{
			if (tokens[c + 1] == ",")
			{
				contextualized_tokens.push_back(std::make_pair(tokens[c + 1], TokenContext::_operator));
			}
			else
			{
				contextualized_tokens.push_back(std::make_pair(tokens[c + 1], TokenContext::unknown));
				break;
			}
		}
		c += 2;
	}

	while (c + 2 < tokens.size() && !(contextualized_tokens.size() > 1 && contextualized_tokens.back().second == TokenContext::unknown))
	{
		contextualized_tokens.push_back(std::make_pair(tokens[c], TokenContext::argument_name));
		if (tokens[c + 1] == "=")
		{
			contextualized_tokens.push_back(std::make_pair(tokens[c + 1], TokenContext::_operator));
		}
		else
		{
			contextualized_tokens.push_back(std::make_pair(tokens[c + 1], TokenContext::unknown));
		}
		contextualized_tokens.push_back(std::make_pair(tokens[c + 2], TokenContext::argument_value));
		if (c + 3 < tokens.size())
		{
			if (tokens[c + 3] == ",")
			{
				contextualized_tokens.push_back(std::make_pair(tokens[c + 3], TokenContext::_operator));
			}
			else
			{
				contextualized_tokens.push_back(std::make_pair(tokens[c + 3], TokenContext::unknown));
			}
		}
		c += 4;
	}

	while (c < tokens.size())
	{
		contextualized_tokens.push_back(std::make_pair(tokens[c], TokenContext::unknown));
		++c;
	}

	return contextualized_tokens;
}

std::optional<CommandExecutionSchema> CommandRegistry::parse(std::vector<std::pair<std::string_view, TokenContext>> const& contextualized_tokens) const
{
	// Short grammar description
	//
	// [namespace.]command value [, value]+ [, name = value]* 
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

	if (contextualized_tokens.empty())
	{
		return std::nullopt;
	}

	std::string namespace_id{};
	size_t c = 0;
	while (c + 1 < contextualized_tokens.size()
		&& contextualized_tokens[c].second == TokenContext::_namespace
		&& contextualized_tokens[c + 1].second == TokenContext::_operator)
	{
		if (contextualized_tokens[c + 1].first != ".")
		{
			LEXGINE_LOG_ERROR(this,
				std::format("Invalid operator {} following {}. Expected '.'", contextualized_tokens[c + 1].first, namespace_id));
			return std::nullopt;
		}

		namespace_id += contextualized_tokens[c].first;
		// look-ahead and check if the next token after the operator is still a namespace. Only append separating operator to the
		// namespace name if the token following it is still a namespace
		if (c + 2 < contextualized_tokens.size() && contextualized_tokens[c + 2].second == TokenContext::_namespace)
		{
			namespace_id += contextualized_tokens[c + 1].first;
		}
		c += 2;
	}
	if (namespace_id.empty())
	{
		namespace_id = "global";
	}

	NamespaceSpec const* p_namespace_spec{};
	{
		auto p = m_namespaces.find(namespace_id);
		if (p == m_namespaces.end())
		{
			auto [suggestion, distance] = findMostLikelyNamespaceName(namespace_id);
			std::string err_msg = distance <= 3
				? std::format("Invalid namespace identifier '{}'. Namespace is not defined. Did you mean {}?", namespace_id, suggestion)
				: std::format("Invalid namespace identifier '{}'. Namespace is not defined.", namespace_id);
			LEXGINE_LOG_ERROR(this, err_msg);
			return std::nullopt;
		}
		p_namespace_spec = &p->second;
	}
	
	// Try fetching target command
	if (c >= contextualized_tokens.size() || contextualized_tokens[c].second != TokenContext::command)
	{
		LEXGINE_LOG_ERROR(this,
			std::format("No command id specifier after namespace '{}'", namespace_id)
		);
		return std::nullopt;
	}

	std::string command_id = std::string{ contextualized_tokens[c].first }; ++c;
	Command const* p_command{};
	{
		auto p = p_namespace_spec->commands.find(command_id);
		if (p == p_namespace_spec->commands.end())
		{
			auto [suggestion, distance] = p_namespace_spec->findMostLikelyCommandName(command_id);
			std::string err_msg = distance <= 3
				? std::format("Command with identifier '{}' is not found in namespace '{}'. Did you mean {}?",
					command_id, namespace_id, suggestion)
				: std::format("Command with identifier '{}' is not found in namespace '{}'.",
					command_id, namespace_id);
			LEXGINE_LOG_ERROR(this, err_msg);
			return std::nullopt;
		}
		p_command = &p->second;
	}

	CommandExecutionSchema rv{ .exec = p_command->exec };
	size_t command_arg_count = p_command->spec.args.size();
	std::unordered_map<std::string_view, size_t> arg_names_lut{}; arg_names_lut.reserve(command_arg_count);
	for (size_t i = 0; i < command_arg_count; ++i)
	{
		const ArgSpec& arg_spec = p_command->spec.args[i];
		arg_names_lut[arg_spec.name] = i;
	}
	std::vector<bool> set_arguments(command_arg_count, false);
	{
		// parse command arguments

		// parse value arguments
		size_t command_argument_counter = 0;
		while (c < contextualized_tokens.size() && contextualized_tokens[c].second == TokenContext::argument_value)
		{
			if (command_argument_counter == arg_names_lut.size())
			{
				LEXGINE_LOG_ERROR(
					this,
					std::format("Unable to invoke command '{}', the command does not accept {} arguments",
						command_id, command_argument_counter)
				);
				return std::nullopt;
			}

			std::string_view const& value_token = contextualized_tokens[c].first;
			ArgSpec const& arg_spec = p_command->spec.args[command_argument_counter];
			auto value_parsing_result = arg_spec.parser(value_token);
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
		while (c + 3 < contextualized_tokens.size()
			&& contextualized_tokens[c].second == TokenContext::argument_name
			&& contextualized_tokens[c + 1].second == TokenContext::_operator
			&& contextualized_tokens[c + 2].second == TokenContext::argument_value)
		{
			std::string_view const& argument_id = contextualized_tokens[c].first;
			if (contextualized_tokens[c + 1].first != "=")
			{
				LEXGINE_LOG_ERROR(
					this,
					std::format("Invalid named argument invocation syntax invoking command '{}'. Argument name {} is followed by {}, when operator '=' is expected",
						command_id, argument_id, contextualized_tokens[c + 1].first)
				);
				return std::nullopt;
			}

			if (arg_names_lut.find(argument_id) == arg_names_lut.end())
			{
				auto [suggestion, distance] = p_command->spec.findMostLikelyArgName(argument_id);
				std::string err_msg = distance <= 3
					? std::format("Error invoking command '{}'. The command does not have argument '{}'. Did you mean '{}'?",
						command_id, argument_id, suggestion)
					: std::format("Error invoking command '{}'. The command does not have argument '{}'.",
						command_id, argument_id);
				LEXGINE_LOG_ERROR(this, err_msg);
				return std::nullopt;
			}

			std::string_view const& value_token = contextualized_tokens[c + 2].first;
			size_t arg_index = arg_names_lut[argument_id];
			const ArgSpec& arg_spec = p_command->spec.args[arg_index];
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
					std::format("Error invoking command '{}'. Argument '{}' already has been assigned a value",
						command_id, arg_spec.name)
				);
				return std::nullopt;
			}
			rv.args[arg_spec.name] = *value_parsing_result.value;
			set_arguments[arg_index] = true;

			++command_arg_count;
			c += 3;
		}

		{
			bool unexpected_tokens = c < contextualized_tokens.size();
			while (c < contextualized_tokens.size())
			{
				LEXGINE_LOG_ERROR(this,
					std::format("Syntax error trying to invoke command {}. Unexpected token {}.", command_id, contextualized_tokens[c].first));
				++c;
			}
			if (unexpected_tokens)
			{
				return std::nullopt;
			}
		}

		// populate default values for the arguments when applicable
		for (size_t i = 0; i < set_arguments.size(); ++i)
		{
			if (!set_arguments[i])
			{
				const ArgSpec& arg_spec = p_command->spec.args[i];
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