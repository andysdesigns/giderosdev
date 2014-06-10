#include "sprite.h"
#include "glcommon.h"
#include <algorithm>
#include <cassert>
#include <stack>
#include "color.h"
#include "blendfunc.h"
#include "stage.h"
#include <application.h>

std::set<GSprite*> GSprite::allSprites_;
std::set<GSprite*> GSprite::allSpritesWithListeners_;

GSprite::GSprite(Application* application) :
    application_(application),
    parent_(NULL),
    isVisible_(true)
{
	allSprites_.insert(this);

//	graphicsBases_.push_back(GraphicsBase());
	
	alpha_ = 1;
	colorTransform_ = 0;
//	graphics_ = 0;

	sfactor_ = -1;
	dfactor_ = -1;
}

GSprite::~GSprite()
{
	delete colorTransform_;
//	delete graphics_;
	
	for (std::size_t i = 0; i < children_.size(); ++i)
		children_[i]->unref();

	allSprites_.erase(this);
	allSpritesWithListeners_.erase(this);
}

void GSprite::doDraw(const CurrentTransform&, float sx, float sy, float ex, float ey)
{

}

/*
void Sprite::draw()
{
	if (isVisible_ == false)
		return;

	if (isWhite_ == false)
	{
		glPushColor();
		glMultColor(r_, g_, b_, a_);
	}

	if (transform_ != 0 && transform_->matrix().type() != Matrix::eIdentity)
	{
		glPushMatrix();

		switch (transform_->matrix().type())
		{
		case Matrix::eTranslationOnly:
			glTranslatef(transform_->matrix().tx(), transform_->matrix().ty(), 0);
			break;
		default:
			glMultMatrixf(transform_->matrix().data());
			break;
		}
	}

	if (sfactor_ != -1)
	{
		glPushBlendFunc();
		glSetBlendFunc(sfactor_, dfactor_);
	}

//	if (graphics_ != 0)
//		graphics_->draw();

	for (GraphicsBaseList::iterator iter = graphicsBases_.begin(), e = graphicsBases_.end(); iter != e; ++iter)
		iter->draw();
	
	for (std::size_t i = 0; i < children_.size(); ++i)
		children_[i]->draw();

	if (sfactor_ != -1)
		glPopBlendFunc();

	if (transform_ != 0 && transform_->matrix().type() != Matrix::eIdentity)
		glPopMatrix();

	if (isWhite_ == false)
		glPopColor();
}

*/

template <typename T>
class GGPool
{
public:
    ~GGPool()
    {
        for (size_t i = 0; i < v_.size(); ++i)
            delete v_[i];
    }

    T *create()
    {
        T *result;

        if (v_.empty())
        {
            result = new T;
        }
        else
        {
            result = v_.back();
            v_.pop_back();
        }

        return result;

    }

    void destroy(T *t)
    {
        v_.push_back(t);
    }

private:
    std::vector<T*> v_;
};



