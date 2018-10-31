#include "rectangle.h"

using namespace lexgine::core::math;


Rectangle::Rectangle() : ul{ 0, 0 }, width{ 0.0f }, height{ 0.0f }
{

}

Rectangle::Rectangle(Vector2f const& ul_corner, float width, float height) :
	ul{ ul_corner }, width{ width }, height{ height }
{

}

Vector2f Rectangle::upperLeft() const
{
	return ul;
}

Vector2f Rectangle::size() const
{
	return Vector2f{ width, height };
}

void Rectangle::setUpperLeft(Vector2f const& ul_corner)
{
	ul = ul_corner;
}

void Rectangle::setSize(float width, float height)
{
	this->width = width;
	this->height = height;
}