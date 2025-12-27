#include <algorithm>
#include <stdexcept>
#include <unordered_set>
#include <queue>

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

std::vector<std::string> CommandSpec::autocompleteSuggestions(size_t max_count) const
{
	auto suggestions = autocompleter.suggestions(max_count);
	std::vector<std::string> rv{};
	rv.resize(suggestions.size());
	std::transform(suggestions.begin(), suggestions.end(), rv.begin(),
		[](auto const& e) {return e.first; });
	return rv;
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
	NamespaceSpec& global_namespace = m_namespaces.at("global");
	m_current_autompletion_context.push(&global_namespace);
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
	m_current_query.clear();
	m_query_tokens.clear();
	while (m_current_autompletion_context.size() != 1) m_current_autompletion_context.pop();
	for (char c : query)
	{
		append(c);
	}
}


void CommandRegistry::append(char c)
{
	m_current_query.push_back(c);
	tokenize(m_current_query.size() - 1);

	QueryContext const& current_context = m_current_autompletion_context.top();
	if (std::holds_alternative<NamespaceSpec*>(current_context))
	{
		// Namespace context (currently specifying command)

		bool is_dot_operator = c == '.';
		bool is_separator = isSeparator(c);
		
		if (is_dot_operator || is_separator)
		{
			auto ns = std::get<NamespaceSpec*>(current_context);
			if (Token command_id = extractCommandIdToken())
			{
				if (is_dot_operator)
				{
					auto p = m_namespaces.find(extractToken(command_id));
					if (p != m_namespaces.end())
					{
						m_current_autompletion_context.push(&p->second);
						return;
					}
				}
				if(is_separator)
				{
					if (Token cname = extractCommandNameTokenFromCommandIdToken(command_id))
					{
						auto& commands_registry = ns->commands;
						auto p = commands_registry.find(extractToken(cname));
						if (p != commands_registry.end())
						{
							m_current_autompletion_context.push(&p->second);
							return;
						}
					}
				}
			}
		}
	}
}

void CommandRegistry::backspace()
{
	if (m_current_query.empty())
	{
		return;
	}
	char c = m_current_query.back();
	m_current_query.pop_back();

	bool is_dot_operator = c == '.';
	bool is_separator = isSeparator(c);
	if (!m_query_tokens.empty() && !is_separator)
	{
		QueryContext const& current_context = m_current_autompletion_context.top();
		Token& t = m_query_tokens.back();
		if (t.tag == TokenTag::operation)
		{
			if (is_dot_operator)
			{
				// switch context 
				if (m_current_autompletion_context.size() > 1)
				{
					m_current_autompletion_context.pop();
				}
			}
			if (is_separator 
				&& m_query_tokens.size() > 1 
				&& m_query_tokens[m_query_tokens.size() - 2].tag == TokenTag::idendifier)
			{
				// switch context
				if (m_current_autompletion_context.size() > 1)
				{
					m_current_autompletion_context.pop();
				}
			}
		}
		--t.count;
		if (t.count == 0)
		{
			m_query_tokens.pop_back();
		}
	}
}

std::vector<std::string> CommandRegistry::autocompleteSuggestions(size_t max_count) const
{
	assert(!m_current_autompletion_context.empty());

	QueryContext const& current_context = m_current_autompletion_context.top();
	if (std::holds_alternative<NamespaceSpec*>(current_context))
	{
		auto* ns = std::get<NamespaceSpec*>(current_context);
		return ns->autocompleteSuggestions(max_count);
	}
	if (std::holds_alternative<Command*>(current_context))
	{
		auto* command_context = std::get<Command*>(current_context);
		return command_context->spec.autocompleteSuggestions(max_count);
	}
	return {};
}

std::optional<CommandExecutionSchema> CommandRegistry::invokeQuery() const
{
	return parse();
}

char CommandRegistry::s_separators[] = { '\t', ' ', '=', '\r', '\n' };
char CommandRegistry::s_operators[] = { '.', '=', ',' };