void GSprite::draw(const CurrentTransform& transform, float sx, float sy, float ex, float ey)
{
    {
        this->worldTransform_ = transform * this->localTransform_.matrix();

        static GGPool<std::stack<GSprite*> > stackPool;
        std::stack<GSprite*> &stack = *stackPool.create();

        for (size_t i = 0; i < children_.size(); ++i)
            stack.push(children_[i]);

        while (!stack.empty())
        {
            GSprite *sprite = stack.top();
            stack.pop();

            if (sprite->isVisible_ == false)
            {
                continue;
            }

            sprite->worldTransform_ = sprite->parent_->worldTransform_ * sprite->localTransform_.matrix();

            for (size_t i = 0; i < sprite->children_.size(); ++i)
                stack.push(sprite->children_[i]);
        }

        stackPool.destroy(&stack);
    }


    static GGPool<std::stack<std::pair<GSprite*, bool> > > stackPool;
    std::stack<std::pair<GSprite*, bool> > &stack = *stackPool.create();

	stack.push(std::make_pair(this, false));

	while (stack.empty() == false)
	{
		GSprite* sprite = stack.top().first;
		bool pop = stack.top().second;
		stack.pop();

		if (pop == true)
		{
			if (sprite->colorTransform_ != 0 || sprite->alpha_ != 1)
				glPopColor();
			if (sprite->sfactor_ != -1)
				glPopBlendFunc();
			continue;
		}

		if (sprite->isVisible_ == false)
		{
			continue;
		}

        glLoadMatrixf(sprite->worldTransform_.data());

		if (sprite->colorTransform_ != 0 || sprite->alpha_ != 1)
		{
			glPushColor();

			float r = 1, g = 1, b = 1, a = 1;

			if (sprite->colorTransform_)
			{
				r = sprite->colorTransform_->redMultiplier();
				g = sprite->colorTransform_->greenMultiplier();
				b = sprite->colorTransform_->blueMultiplier();
				a = sprite->colorTransform_->alphaMultiplier();
			}
			
			glMultColor(r, g, b, a * sprite->alpha_);
		}

		if (sprite->sfactor_ != -1)
		{
			glPushBlendFunc();
			glSetBlendFunc(sprite->sfactor_, sprite->dfactor_);
		}

        sprite->doDraw(sprite->worldTransform_, sx, sy, ex, ey);

		stack.push(std::make_pair(sprite, true));
		for (int i = (int)sprite->children_.size() - 1; i >= 0; --i)
			stack.push(std::make_pair(sprite->children_[i], false));
	}

    stackPool.destroy(&stack);
}


bool GSprite::canChildBeAdded(GSprite* sprite, GStatus* status)
{
	if (sprite == this)
	{
		if (status != 0)
			*status = GStatus(2024); // Error #2024: An object cannot be added as a child of itself.

		return false;
	}

	if (sprite->contains(this) == true)
	{
		if (status != 0)
			*status = GStatus(2150);	// Error #2150: An object cannot be added as a child to one of it's children (or children's children, etc.).

		return false;
	}

	return true;
}

bool GSprite::canChildBeAddedAt(GSprite* sprite, int index, GStatus* status)
{
	if (canChildBeAdded(sprite, status) == false)
		return false;

	if (index < 0 || index > childCount())
	{
		if (status)
			*status = GStatus(2006); // Error #2006: The supplied index is out of bounds.
		return false;
	}
	
	return true;
}

void GSprite::addChild(GSprite* sprite, GStatus* status)
{
	addChildAt(sprite, childCount(), status);
}

void GSprite::addChildAt(GSprite* sprite, int index, GStatus* status)
{
	if (canChildBeAddedAt(sprite, index, status) == false)
		return;

    Stage* stage1 = sprite->getStage();

    if (stage1)
        stage1->setSpritesWithListenersDirty();

	if (sprite->parent_ == this)
	{
		*std::find(children_.begin(), children_.end(), sprite) = NULL;
		children_.insert(children_.begin() + index, sprite);
		children_.erase(std::find(children_.begin(), children_.end(), (GSprite*)NULL));
		return;
	}

    bool connected1 = stage1 != NULL;

	sprite->ref();		// guard

	if (sprite->parent_)
	{
		SpriteVector& children = sprite->parent_->children_;
		children.erase(std::find(children.begin(), children.end(), sprite));
		sprite->unref();
	}
	sprite->parent_ = this;

	children_.insert(children_.begin() + index, sprite);
	sprite->ref();

	sprite->unref();	// unguard

    Stage *stage2 = sprite->getStage();

    if (stage2)
        stage2->setSpritesWithListenersDirty();

    bool connected2 = stage2 != NULL;

	if (connected1 && !connected2)
	{
		Event event(Event::REMOVED_FROM_STAGE);
		sprite->recursiveDispatchEvent(&event, false, false);
	}
	else if (!connected1 && connected2)
	{
		Event event(Event::ADDED_TO_STAGE);
		sprite->recursiveDispatchEvent(&event, false, false);
	}
}

