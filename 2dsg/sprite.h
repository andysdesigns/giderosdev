#ifndef SPRITE_H
#define SPRITE_H

#include "eventdispatcher.h"
#include "transform.h"
//#include "graphics.h"
#include "colortransform.h"
#include "gstatus.h"
#include "graphicsbase.h"
#include <list>

typedef Matrix CurrentTransform;

class Application;
class Stage;

class GSprite : public EventDispatcher
{
public:
    GSprite(Application* application);
    virtual ~GSprite();

    void draw(const CurrentTransform&, float sx, float sy, float ex, float ey);

    void addChild(GSprite* sprite, GStatus* status = 0);
    void removeChild(GSprite* child, GStatus* status = 0);
	void removeChild(int index, GStatus* status = 0);
    bool contains(GSprite* sprite) const;
    void replaceChild(GSprite* oldChild, GSprite* newChild);
    bool canChildBeAdded(GSprite* sprite, GStatus* status = 0);
    void addChildAt(GSprite* sprite, int index, GStatus* status = 0);
    bool canChildBeAddedAt(GSprite* sprite, int index, GStatus* status = 0);
    int getChildIndex(GSprite* sprite, GStatus* status = 0);
    void setChildIndex(GSprite* child, int index, GStatus* status = 0);
    void swapChildren(GSprite* child1, GSprite* child2, GStatus* status = 0);
	void swapChildrenAt(int index1, int index2, GStatus* status = 0);
	void removeChildAt(int index, GStatus* status = 0);
	
	void localToGlobal(float x, float y, float* tx, float* ty) const;
	void globalToLocal(float x, float y, float* tx, float* ty) const;

	void setAlpha(float alpha);
	float alpha() const;
	
	int childCount() const
	{
		return (int)children_.size();
	}

    GSprite* child(int index) const
	{
		return children_[index];
	}

    GSprite* getChildAt(int index, GStatus* status = 0) const;

    GSprite* parent() const
	{
		return parent_;
	}

	const ColorTransform& colorTransform() const
	{
		if (colorTransform_ == 0)
			colorTransform_ = new ColorTransform();
		
		return *colorTransform_;
	}

	void setColorTransform(const ColorTransform& colorTransform);

    void setRedMultiplier(float redMultiplier);
    void setGreenMultiplier(float greenMultiplier);
    void setBlueMultiplier(float blueMultiplier);
    void setAlphaMultiplier(float alphaMultiplier);

    float getRedMultiplier() const;
    float getGreenMultiplier() const;
    float getBlueMultiplier() const;
    float getAlphaMultiplier() const;

	void setRotation(float r)
	{
        localTransform_.setRotation(r);
	}

	void setScaleX(float sx)
	{
        localTransform_.setScaleX(sx);
	}

	void setScaleY(float sy)
	{
        localTransform_.setScaleY(sy);
	}

	void setScaleXY(float sx, float sy)
	{
        localTransform_.setScaleXY(sx, sy);
	}

	void setScale(float s)
	{
        localTransform_.setScaleXY(s, s);
	}

	void setX(float x)
	{
        localTransform_.setX(x);
	}

	void setY(float y)
	{
        localTransform_.setY(y);
	}

	void setXY(float x, float y)
	{
        localTransform_.setXY(x, y);
	}

	float rotation() const
	{
        return localTransform_.rotation();
	}

	float scaleX() const
	{
        return localTransform_.scaleX();
	}

	float scaleY() const
	{
        return localTransform_.scaleY();
	}

	float x() const
	{
        return localTransform_.x();
	}

	float y() const
	{
        return localTransform_.y();
	}

    void setMatrix(float m11, float m12, float m21, float m22, float tx, float ty)
	{
        localTransform_.setMatrix(m11, m12, m21, m22, tx, ty);
	}

	void setMatrix(const Matrix& matrix)
	{
        localTransform_.setMatrix(matrix);
	}

	const Matrix& matrix() const
	{
        return localTransform_.matrix();
	}

