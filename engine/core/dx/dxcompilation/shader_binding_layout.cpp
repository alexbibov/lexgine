#include <algorithm>
#include <bitset>
#include <climits>
#include <format>
#include <map>
#include <tuple>

#include "engine/core/misc/misc.h"

#include "shader_binding_layout.h"


namespace lexgine::core::dx::dxcompilation {

namespace
{

enum class RegisterClass
{
    b,
    t,
    u,
    s
};

struct MergedRange
{
    RegisterClass register_class;
    uint32_t register_space;
    uint32_t base_register;
    uint32_t register_count;
    bool is_unbounded;
    std::vector<size_t> declarations;
};

RegisterClass registerClassOf(ShaderInputKind kind)
{
    switch (kind)
    {
    case ShaderInputKind::cbv:
        return RegisterClass::b;
    case ShaderInputKind::srv:
        return RegisterClass::t;
    case ShaderInputKind::uav:
        return RegisterClass::u;
    case ShaderInputKind::sampler:
    case ShaderInputKind::comparison_sampler:
        return RegisterClass::s;
    default:
        LEXGINE_ASSUME;
    }

    return RegisterClass::t;
}

char registerLiteral(RegisterClass register_class)
{
    switch (register_class)
    {
    case RegisterClass::b:
        return 'b';
    case RegisterClass::t:
        return 't';
    case RegisterClass::u:
        return 'u';
    case RegisterClass::s:
        return 's';
    default:
        LEXGINE_ASSUME;
    }

    return '?';
}

d3d12::ShaderVisibleMemoryResourceType rangeTypeOf(RegisterClass register_class)
{
    switch (register_class)
    {
    case RegisterClass::b:
        return d3d12::ShaderVisibleMemoryResourceType::cbv;
    case RegisterClass::t:
        return d3d12::ShaderVisibleMemoryResourceType::srv;
    case RegisterClass::u:
        return d3d12::ShaderVisibleMemoryResourceType::uav;
    case RegisterClass::s:
        return d3d12::ShaderVisibleMemoryResourceType::sampler;
    default:
        LEXGINE_ASSUME;
    }

    return d3d12::ShaderVisibleMemoryResourceType::count;
}

d3d12::DescriptorHeapType heapTypeOf(RegisterClass register_class)
{
    return register_class == RegisterClass::s ? d3d12::DescriptorHeapType::sampler : d3d12::DescriptorHeapType::cbv_srv_uav;
}

d3d12::ShaderVisibility visibilityOfStage(ShaderType stage)
{
    switch (stage)
    {
    case ShaderType::vertex:
        return d3d12::ShaderVisibility::vertex;
    case ShaderType::hull:
        return d3d12::ShaderVisibility::hull;
    case ShaderType::domain:
        return d3d12::ShaderVisibility::domain;
    case ShaderType::geometry:
        return d3d12::ShaderVisibility::geometry;
    case ShaderType::pixel:
        return d3d12::ShaderVisibility::pixel;
    default:
        return d3d12::ShaderVisibility::all;
    }
}

char const* stageName(ShaderType stage)
{
    switch (stage)
    {
    case ShaderType::vertex:
        return "vertex";
    case ShaderType::hull:
        return "hull";
    case ShaderType::domain:
        return "domain";
    case ShaderType::geometry:
        return "geometry";
    case ShaderType::pixel:
        return "pixel";
    case ShaderType::compute:
        return "compute";
    default:
        return "unknown";
    }
}

std::string describe(ReflectedDeclaration const& declaration)
{
    ShaderBindingPoint const& binding = declaration.binding;
    char literal = registerLiteral(registerClassOf(binding.kind));
    std::string registers = binding.is_unbounded
        ? std::format("{}{}+", literal, binding.first_register)
        : std::format("{}{}..{}{}", literal, binding.first_register, literal, binding.first_register + binding.register_count - 1);
    return std::format("'{}' ({}, space{}, {} shader)", declaration.name.string(), registers, binding.register_space, stageName(declaration.stage));
}

bool overlap(ShaderBindingPoint const& a, ShaderBindingPoint const& b)
{
    if (registerClassOf(a.kind) != registerClassOf(b.kind) || a.register_space != b.register_space)
    {
        return false;
    }

    uint64_t a_end = a.is_unbounded ? UINT64_MAX : static_cast<uint64_t>(a.first_register) + a.register_count;
    uint64_t b_end = b.is_unbounded ? UINT64_MAX : static_cast<uint64_t>(b.first_register) + b.register_count;
    return a.first_register < b_end && b.first_register < a_end;
}

}  // namespace


std::optional<BindingDomain> bindingDomainOfRegisterSpace(uint32_t register_space)
{
    if (register_space <= 9)
    {
        return BindingDomain::pass;
    }
    if (register_space >= 10 && register_space <= 19)
    {
        return BindingDomain::material;
    }
    if (register_space >= 20 && register_space <= 29)
    {
        return BindingDomain::bindless;
    }
    if (register_space == c_root_constant_buffer_register_space)
    {
        return BindingDomain::root_constants;
    }

    return std::nullopt;
}

BindingLayoutCompilationResult compileBindingLayout(std::vector<ReflectedDeclaration> const& declarations,
    std::vector<uint32_t> const& root_constant_buffer_registers)
{
    CompiledBindingLayout layout{};
    uint32_t next_root_slot = 0;
    for (uint32_t shader_register : root_constant_buffer_registers)
    {
        layout.root_constant_buffers.push_back({ .shader_register = shader_register, .root_slot = next_root_slot++ });
    }

    struct NamedInput
    {
        size_t first_declaration;
        uint32_t capacity;
    };

    std::unordered_map<misc::HashedString, NamedInput> named_inputs;
    std::vector<size_t> table_declarations;
    for (size_t i = 0; i < declarations.size(); ++i)
    {
        ReflectedDeclaration const& declaration = declarations[i];
        ShaderBindingPoint const& binding = declaration.binding;

        std::optional<BindingDomain> domain = bindingDomainOfRegisterSpace(binding.register_space);
        if (!domain)
        {
            return std::format("{} uses register space {}, which is not assigned to any binding domain",
                describe(declaration), binding.register_space);
        }

        if (*domain == BindingDomain::root_constants)
        {
            if (binding.kind != ShaderInputKind::cbv)
            {
                return std::format("{} is not a constant buffer, but register space {} is reserved for root constant buffers",
                    describe(declaration), c_root_constant_buffer_register_space);
            }
            if (binding.is_unbounded || binding.register_count != 1)
            {
                return std::format("{} is an array, but root constant buffers cannot be arrays", describe(declaration));
            }
            if (std::find(root_constant_buffer_registers.begin(), root_constant_buffer_registers.end(), binding.first_register)
                == root_constant_buffer_registers.end())
            {
                return std::format("{} has no corresponding root constant buffer parameter", describe(declaration));
            }
            continue;
        }

        if (binding.is_unbounded && *domain != BindingDomain::bindless)
        {
            return std::format("{} is an unbounded array, which may only be declared in bindless register spaces", describe(declaration));
        }
        if (!binding.is_unbounded && *domain == BindingDomain::bindless)
        {
            return std::format("{} is not an unbounded array, but only unbounded arrays may be declared in bindless register spaces",
                describe(declaration));
        }

        auto [named_input, inserted] = named_inputs.try_emplace(declaration.name, NamedInput{ .first_declaration = i, .capacity = binding.register_count });
        if (!inserted)
        {
            ReflectedDeclaration const& first = declarations[named_input->second.first_declaration];
            if (first.binding.kind != binding.kind
                || first.binding.register_space != binding.register_space
                || first.binding.first_register != binding.first_register
                || first.binding.is_unbounded != binding.is_unbounded
                || first.view != declaration.view)
            {
                return std::format("{} and {} share the name but are declared differently", describe(first), describe(declaration));
            }
            named_input->second.capacity = (std::max)(named_input->second.capacity, binding.register_count);
        }

        table_declarations.push_back(i);
    }

    std::sort(table_declarations.begin(), table_declarations.end(),
        [&declarations](size_t a, size_t b)
        {
            ShaderBindingPoint const& lhs = declarations[a].binding;
            ShaderBindingPoint const& rhs = declarations[b].binding;
            return std::make_tuple(registerClassOf(lhs.kind), lhs.register_space, lhs.first_register, lhs.register_count, static_cast<int>(declarations[a].stage))
                < std::make_tuple(registerClassOf(rhs.kind), rhs.register_space, rhs.first_register, rhs.register_count, static_cast<int>(declarations[b].stage));
        }
    );

    std::vector<MergedRange> ranges;
    for (size_t i : table_declarations)
    {
        ReflectedDeclaration const& declaration = declarations[i];
        ShaderBindingPoint const& binding = declaration.binding;
        RegisterClass register_class = registerClassOf(binding.kind);

        bool continues_last_range = !ranges.empty()
            && ranges.back().register_class == register_class
            && ranges.back().register_space == binding.register_space
            && (ranges.back().is_unbounded
                || static_cast<uint64_t>(binding.first_register) <= static_cast<uint64_t>(ranges.back().base_register) + ranges.back().register_count);

        if (!continues_last_range)
        {
            ranges.push_back(MergedRange{
                .register_class = register_class,
                .register_space = binding.register_space,
                .base_register = binding.first_register,
                .register_count = binding.register_count,
                .is_unbounded = binding.is_unbounded,
                .declarations = { i }
            });
            continue;
        }

        MergedRange& range = ranges.back();
        for (size_t j : range.declarations)
        {
            ReflectedDeclaration const& other = declarations[j];
            if (overlap(other.binding, binding) && (other.binding.kind != binding.kind || other.view != declaration.view))
            {
                return std::format("{} and {} overlap but require different views", describe(other), describe(declaration));
            }
        }

        if (range.is_unbounded && binding.first_register != range.base_register)
        {
            return std::format("{} and {} are unbounded arrays with different base registers in the same register space",
                describe(declarations[range.declarations.front()]), describe(declaration));
        }

        if (!range.is_unbounded)
        {
            uint32_t end_register = binding.first_register + binding.register_count;
            range.register_count = (std::max)(range.register_count, end_register - range.base_register);
        }
        range.declarations.push_back(i);
    }

    std::map<std::pair<BindingDomain, d3d12::DescriptorHeapType>, std::vector<size_t>> ranges_per_table;
    for (size_t i = 0; i < ranges.size(); ++i)
    {
        BindingDomain domain = *bindingDomainOfRegisterSpace(ranges[i].register_space);
        ranges_per_table[{ domain, heapTypeOf(ranges[i].register_class) }].push_back(i);
    }

    std::vector<DescriptorTableId> range_tables(ranges.size());
    std::vector<uint32_t> range_offsets(ranges.size());
    for (auto const& [table_key, table_ranges] : ranges_per_table)
    {
        auto [domain, heap_type] = table_key;
        DescriptorTableId table_id = static_cast<DescriptorTableId>(layout.descriptor_tables.size());
        bool is_bindless = domain == BindingDomain::bindless;

        d3d12::RootEntryDescriptorTable declaration{};
        std::bitset<static_cast<size_t>(ShaderType::count)> stages{};
        ShaderType last_stage = ShaderType::unknown;
        uint32_t next_offset = 0;
        for (size_t range_index : table_ranges)
        {
            MergedRange const& range = ranges[range_index];
            uint32_t range_offset = is_bindless ? 0 : next_offset;
            declaration.addRange(rangeTypeOf(range.register_class), range.is_unbounded ? UINT_MAX : range.register_count,
                range.base_register, range.register_space, range_offset);

            range_tables[range_index] = table_id;
            range_offsets[range_index] = range_offset;
            if (!is_bindless)
            {
                next_offset += range.register_count;
            }

            for (size_t declaration_index : range.declarations)
            {
                last_stage = declarations[declaration_index].stage;
                stages.set(static_cast<size_t>(last_stage));
            }
        }

        layout.descriptor_tables.push_back(DescriptorTableLayout{
            .domain = domain,
            .heap_type = heap_type,
            .visibility = stages.count() == 1 ? visibilityOfStage(last_stage) : d3d12::ShaderVisibility::all,
            .declaration = std::move(declaration),
            .descriptor_count = next_offset,
            .root_slot = next_root_slot++
        });
    }

    for (size_t range_index = 0; range_index < ranges.size(); ++range_index)
    {
        MergedRange const& range = ranges[range_index];
        for (size_t declaration_index : range.declarations)
        {
            ReflectedDeclaration const& declaration = declarations[declaration_index];
            layout.bindings.try_emplace(declaration.name, BindingPlacement{
                .table = range_tables[range_index],
                .first_descriptor = range.is_unbounded ? 0 : range_offsets[range_index] + (declaration.binding.first_register - range.base_register),
                .capacity = named_inputs.at(declaration.name).capacity,
                .is_unbounded = declaration.binding.is_unbounded,
                .kind = declaration.binding.kind,
                .view = declaration.view
            });
        }
    }

    return layout;
}

}