/**
   Returns the index position of a child Sprite instance.
*/
int GSprite::getChildIndex(GSprite* sprite, GStatus* status)
{
	SpriteVector::iterator iter = std::find(children_.begin(), children_.end(), sprite);

	if (iter == children_.end())
	{
		if (status)
			*status = GStatus(2025); // Error #2025: The supplied Sprite must be a child of the caller.
	}
	
	return iter - children_.begin();
}

/**
   Changes the position of an existing child in the display object container.
*/
void GSprite::setChildIndex(GSprite* child, int index, GStatus* status)
{
	int currentIndex = getChildIndex(child, status);

	if (currentIndex == childCount())
		return;

	if (index < 0 || index > childCount())
	{
		if (status)
			*status = GStatus(2006); // Error #2006: The supplied index is out of bounds.
		return;
	}

	children_.erase(children_.begin() + currentIndex);
	children_.insert(children_.begin() + index, child);
}

void GSprite::swapChildren(GSprite* child1, GSprite* child2, GStatus* status)
{
	int index1 = getChildIndex(child1, status);
	if (index1 == childCount())
		return;

	int index2 = getChildIndex(child2, status);
	if (index2 == childCount())
		return;

	std::swap(children_[index1], children_[index2]);
}

void GSprite::swapChildrenAt(int index1, int index2, GStatus* status)
{
	if (index1 < 0 || index1 >= childCount())
	{
		if (status)
			*status = GStatus(2006); // Error #2006: The supplied index is out of bounds.
		return;
	}

	if (index2 < 0 || index2 >= childCount())
	{
		if (status)
			*status = GStatus(2006); // Error #2006: The supplied index is out of bounds.
		return;
	}

	std::swap(children_[index1], children_[index2]);
}

GSprite* GSprite::getChildAt(int index, GStatus* status) const
{
	if (index < 0 || index >= childCount())
	{
		if (status)
			*status = GStatus(2006); // Error #2006: The supplied index is out of bounds.
		return 0;
	}

	return children_[index];
}

void GSprite::removeChildAt(int index, GStatus* status)
{
	if (index < 0 || index >= childCount())
	{
		if (status)
			*status = GStatus(2006); // Error #2006: The supplied index is out of bounds.
		return;
	}

    void *pool = application_->createAutounrefPool();

	GSprite* child = children_[index];

    Stage *stage = child->getStage();

    if (stage)
        stage->setSpritesWithListenersDirty();

    bool connected = stage != NULL;

	child->parent_ = 0;
	children_.erase(children_.begin() + index);

    application_->autounref(child);

    if (connected)
	{
        Event event(Event::REMOVED_FROM_STAGE);
        child->recursiveDispatchEvent(&event, false, false);
	}

    application_->deleteAutounrefPool(pool);
}

void GSprite::removeChild(GSprite* child, GStatus* status)
{
	int index = getChildIndex(child, status);
	if (index == childCount())
	{
		if (status)
			*status = GStatus(2025); // Error #2025: The supplied Sprite must be a child of the caller.
		return;
	}

	removeChildAt(index);
}

void GSprite::removeChild(int index, GStatus* status)
{
	if (index < 0 || index >= childCount())
	{
		if (status)
			*status = GStatus(2025); // Error #2025: The supplied Sprite must be a child of the caller.
		return;
	}

	removeChildAt(index);
}

bool GSprite::contains(GSprite* sprite) const
{
	std::stack<const GSprite*> stack;
	stack.push(this);

	while (stack.empty() == false)
	{
		const GSprite* s = stack.top();
		stack.pop();

		if (s == sprite)
			return true;

		for (int i = 0; i < s->childCount(); ++i)
			stack.push(s->child(i));
	}

	return false;
}