	// Gets the bounds of Sprite in its own coordinate system
	void objectBounds(float* minx, float* miny, float* maxx, float* maxy) const;

	// Gets the bounds of Sprite after transformed by its Matrix.
	// localBounds = Matrix * objectBounds
	void localBounds(float* minx, float* miny, float* maxx, float* maxy) const;
	
#if 0
	// Gets the bounds of Sprite after transformed by localToGlobal
	void globalBounds(float* minx, float* miny, float* maxx, float* maxy) const;
#endif

	// Indicates the width of the sprite, in pixels.
	// The width is calculated based on the local bounds of the content of the sprite.
	float width() const;

	// Indicates the height of the sprite, in pixels.
	// The height is calculated based on the local bounds of the content of the sprite.
	float height() const;

	// Gets whether or not the sprite is visible.
	bool visible() const
	{
		return isVisible_;
	}

	// Sets whether or not the sprite is visible.
	void setVisible(bool visible)
	{
		isVisible_ = visible;
	}

	// Evaluates the sprite to see if its bounds overlaps or intersects with the point specified by the x and y parameters.
	// The x and y parameters specify a point in the global coordinate space.
	bool hitTestPoint(float x, float y) const;
	
	virtual bool isStage() const
	{
		return false;
	}

    static const std::set<GSprite*>& allSprites()
	{
		return allSprites_;
	}

    void getBounds(const GSprite* targetCoordinateSpace, float* minx, float* miny, float* maxx, float* maxy) const;

	enum BlendFactor
	{
		NONE,
		ZERO,
		ONE,
		SRC_COLOR,
		ONE_MINUS_SRC_COLOR,
		DST_COLOR,
		ONE_MINUS_DST_COLOR,
		SRC_ALPHA,
		ONE_MINUS_SRC_ALPHA,
		DST_ALPHA,
		ONE_MINUS_DST_ALPHA,
		//CONSTANT_COLOR,
		//ONE_MINUS_CONSTANT_COLOR,
		//CONSTANT_ALPHA,
		//ONE_MINUS_CONSTANT_ALPHA,
		SRC_ALPHA_SATURATE,
	};

	void clearBlendFunc();
	void setBlendFunc(BlendFactor sfactor, BlendFactor dfactor);

	void set(const char* param, float value, GStatus* status = NULL);
	float get(const char* param, GStatus* status = NULL);
	void set(int param, float value, GStatus* status = NULL);
	float get(int param, GStatus* status = NULL);

	const Transform& transform() const
	{
        return localTransform_;
	}

    void setTransform(const Transform& transform)
	{
        localTransform_ = transform;
	}



protected:
	virtual void extraBounds(float* minx, float* miny, float* maxx, float* maxy) const
	{
		if (minx)
			*minx =  1e30f;
		if (miny)
			*miny =  1e30f;
		if (maxx)
			*maxx = -1e30f;
		if (maxy)
			*maxy = -1e30f;
	}

protected:
	void recursiveDispatchEvent(Event* event, bool canBeStopped, bool reverse);

private:
    Stage *getStage() const;
    void boundsHelper(const Matrix& transform, float* minx, float* miny, float* maxx, float* maxy) const;

protected:
    Application *application_;

private:
	bool isVisible_;
    Transform localTransform_;
    mutable Matrix worldTransform_;
//	Graphics* graphics_;

	unsigned int sfactor_, dfactor_;

    typedef std::vector<GSprite*> SpriteVector;
	SpriteVector children_;
    GSprite* parent_;

	mutable ColorTransform* colorTransform_;
	float alpha_;

private:
    virtual void doDraw(const CurrentTransform&, float sx, float sy, float ex, float ey);

protected:
    static std::set<GSprite*> allSprites_;
    static std::set<GSprite*> allSpritesWithListeners_;

protected:
//	typedef std::list<GraphicsBase, Gideros::STLAllocator<GraphicsBase, StdAllocator> > GraphicsBaseList;
//	GraphicsBaseList graphicsBases_;

protected:
	virtual void eventListenersChanged();
};

#endif

