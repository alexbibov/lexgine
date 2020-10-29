// THIS FILE WAS GENERATED BASED ON 'E:/Repos/lexgine/engine/osinteraction/windows/window.h' BY AUTOMATIC PARSER.
// THIS IS A PART OF LEXGINE DLL C++ INTERFACE. DO NOT MODIFY OR DELETE.
// COPYRIGHT(C) ALEXANDER BIBOV, 2020


#include "window.h"
#include "E:/Repos/lexgine/engine/osinteraction/windows/window.h"
using namespace lexgine;
using namespace lexgine::runtime::osinteraction::windows;

void Window::setTitle(std::wstring const & title)
{
	reinterpret_cast<lexgine::osinteraction::windows::Window*>(m_ptr)->setTitle(title);
}

std::wstring Window::getTitle() const
{
	return reinterpret_cast<lexgine::osinteraction::windows::Window const*>(m_ptr)->getTitle();
}

void Window::setDimensions(uint32_t width, uint32_t height)
{
	reinterpret_cast<lexgine::osinteraction::windows::Window*>(m_ptr)->setDimensions(width, height);
}

void Window::setDimensions(core::math::Vector2u const & dimensions)
{
	reinterpret_cast<lexgine::osinteraction::windows::Window*>(m_ptr)->setDimensions(dimensions);
}

core::math::Vector2u Window::getDimensions() const
{
	return reinterpret_cast<lexgine::osinteraction::windows::Window const*>(m_ptr)->getDimensions();
}

core::math::Rectangle Window::getClientArea() const
{
	return reinterpret_cast<lexgine::osinteraction::windows::Window const*>(m_ptr)->getClientArea();
}

void Window::setLocation(uint32_t x, uint32_t y)
{
	reinterpret_cast<lexgine::osinteraction::windows::Window*>(m_ptr)->setLocation(x, y);
}

void Window::setLocation(core::math::Vector2u const & location)
{
	reinterpret_cast<lexgine::osinteraction::windows::Window*>(m_ptr)->setLocation(location);
}

core::math::Vector2u Window::getLocation() const
{
	return reinterpret_cast<lexgine::osinteraction::windows::Window const*>(m_ptr)->getLocation();
}

void Window::setVisibility(bool visibility_flag)
{
	reinterpret_cast<lexgine::osinteraction::windows::Window*>(m_ptr)->setVisibility(visibility_flag);
}

bool Window::getVisibility() const
{
	return reinterpret_cast<lexgine::osinteraction::windows::Window const*>(m_ptr)->getVisibility();
}

Window::Window(void* internal_ptr)
: m_ptr{internal_ptr}
{
}