void GSprite::replaceChild(GSprite* oldChild, GSprite* newChild)
{
	// TODO: burada addedToStage ile removedFromStage'i halletmek lazim
	SpriteVector::iterator iter = std::find(children_.begin(), children_.end(), oldChild);

	assert(iter != children_.end());

	if (oldChild == newChild)
		return;

	oldChild->parent_ = 0;

	newChild->ref();
	oldChild->unref();
	*iter = newChild;

	newChild->parent_ = this;
}

void GSprite::localToGlobal(float x, float y, float* tx, float* ty) const
{
	const GSprite* curr = this;

	while (curr)
	{
		curr->matrix().transformPoint(x, y, &x, &y);
		curr = curr->parent_;
	}

	if (tx)
		*tx = x;

	if (ty)
		*ty = y;
}

void GSprite::globalToLocal(float x, float y, float* tx, float* ty) const
{
	std::stack<const GSprite*> stack;

	const GSprite* curr = this;
	while (curr)
	{
		stack.push(curr);
		curr = curr->parent_;
	}

	while (stack.empty() == false)
	{
		stack.top()->matrix().inverseTransformPoint(x, y, &x, &y);
		stack.pop();
	}

	if (tx)
		*tx = x;

	if (ty)
		*ty = y;
}

void GSprite::objectBounds(float* minx, float* miny, float* maxx, float* maxy) const
{
    boundsHelper(Matrix(), minx, miny, maxx, maxy);
}

#if 0
void Sprite::objectBounds(float* minx, float* miny, float* maxx, float* maxy) const
{
	Point2f min( 1e30f,  1e30f);
	Point2f max(-1e30f, -1e30f);

//	if (graphics_ != 0)
//		graphics_->bounds(&min, &max);

	float eminx, eminy, emaxx, emaxy;
	extraBounds(&eminx, &eminy, &emaxx, &emaxy);
	min.x = std::min(min.x, eminx);
	min.y = std::min(min.y, eminy);
	max.x = std::max(max.x, emaxx);
	max.y = std::max(max.y, emaxy);

	for (std::size_t i = 0; i < children_.size(); ++i)
	{
		float cminx, cminy, cmaxx, cmaxy;
		children_[i]->localBounds(&cminx, &cminy, &cmaxx, &cmaxy);

		min.x = std::min(min.x, cminx);
		min.y = std::min(min.y, cminy);
		max.x = std::max(max.x, cmaxx);
		max.y = std::max(max.y, cmaxy);
	}

	if (minx)
		*minx = min.x;
	if (miny)
		*miny = min.y;
	if (maxx)
		*maxx = max.x;
	if (maxy)
		*maxy = max.y;
}
#endif

void GSprite::localBounds(float* minx, float* miny, float* maxx, float* maxy) const
{
    boundsHelper(localTransform_.matrix(), minx, miny, maxx, maxy);
}


#if 0
void Sprite::localBounds(float* minx, float* miny, float* maxx, float* maxy) const
{
	float ominx, ominy, omaxx, omaxy;
	objectBounds(&ominx, &ominy, &omaxx, &omaxy);

	float lminx, lminy, lmaxx, lmaxy;

	if (ominx > omaxx || ominy > omaxy)
	{
		// empty bounds, dont transform
		lminx = ominx;
		lminy = ominy;
		lmaxx = omaxx;
		lmaxy = omaxy;
	}
	else
	{
		// transform 4 corners
		float x[4], y[4];
		matrix().transformPoint(ominx, ominy, &x[0], &y[0]);
		matrix().transformPoint(omaxx, ominy, &x[1], &y[1]);
		matrix().transformPoint(omaxx, omaxy, &x[2], &y[2]);
		matrix().transformPoint(ominx, omaxy, &x[3], &y[3]);

		// calculate local bounding box
		lminx = lmaxx = x[0];
		lminy = lmaxy = y[0];
		for (int i = 1; i < 4; ++i)
		{
			lminx = std::min(lminx, x[i]);
			lminy = std::min(lminy, y[i]);
			lmaxx = std::max(lmaxx, x[i]);
			lmaxy = std::max(lmaxy, y[i]);
		}
	}

	if (minx)
		*minx = lminx;
	if (miny)
		*miny = lminy;
	if (maxx)
		*maxx = lmaxx;
	if (maxy)
		*maxy = lmaxy;
}
#endif

