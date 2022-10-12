#ifndef LEXGINE_API_IOC_H
#define LEXGINE_API_IOC_H

#include <cstdint>
#include <memory>
#include <api/link_result.h>
#include <api/preprocessing/preprocessor_tokens.h>
#include <common/ioc_traits.h>

namespace lexgine::api{

class Ioc
{
public:
    using deleter_type = void(*)(void*);


public:    // runtime linking infrastructure
    static LinkResult link(HMODULE module);    //! Runtime link interface
    void const* getNative() const;
    void* getNative();
    void reset();

public:
    virtual ~Ioc() = default;

protected:
    Ioc(FakeConstruction_tag): m_runtime_dangling_pointer{nullptr} {}
    Ioc(common::ImportedOpaqueClass ioc_name, deleter_type deleter);
    Ioc(std::shared_ptr<Ioc> const& ptr);
    Ioc(Ioc* ptr);

private:
    std::shared_ptr<uint8_t[]> m_impl_buf;
    std::shared_ptr<Ioc> m_runtime_owned_object;
    Ioc* m_runtime_dangling_pointer;
};

}

#endif