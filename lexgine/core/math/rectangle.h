#ifndef LEXGINE_CORE_MATH_RECTANGLE_H

#include "vector_types.h"

namespace lexgine { namespace core { namespace math {

class Rectangle
{
public:
	Rectangle();	//! initializes rectangle with zero size and the upper left corner located in the origin
	Rectangle(vector2f const& ul_corner, float width, float height);	//! initializes rectangle with upper left corner at given location and with the given size

	vector2f upperLeft() const;	//! returns geometric location of the upper left corner of the rectangle
	vector2f size() const;	//! returns width and height of the rectangle packed into a 2D vector

	void setUpperLeft(vector2f const& ul_corner);	//! sets upper left corner of the rectangle to the given value
	void setSize(float width, float height);	//! updates width and height of the rectangle to the given values

private:
	vector2f ul;	//!< upper left corner of the rectangle
	float width;	//!< width of the rectangle
	float height;	//!< height of the rectangle
};

}}}


#define LEXGINE_CORE_MATH_RECTANGLE_H
#endif