std::vector<std::string> CommandRegistry::NamespaceSpec::autocompleteSuggestions(size_t max_count) const
{
	auto suggestions = autocompleter.suggestions(max_count);
	std::vector<std::string> rv{};
	rv.resize(suggestions.size());
	std::transform(suggestions.begin(), suggestions.end(), rv.begin(),
		[](auto const& e) {return e.first; });
	return rv;
}

std::pair<std::string_view, uint16_t> CommandRegistry::NamespaceSpec::findMostLikelyCommandName(std::string_view const& requested_command_name) const
{
	uint16_t d = requested_command_name.length();
	std::string_view suggestion{};
	for (auto [name, spec] : commands)
	{
		uint16_t new_d = ConsoleTokenAutocomplete::computeDLDistance(requested_command_name, name);
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

void CommandRegistry::tokenize(size_t start_position)
{
	if (start_position == 0)
	{
		m_query_tokens.clear();
	}

	Token* p_existing_token{ nullptr };
	if (
		!m_query_tokens.empty() 
		&& (m_query_tokens.back().tag == TokenTag::idendifier || m_query_tokens.back().tag == TokenTag::value)
		)
	{
		Token& e = m_query_tokens.back();
		p_existing_token = &e;
	}
	for (size_t i = start_position; i < m_current_query.size(); ++i)
	{
		if (isOperator(m_current_query[i]))
		{
			p_existing_token = nullptr;
			m_query_tokens.push_back({ .start_index = i, .count = 1, .tag = TokenTag::operation });
			continue;
		}
		if (isSeparator(m_current_query[i]))
		{
			p_existing_token = nullptr;
			continue;
		}

		if (p_existing_token)
		{
			++p_existing_token->count;
		}
		else
		{
			TokenTag tag = !m_query_tokens.empty()
				&& m_query_tokens.back().tag == TokenTag::operation
				&& extractToken(m_query_tokens.back()) == "="
				? TokenTag::value
				: TokenTag::idendifier;
			m_query_tokens.push_back({ .start_index = i, .count = 1, .tag = tag });
			Token& e = m_query_tokens.back();
			p_existing_token = &e;
		}
	}
}

CommandRegistry::Token CommandRegistry::extractCommandIdToken() const
{
	// Command identifier is defined by the following grammar:
	// command_identifier:= \.?<ns_identifier>(\.<ns_identifier>)*
	// ns_identifier:= ^[A-Za-z_][A-Za-z0-9_]*$

	if (m_query_tokens.empty())
	{
		return {};
	}

	auto is_dot_operator = [this](Token const& t) -> bool
		{
			return t.tag == TokenTag::operation && extractToken(t) == ".";
		};

	size_t start_token_index = is_dot_operator(m_query_tokens[0]) ? 1 : 0;
	size_t end_token_index = start_token_index;
	if (end_token_index < m_query_tokens.size()
		&& m_query_tokens[end_token_index].tag == TokenTag::idendifier)
	{
		++end_token_index;
	}
	while (
		end_token_index + 1 < m_query_tokens.size()
		&& is_dot_operator(m_query_tokens[end_token_index])
		&& m_query_tokens[end_token_index + 1].tag == TokenTag::idendifier
		)
	{
		end_token_index += 2;
	}

	end_token_index = (std::min)(end_token_index, m_query_tokens.size());
	if (end_token_index <= start_token_index)
	{
		return {};
	}
	size_t s = m_query_tokens[start_token_index].start_index;
	size_t e = m_query_tokens[end_token_index - 1].start_index + m_query_tokens[end_token_index - 1].count - s;
	return { .start_index = s, .count = e, .tag = TokenTag::idendifier };
}

CommandRegistry::Token CommandRegistry::extractCommandNameTokenFromCommandIdToken(Token const& command_id) const
{
	if (command_id)
	{
		std::string_view cid = extractToken(command_id);
		size_t p = cid.find_last_of(".");
		if (p != std::string_view::npos)
		{
			size_t s = command_id.start_index + p + 1;
			return { .start_index = command_id.start_index + p + 1, .count = command_id.count - p - 1, .tag = TokenTag::idendifier };
		}
		return command_id;
	}
	return {};
}

CommandRegistry::Token CommandRegistry::extractNamespaceNameTokenFromCommandIdToken(Token const& command_id) const
{
	Token command_name = extractCommandNameTokenFromCommandIdToken(command_id);
	if (command_name)
	{
		std::string_view namespace_name_string{ m_current_query.data() + command_id.start_index, command_id.count - command_name.count };
		if (namespace_name_string.empty())
		{
			return {};
		}
		size_t s = namespace_name_string.find_first_not_of(".");
		if (s == std::string_view::npos)
		{
			return {};
		}
		size_t e = namespace_name_string.find_last_of(".");
		return { 
			.start_index = command_id.start_index + s,
			.count = e != std::string_view::npos ? e - command_id.start_index - s : namespace_name_string.length() - s,
			.tag = TokenTag::idendifier
		};
	}
	return {};
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

	Token command_id = extractCommandIdToken();
	if (!command_id)
	{
		LEXGINE_LOG_ERROR(this, std::format("Invalid command identifier {}", extractToken(command_id)));
		return std::nullopt;
	}

	Token namespace_name = extractNamespaceNameTokenFromCommandIdToken(command_id);
	std::string requested_namespace_name{};
	NamespaceSpec const* p_namespace_spec{};
	if(namespace_name)
	{
		requested_namespace_name = extractToken(namespace_name);
		auto p = m_namespaces.find(requested_namespace_name);
		if (p == m_namespaces.end())
		{
			auto [suggestion, distance] = findMostLikelyNamespaceName(requested_namespace_name);
			std::string err_msg = distance <= 3
				? std::format("Invalid namespace identifier '{}'. Namespace is not defined. Did you mean {}?", requested_namespace_name, suggestion)
				: std::format("Invalid namespace identifier '{}'. Namespace is not defined.", requested_namespace_name);
			LEXGINE_LOG_ERROR(this, err_msg);
			return std::nullopt;
		}
		p_namespace_spec = &p->second;
	}
	else
	{
		requested_namespace_name = "global";
		p_namespace_spec = &m_namespaces.at("global");
	}

	Token command_name = extractCommandNameTokenFromCommandIdToken(command_id);
	if (!command_name)
	{
		LEXGINE_LOG_ERROR(this, std::format("Command identifier {} has invalid syntax", extractToken(command_id)));
		return std::nullopt;
	}
	std::string_view requested_command_name = extractToken(command_name);
	Command const* p_command{};
	{

		auto p = p_namespace_spec->commands.find(requested_command_name);
		if (p == p_namespace_spec->commands.end())
		{
			auto [suggestion, distance] = p_namespace_spec->findMostLikelyCommandName(requested_command_name);
			std::string err_msg = distance <= 3
				? std::format("Command with identifier '{}' is not found in namespace '{}'. Did you mean {}?",
					requested_command_name, requested_namespace_name, suggestion)
				: std::format("Command with identifier '{}' is not found in namespace '{}'.",
					requested_command_name, requested_namespace_name);
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

		size_t c = 0;
		while (c < m_query_tokens.size() && m_query_tokens[c].start_index <= command_name.start_index)
		{
			++c;
		}

		// parse value arguments
		size_t command_argument_counter = 0;
		while (c < m_query_tokens.size() && m_query_tokens[c].tag == TokenTag::value)
		{
			if (command_argument_counter == arg_names_lut.size())
			{
				LEXGINE_LOG_ERROR(
					this,
					std::format("Unable to invoke command '{}', the command does not accept {} arguments",
						requested_command_name, command_argument_counter)
				);
				return std::nullopt;
			}

			std::string_view value_str = extractToken(m_query_tokens[c]);
			ArgSpec const& arg_spec = p_command->spec.args[command_argument_counter];
			auto value_parsing_result = arg_spec.parser(value_str);
			if (!value_parsing_result.value)
			{
				LEXGINE_LOG_ERROR(
					this,
					std::format("Unable to invoke command '{}', positional argument #{} value parsing error: {}",
						requested_command_name, command_argument_counter + 1, value_parsing_result.error_message)
				);
				return std::nullopt;
			}
			rv.args[arg_spec.name] = *value_parsing_result.value;
			set_arguments[arg_names_lut[arg_spec.name]] = true;
			++command_argument_counter;
			++c;
		}

		// parse trailing named arguments
		while (c + 3 < m_query_tokens.size()
			&& m_query_tokens[c].tag == TokenTag::idendifier
			&& m_query_tokens[c + 1].tag == TokenTag::operation
			&& m_query_tokens[c + 2].tag == TokenTag::value)
		{
			std::string_view argument_name_str = extractToken(m_query_tokens[c]);
			std::string_view op_str = extractToken(m_query_tokens[c + 1]);
			if (op_str != "=")
			{
				LEXGINE_LOG_ERROR(
					this,
					std::format("Invalid named argument invocation syntax invoking command '{}'. Argument name {} is followed by {} when operator '=' is expected",
						requested_command_name, argument_name_str, op_str)
				);
				return std::nullopt;
			}

			if (arg_names_lut.find(argument_name_str) == arg_names_lut.end())
			{
				auto [suggestion, distance] = p_command->spec.findMostLikelyArgName(argument_name_str);
				std::string err_msg = distance <= 3
					? std::format("Error invoking command '{}'. The command does not have argument '{}'. Did you mean '{}'?",
						requested_command_name, argument_name_str, suggestion)
					: std::format("Error invoking command '{}'. The command does not have argument '{}'.",
						requested_command_name, argument_name_str);
				LEXGINE_LOG_ERROR(this, err_msg);
				return std::nullopt;
			}

			std::string_view value_str = extractToken(m_query_tokens[c + 2]);
			size_t arg_index = arg_names_lut[value_str];
			const ArgSpec& arg_spec = p_command->spec.args[arg_index];
			auto value_parsing_result = arg_spec.parser(value_str);
			if (!value_parsing_result.value)
			{
				LEXGINE_LOG_ERROR(
					this,
					std::format("Error invoking command '{}' when parsing named argument '{}': {}",
						requested_command_name, argument_name_str, value_parsing_result.error_message)
				);
				return std::nullopt;
			}
			if (set_arguments[arg_index])
			{
				LEXGINE_LOG_ERROR(
					this,
					std::format("Error invoking command '{}'. Argument '{}' already has been assigned a value",
						requested_command_name, arg_spec.name)
				);
				return std::nullopt;
			}
			rv.args[arg_spec.name] = *value_parsing_result.value;
			set_arguments[arg_index] = true;

			++command_arg_count;
			c += 3;
		}

		{
			bool unexpected_tokens = c < m_query_tokens.size();
			while (c < m_query_tokens.size())
			{
				LEXGINE_LOG_ERROR(this,
					std::format("Syntax error trying to invoke command {}. Unexpected token {}.", requested_command_name, extractToken(m_query_tokens[c])));
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
							requested_command_name, arg_spec.name)
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

bool CommandRegistry::isOperator(char c)
{
	for (char e : s_operators)
	{
		if (e == c)
		{
			return true;
		}
	}
	return false;
}

bool CommandRegistry::isOperator(Token const& t) const
{
	return t.count == 1 && isOperator(m_current_query[t.start_index]);
}

std::pair<std::string_view, uint16_t> CommandRegistry::findMostLikelyNamespaceName(std::string_view const& requested_namespace_name) const
{
	uint16_t d = requested_namespace_name.length();
	std::string_view suggestion{};
	for (auto [name, spec] : m_namespaces)
	{
		uint16_t new_d = ConsoleTokenAutocomplete::computeDLDistance(requested_namespace_name, name);
		if (new_d < d)
		{
			suggestion = name;
			d = new_d;
		}
	}
	return { suggestion, d };
}



}