#if 0
void Sprite::globalBounds(float* minx, float* miny, float* maxx, float* maxy) const
{
	float ominx, ominy, omaxx, omaxy;
	objectBounds(&ominx, &ominy, &omaxx, &omaxy);

	float gminx, gminy, gmaxx, gmaxy;

	if (ominx > omaxx || ominy > omaxy)
	{
		// empty bounds, dont transform
		gminx = ominx;
		gminy = ominy;
		gmaxx = omaxx;
		gmaxy = omaxy;
	}
	else
	{
		// transform 4 corners
		float x[4], y[4];
		localToGlobal(ominx, ominy, &x[0], &y[0]);
		localToGlobal(omaxx, ominy, &x[1], &y[1]);
		localToGlobal(omaxx, omaxy, &x[2], &y[2]);
		localToGlobal(ominx, omaxy, &x[3], &y[3]);

		// calculate global bounding box
		gminx = gmaxx = x[0];
		gminy = gmaxy = y[0];
		for (int i = 1; i < 4; ++i)
		{
			gminx = std::min(gminx, x[i]);
			gminy = std::min(gminy, y[i]);
			gmaxx = std::max(gmaxx, x[i]);
			gmaxy = std::max(gmaxy, y[i]);
		}
	}

	if (minx)
		*minx = gminx;
	if (miny)
		*miny = gminy;
	if (maxx)
		*maxx = gmaxx;
	if (maxy)
		*maxy = gmaxy;
}
#endif

float GSprite::width() const
{
	float minx, maxx;
	localBounds(&minx, 0, &maxx, 0);

	if (minx > maxx)
		return 0;

	return maxx - minx;
}

float GSprite::height() const
{
	float miny, maxy;
	localBounds(0, &miny, 0, &maxy);

	if (miny > maxy)
		return 0;

	return maxy - miny;
}

bool GSprite::hitTestPoint(float x, float y) const
{
	float tx, ty;
	globalToLocal(x, y, &tx, &ty);

	float minx, miny, maxx, maxy;
	objectBounds(&minx, &miny, &maxx, &maxy);

	return (tx >= minx && ty >= miny && tx <= maxx && ty <= maxy);
}

Stage *GSprite::getStage() const
{
    const GSprite* curr = this;

    while (curr != NULL)
	{
		if (curr->isStage() == true)
            return static_cast<Stage*>(const_cast<GSprite*>(curr));

		curr = curr->parent();
	}

    return NULL;
}


void GSprite::recursiveDispatchEvent(Event* event, bool canBeStopped, bool reverse)
{
    void *pool = application_->createAutounrefPool();

	std::vector<GSprite*> sprites;		// NOTE: bunu static yapma. recursiveDispatchEvent icindeyken recursiveDispatchEvent cagirilabiliyor

	std::stack<GSprite*> stack;

	stack.push(this);

	while (stack.empty() == false)
	{
		GSprite* sprite = stack.top();
		stack.pop();

		sprites.push_back(sprite);

		for (int i = sprite->childCount() - 1; i >= 0; --i)
			stack.push(sprite->child(i));
	}

	if (reverse == true)
		std::reverse(sprites.begin(), sprites.end());

	for (std::size_t i = 0; i < sprites.size(); ++i)
    {
		sprites[i]->ref();
        application_->autounref(sprites[i]);
    }
	
    for (std::size_t i = 0; i < sprites.size(); ++i)
    {
        if (canBeStopped == false || event->propagationStopped() == false)
            sprites[i]->dispatchEvent(event);
        else
            break;
    }

    application_->deleteAutounrefPool(pool);
}

