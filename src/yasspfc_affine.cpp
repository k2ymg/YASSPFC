#include "yasspfc_affine.h"
#include <math.h>


using namespace yasspfc;

void Affine::set_translate(float tx, float ty)
{
	this->a = 1;
	this->b = 0;
	this->c = 0;
	this->d = 1;
	this->tx = tx;
	this->ty = ty;
}

void Affine::translate(float tx, float ty)
{
	this->tx += this->a * tx + this->c * ty;
	this->ty += this->b * tx + this->d * ty;
}

void Affine::scale(float sx, float sy)
{
	this->a *= sx;
	this->b *= sx;
	this->c *= sy;
	this->d *= sy;
}

void Affine::rotate(float radian)
{
	float sa = sinf(radian);
	float ca = cosf(radian);

	float a = this->a;
	float b = this->b;
	float c = this->c;
	float d = this->d;

	this->a = a * ca + c * sa;
	this->b = b * ca + d * sa;
	this->c = c * ca - a * sa;
	this->d = d * ca - b * sa;
}

void Affine::transform(int16_t* x, int16_t* y) const
{
	float fx = (float)*x;
	float fy = (float)*y;

	*x = (int16_t)(this->a * fx + this->c * fy + this->tx);
	*y = (int16_t)(this->b * fx + this->d * fy + this->ty);
}

void Affine::concat(const Affine* t)
{
	float a = this->a;
	float b = this->b;
	float c = this->c;
	float d = this->d;
	float tx = this->tx;
	float ty = this->ty;

	this->a = a * t->a + b * t->c;
	this->b = a * t->b + b * t->d;
	this->c = c * t->a + d * t->c;
	this->d = c * t->b + d * t->d;
	this->tx = tx * t->a + ty * t->c + t->tx;
	this->ty = tx * t->b + ty * t->d + t->ty;
}
