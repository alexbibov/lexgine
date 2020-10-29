// THIS FILE WAS GENERATED BASED ON 'E:/Repos/lexgine/engine/osinteraction/windows/window.h' BY AUTOMATIC PARSER.
// THIS IS A PART OF LEXGINE DLL C++ INTERFACE. DO NOT MODIFY OR DELETE.
// COPYRIGHT(C) ALEXANDER BIBOV, 2020


#ifndef LEXGINE_RUNTIME_WINDOW_H
#define LEXGINE_RUNTIME_WINDOW_H

#include <windows.h>
#include <dxgi1_5.h>
#include <d3d12.h>
#include <cstdint>
#include <string>
#include <list>
#include <vector>
#include "engine/runtime/external_parser_tokens.h"
#include "engine/core/math/vector_types.h"
#include "engine/core/math/rectangle.h"
#include "engine/core/misc/flags.h"
#include "engine/core/entity.h"
#include "engine/core/class_names.h"
#include "engine/osinteraction/listener.h"

namespace lexgine::runtime::osinteraction::windows {

class LEXGINE_API Window
{
	friend class EngineManager;

public:

	//! sets new title for the window
	void setTitle(std::wstring const & title);

	//! gets current title of the window
	std::wstring getTitle() const;

	//! sets new dimensions for the window
	void setDimensions(uint32_t width, uint32_t height);

	//! sets new dimensions for the window. The new dimensions are packed into two-dimensional vector (width, height) in this order
	void setDimensions(core::math::Vector2u const & dimensions);

	//! returns current dimensions of the window
	core::math::Vector2u getDimensions() const;

	//! returns client area of the window
	core::math::Rectangle getClientArea() const;

	//! sets new location of the top-left corner of the window
	void setLocation(uint32_t x, uint32_t y);

	//! sets new location of the top-left corner of the window. The new location (x, y) in this order is packed into two-dimensional vector
	void setLocation(core::math::Vector2u const & location);

	//! returns current location of the top-left corner of the window represented in screen space coordinates
	core::math::Vector2u getLocation() const;

	//! sets visibility of the window
	void setVisibility(bool visibility_flag);

	//! returns visibility flag of the window
	bool getVisibility() const;

	
private:
	Window(void*);

private:
	void* m_ptr;
};

}

#endif