float GSprite::alpha() const
{
	return alpha_;
}

void GSprite::boundsHelper(const Matrix& transform, float* minx, float* miny, float* maxx, float* maxy) const
{
    {
        this->worldTransform_ = transform;
        std::stack<GSprite*> stack; // this shouldn't be static because MovieClip calls draw again
        for (size_t i = 0; i < children_.size(); ++i)
            stack.push(children_[i]);

        while (!stack.empty())
        {
            GSprite *sprite = stack.top();
            stack.pop();

            sprite->worldTransform_ = sprite->parent_->worldTransform_ * sprite->localTransform_.matrix();

            for (size_t i = 0; i < sprite->children_.size(); ++i)
                stack.push(sprite->children_[i]);
        }
    }

    {
        float gminx = 1e30, gminy = 1e30, gmaxx = -1e30, gmaxy = -1e30;

        std::stack<const GSprite*> stack; // this shouldn't be static because MovieClip calls draw again
        stack.push(this);

        while (!stack.empty())
        {
            const GSprite *sprite = stack.top();
            stack.pop();

            float eminx, eminy, emaxx, emaxy;
            sprite->extraBounds(&eminx, &eminy, &emaxx, &emaxy);

            if (eminx <= emaxx && eminy <= emaxy)
            {
                float x[4], y[4];
                sprite->worldTransform_.transformPoint(eminx, eminy, &x[0], &y[0]);
                sprite->worldTransform_.transformPoint(emaxx, eminy, &x[1], &y[1]);
                sprite->worldTransform_.transformPoint(emaxx, emaxy, &x[2], &y[2]);
                sprite->worldTransform_.transformPoint(eminx, emaxy, &x[3], &y[3]);

                for (int i = 0; i < 4; ++i)
                {
                    gminx = std::min(gminx, x[i]);
                    gminy = std::min(gminy, y[i]);
                    gmaxx = std::max(gmaxx, x[i]);
                    gmaxy = std::max(gmaxy, y[i]);
                }
            }

            for (size_t i = 0; i < sprite->children_.size(); ++i)
                stack.push(sprite->children_[i]);
        }

        if (minx)
            *minx = gminx;
        if (miny)
            *miny = gminy;
        if (maxx)
            *maxx = gmaxx;
        if (maxy)
            *maxy = gmaxy;
    }
}

void GSprite::getBounds(const GSprite* targetCoordinateSpace, float* minx, float* miny, float* maxx, float* maxy) const
{
    bool found = false;
    Matrix transform;
    const GSprite *curr = this;
    while (curr)
    {
        if (curr == targetCoordinateSpace)
        {
            found = true;
            break;
        }
        transform = curr->localTransform_.matrix() * transform;
        curr = curr->parent_;
    }

    if (found == false)
    {
        Matrix inverse;
        const GSprite *curr = targetCoordinateSpace;
        while (curr)
        {
            inverse  = inverse * curr->localTransform_.matrix().inverse();
            curr = curr->parent_;
        }

        transform = inverse * transform;
    }

    boundsHelper(transform, minx, miny, maxx, maxy);
}

