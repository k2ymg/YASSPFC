#ifndef YASSPFC_AFFINE_H
#define YASSPFC_AFFINE_H

#include "cocos2d.h"
#include "yasspfc_ccc.h"

namespace yasspfc {
struct Affine {
	float a, b, c, d, tx, ty;

	void set_translate(float tx, float ty);

	void translate(float tx, float ty);
	void scale(float sx, float sy);
	void rotate(float radian);

	void transform(int16_t* x, int16_t* y) const;
	void concat(const Affine* t);
};

}

#endif
