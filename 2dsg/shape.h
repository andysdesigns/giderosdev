#ifndef SHAPE_H
#define SHAPE_H

#include "sprite.h"
#include <vector>
#include "texturebase.h"

class Application;

class Shape : public GSprite
{
public:
	enum WindingRule
	{
		eEvenOdd,
		eNonZero,
	};

    Shape(Application *application);
	virtual ~Shape();

	void setLineStyle(float thickness, unsigned int color, float alpha = 1.f);

	void clearFillStyle();
	void setSolidFillStyle(unsigned int color, float alpha = 1.f);
	void setTextureFillStyle(GTextureBase* texture, const Matrix* matrix = NULL);
//	void setLinearGradientFillStyle();
//	void setRadialGradientFillStyle();

	void beginPath(WindingRule windingRule = eEvenOdd);
	void moveTo(float x, float y);
	void lineTo(float x, float y);
	void closePath();
	void endPath();

	void clear();
protected:
	virtual void extraBounds(float* minx, float* miny, float* maxx, float* maxy) const;

private:
	enum FillType
	{
		eNone,
		eSolid,
		eTexture,
	};

	FillType fillType_;
	float fillr_, fillg_, fillb_, filla_;
	GTextureBase* texture_;
	Matrix matrix_;

	std::vector<GTextureBase*> textures_;

	float liner_, lineg_, lineb_, linea_;
	float thickness_;

	std::vector<std::vector<Point2f> > paths_;

	Point2f min_, max_;

	WindingRule windingRule_;

private:
	void ensureSubpath(float x, float y);
	void extendBounds(float x, float y, float thickness);

private:
    virtual void doDraw(const CurrentTransform&, float sx, float sy, float ex, float ey);
	typedef std::list<GraphicsBase> GraphicsBaseList;
	GraphicsBaseList graphicsBases_;
};

#endif