#if 0
void Sprite::getBounds(const Sprite* targetCoordinateSpace, float* minx, float* miny, float* maxx, float* maxy)
{
	if (targetCoordinateSpace == this)
	{
		// optimization
		objectBounds(minx, miny, maxx, maxy);
		return;
	}

	float ominx, ominy, omaxx, omaxy;
	objectBounds(&ominx, &ominy, &omaxx, &omaxy);

	float gminx, gminy, gmaxx, gmaxy;

	if (ominx > omaxx || ominy > omaxy)
	{
		// empty bounds, dont transform
		gminx = ominx;
		gminy = ominy;
		gmaxx = omaxx;
		gmaxy = omaxy;
	}
	else
	{
		// transform 4 corners
		float x[4], y[4];
		this->localToGlobal(ominx, ominy, &gminx, &gminy);
		this->localToGlobal(omaxx, ominy, &gmaxx, &gminy);
		this->localToGlobal(omaxx, omaxy, &gmaxx, &gmaxy);
		this->localToGlobal(ominx, omaxy, &gminx, &gmaxy);

		targetCoordinateSpace->globalToLocal(gminx, gminy, &x[0], &y[0]);
		targetCoordinateSpace->globalToLocal(gmaxx, gminy, &x[1], &y[1]);
		targetCoordinateSpace->globalToLocal(gmaxx, gmaxy, &x[2], &y[2]);
		targetCoordinateSpace->globalToLocal(gminx, gmaxy, &x[3], &y[3]);

		// calculate bounding box
		gminx = gmaxx = x[0];
		gminy = gmaxy = y[0];
		for (int i = 1; i < 4; ++i)
		{
			gminx = std::min(gminx, x[i]);
			gminy = std::min(gminy, y[i]);
			gmaxx = std::max(gmaxx, x[i]);
			gmaxy = std::max(gmaxy, y[i]);
		}
	}

	if (minx)
		*minx = gminx;
	if (miny)
		*miny = gminy;
	if (maxx)
		*maxx = gmaxx;
	if (maxy)
		*maxy = gmaxy;
}
#endif

GLenum blendFactor2GLenum(GSprite::BlendFactor blendFactor)
{
	switch (blendFactor)
	{
		case GSprite::ZERO:
		   return GL_ZERO;
		case GSprite::ONE:
		   return GL_ONE;
		case GSprite::SRC_COLOR:
		   return GL_SRC_COLOR;
		case GSprite::ONE_MINUS_SRC_COLOR:
		   return GL_ONE_MINUS_SRC_COLOR;
		case GSprite::DST_COLOR:
		   return GL_DST_COLOR;
		case GSprite::ONE_MINUS_DST_COLOR:
		   return GL_ONE_MINUS_DST_COLOR;
		case GSprite::SRC_ALPHA:
		   return GL_SRC_ALPHA;
		case GSprite::ONE_MINUS_SRC_ALPHA:
		   return GL_ONE_MINUS_SRC_ALPHA;
		case GSprite::DST_ALPHA:
		   return GL_DST_ALPHA;
		case GSprite::ONE_MINUS_DST_ALPHA:
		   return GL_ONE_MINUS_DST_ALPHA;
		//case Sprite::CONSTANT_COLOR:
		//   return GL_CONSTANT_COLOR;
		//case Sprite::ONE_MINUS_CONSTANT_COLOR:
		//   return GL_ONE_MINUS_CONSTANT_COLOR;
		//case Sprite::CONSTANT_ALPHA:
		//   return GL_CONSTANT_ALPHA;
		//case Sprite::ONE_MINUS_CONSTANT_ALPHA:
		//   return GL_ONE_MINUS_CONSTANT_ALPHA;
		case GSprite::SRC_ALPHA_SATURATE:
		   return GL_SRC_ALPHA_SATURATE;
	}

	return GL_ZERO;
}

void GSprite::setBlendFunc(BlendFactor sfactor, BlendFactor dfactor)
{
	sfactor_ = blendFactor2GLenum(sfactor);
	dfactor_ = blendFactor2GLenum(dfactor);
}


void GSprite::clearBlendFunc()
{
	sfactor_ = -1;
	dfactor_ = -1;
}

void GSprite::setColorTransform(const ColorTransform& colorTransform)
{
	if (colorTransform_ == 0)
		colorTransform_ = new ColorTransform();

	*colorTransform_ = colorTransform;
}

void GSprite::setAlpha(float alpha)
{
	alpha_ = alpha;
}

void GSprite::eventListenersChanged()
{
    Stage *stage = getStage();
    if (stage)
        stage->setSpritesWithListenersDirty();

	if (hasEventListener(Event::ENTER_FRAME))
		allSpritesWithListeners_.insert(this);
	else
		allSpritesWithListeners_.erase(this);
}

