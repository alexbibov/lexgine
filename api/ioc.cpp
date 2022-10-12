#include <cassert>
#include "ioc.h"

namespace lexgine::api {

namespace {

size_t(LEXGINE_CALL* api__lexgineIocTraitsGetImportedOpaqueClassSize)(common::ImportedOpaqueClass) = nullptr;

}

LinkResult Ioc::link(HMODULE module)
{
    LinkResult rv{ module };
    api__lexgineIocTraitsGetImportedOpaqueClassSize = reinterpret_cast<decltype(api__lexgineIocTraitsGetImportedOpaqueClassSize)>(rv.attemptLink("lexgineIocTraitsGetImportedOpaqueClassSize"));
    return rv;
}

Ioc::Ioc(common::ImportedOpaqueClass ioc_name, deleter_type deleter)
    : m_runtime_dangling_pointer{ nullptr }
{
    assert(api__lexgineIocTraitsGetImportedOpaqueClassSize);
    size_t ioc_size = api__lexgineIocTraitsGetImportedOpaqueClassSize(ioc_name);
    m_impl_buf.reset(new uint8_t[ioc_size], deleter);
}


Ioc::Ioc(std::shared_ptr<Ioc> const& ptr)
    : m_runtime_owned_object{ ptr }
    , m_runtime_dangling_pointer{ nullptr }
{
}


Ioc::Ioc(Ioc* ptr)
    : m_runtime_dangling_pointer{ ptr }
{

}


void const* Ioc::getNative() const 
{
    return const_cast<Ioc*>(this)->getNative();
}

void* Ioc::getNative()
{
    if (m_impl_buf) return m_impl_buf.get();
    if (m_runtime_owned_object) return m_runtime_owned_object.get();
    if (m_runtime_dangling_pointer) return m_runtime_dangling_pointer;
    return nullptr;
}

void Ioc::reset()
{
    if (m_impl_buf) m_impl_buf.reset();
    if (m_runtime_owned_object) m_runtime_owned_object.reset();
}



}