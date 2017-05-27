#include "rectangle.h"

using namespace lexgine::core::math;


Rectangle::Rectangle() : ul{ 0, 0 }, width{ 0.0f }, height{ 0.0f }
{

}

Rectangle::Rectangle(vector2f const& ul_corner, float width, float height) :
	ul{ ul_corner }, width{ width }, height{ height }
{

}

vector2f Rectangle::upperLeft() const
{
	return ul;
}

vector2f Rectangle::size() const
{
	return vector2f{ width, height };
}

void Rectangle::setUpperLeft(vector2f const& ul_corner)
{
	ul = ul_corner;
}

void Rectangle::setSize(float width, float height)
{
	this->width = width;
	this->height = height;
}