void GSprite::set(const char* param, float value, GStatus* status)
{
	int id = StringId::instance().id(param);
	set(id, value, status);
}

float GSprite::get(const char* param, GStatus* status)
{
	int id = StringId::instance().id(param);
	return get(id, status);
}

void GSprite::set(int param, float value, GStatus* status)
{
	switch (param)
	{
	case eStringIdX:
		setX(value);
		break;
	case eStringIdY:
		setY(value);
		break;
	case eStringIdRotation:
		setRotation(value);
		break;
	case eStringIdScale:
		setScale(value);
		break;
	case eStringIdScaleX:
		setScaleX(value);
		break;
	case eStringIdScaleY:
		setScaleY(value);
		break;
	case eStringIdAlpha:
		setAlpha(value);
		break;
    case eStringIdRedMultiplier:
        setRedMultiplier(value);
        break;
    case eStringIdGreenMultiplier:
        setGreenMultiplier(value);
        break;
    case eStringIdBlueMultiplier:
        setBlueMultiplier(value);
        break;
    case eStringIdAlphaMultiplier:
        setAlphaMultiplier(value);
        break;
	default:
		if (status)
			*status = GStatus(2008, "param"); // Error #2008: Parameter '%s' must be one of the accepted values.
		break;
	}
}

float GSprite::get(int param, GStatus* status)
{
	switch (param)
	{
	case eStringIdX:
		return x();
	case eStringIdY:
		return y();
	case eStringIdRotation:
		return rotation();
	case eStringIdScaleX:
		return scaleX();
	case eStringIdScaleY:
		return scaleY();
	case eStringIdAlpha:
		return alpha();
    case eStringIdRedMultiplier:
        return getRedMultiplier();
    case eStringIdGreenMultiplier:
        return getGreenMultiplier();
    case eStringIdBlueMultiplier:
        return getBlueMultiplier();
    case eStringIdAlphaMultiplier:
        return getAlphaMultiplier();
	}

	if (status)
		*status = GStatus(2008, "param"); // Error #2008: Parameter '%s' must be one of the accepted values.

	return 0;
}


void GSprite::setRedMultiplier(float redMultiplier)
{
    if (colorTransform_ == NULL)
        colorTransform_ = new ColorTransform();

    colorTransform_->setRedMultiplier(redMultiplier);
}

void GSprite::setGreenMultiplier(float greenMultiplier)
{
    if (colorTransform_ == NULL)
        colorTransform_ = new ColorTransform();

    colorTransform_->setGreenMultiplier(greenMultiplier);
}

void GSprite::setBlueMultiplier(float blueMultiplier)
{
    if (colorTransform_ == NULL)
        colorTransform_ = new ColorTransform();

    colorTransform_->setBlueMultiplier(blueMultiplier);
}

void GSprite::setAlphaMultiplier(float alphaMultiplier)
{
    if (colorTransform_ == NULL)
        colorTransform_ = new ColorTransform();

    colorTransform_->setAlphaMultiplier(alphaMultiplier);
}

float GSprite::getRedMultiplier() const
{
    if (colorTransform_ == NULL)
        colorTransform_ = new ColorTransform();

    return colorTransform_->redMultiplier();
}

float GSprite::getGreenMultiplier() const
{
    if (colorTransform_ == NULL)
        colorTransform_ = new ColorTransform();

    return colorTransform_->greenMultiplier();
}

float GSprite::getBlueMultiplier() const
{
    if (colorTransform_ == NULL)
        colorTransform_ = new ColorTransform();

    return colorTransform_->blueMultiplier();
}

float GSprite::getAlphaMultiplier() const
{
    if (colorTransform_ == NULL)
        colorTransform_ = new ColorTransform();

    return colorTransform_->alphaMultiplier();
}

