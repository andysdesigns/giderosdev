#include "bitmap.h"
#include "ogl.h"

void GBitmap::doDraw(const CurrentTransform&, float sx, float sy, float ex, float ey)
{
	graphicsBase_.draw();
}

void GBitmap::updateBounds()
{
    if (bitmapdata_ != NULL)
    {
        float sizescalex = bitmapdata_->texture()->sizescalex;
        float sizescaley = bitmapdata_->texture()->sizescaley;

        minx_ = sizescalex * dx_;
        miny_ = sizescaley * dy_;
        maxx_ = sizescalex * (bitmapdata_->width + bitmapdata_->dx1 + bitmapdata_->dx2 + dx_);
        maxy_ = sizescaley * (bitmapdata_->height + bitmapdata_->dy1 + bitmapdata_->dy2 + dy_);
    }
    else if (texturebase_ != NULL)
    {
        float sizescalex = texturebase_->sizescalex;
        float sizescaley = texturebase_->sizescaley;

        minx_ = sizescalex * dx_;
        miny_ = sizescaley * dy_;
        maxx_ = sizescalex * (texturebase_->data->baseWidth + dx_);
        maxy_ = sizescaley * (texturebase_->data->baseHeight + dy_);
    }
}

void GBitmap::extraBounds(float* minx, float* miny, float* maxx, float* maxy) const
{
    if (minx)
        *minx = minx_;
    if (miny)
        *miny = miny_;
    if (maxx)
        *maxx = maxx_;
    if (maxy)
        *maxy = maxy_;
}

void GBitmap::setCoords()
{
	if (bitmapdata_ != NULL)
	{
		graphicsBase_.data = bitmapdata_->texture()->data;

		graphicsBase_.mode = GL_TRIANGLE_STRIP;

        float sizescalex = bitmapdata_->texture()->sizescalex;
        float sizescaley = bitmapdata_->texture()->sizescaley;

		graphicsBase_.vertices.resize(4);
        graphicsBase_.vertices[0] = Point2f(sizescalex * (bitmapdata_->dx1 + dx_),                      sizescaley * (bitmapdata_->dy1 + dy_));
        graphicsBase_.vertices[1] = Point2f(sizescalex * (bitmapdata_->dx1 + bitmapdata_->width + dx_),	sizescaley * (bitmapdata_->dy1 + dy_));
        graphicsBase_.vertices[2] = Point2f(sizescalex * (bitmapdata_->dx1 + bitmapdata_->width + dx_),	sizescaley * (bitmapdata_->dy1 + bitmapdata_->height + dy_));
        graphicsBase_.vertices[3] = Point2f(sizescalex * (bitmapdata_->dx1 + dx_),                      sizescaley * (bitmapdata_->dy1 + bitmapdata_->height + dy_));

		graphicsBase_.texcoords.resize(4);
		graphicsBase_.texcoords[0] = Point2f(bitmapdata_->u0, bitmapdata_->v0);
		graphicsBase_.texcoords[1] = Point2f(bitmapdata_->u1, bitmapdata_->v0);
		graphicsBase_.texcoords[2] = Point2f(bitmapdata_->u1, bitmapdata_->v1);
		graphicsBase_.texcoords[3] = Point2f(bitmapdata_->u0, bitmapdata_->v1);

		graphicsBase_.indices.resize(4);
		graphicsBase_.indices[0] = 0;
		graphicsBase_.indices[1] = 1;
		graphicsBase_.indices[2] = 3;
		graphicsBase_.indices[3] = 2;
	}
	else if (texturebase_ != NULL)
	{
		graphicsBase_.data = texturebase_->data;

		graphicsBase_.mode = GL_TRIANGLE_STRIP;

        float sizescalex = texturebase_->sizescalex;
        float sizescaley = texturebase_->sizescaley;

		graphicsBase_.vertices.resize(4);
        graphicsBase_.vertices[0] = Point2f(sizescalex * dx_,                                   sizescaley * dy_);
        graphicsBase_.vertices[1] = Point2f(sizescalex * (texturebase_->data->baseWidth + dx_), sizescaley * dy_);
        graphicsBase_.vertices[2] = Point2f(sizescalex * (texturebase_->data->baseWidth + dx_), sizescaley * (texturebase_->data->baseHeight + dy_));
        graphicsBase_.vertices[3] = Point2f(sizescalex * dx_,                                   sizescaley * (texturebase_->data->baseHeight + dy_));

		float u = (float)texturebase_->data->width / (float)texturebase_->data->exwidth;
		float v = (float)texturebase_->data->height / (float)texturebase_->data->exheight;

		graphicsBase_.texcoords.resize(4);
		graphicsBase_.texcoords[0] = Point2f(0, 0);
		graphicsBase_.texcoords[1] = Point2f(u, 0);
		graphicsBase_.texcoords[2] = Point2f(u, v);
		graphicsBase_.texcoords[3] = Point2f(0, v);

		graphicsBase_.indices.resize(4);
		graphicsBase_.indices[0] = 0;
		graphicsBase_.indices[1] = 1;
		graphicsBase_.indices[2] = 3;
		graphicsBase_.indices[3] = 2;
	}
}

void GBitmap::setAnchorPoint(float x, float y)
{
	anchorx_ = x;
	anchory_ = y;

	if (bitmapdata_ != NULL)
	{
		float rx = anchorx_ * (bitmapdata_->width + bitmapdata_->dx1 + bitmapdata_->dx2);
		float ry = anchory_ * (bitmapdata_->height + bitmapdata_->dy1 + bitmapdata_->dy2);
#if 0
		dx_ = floor(-rx + 0.5f);
		dy_ = floor(-ry + 0.5f);
#else
        dx_ = -rx;
        dy_ = -ry;
#endif
	}
	else if (texturebase_ != NULL)
	{
		float rx = anchorx_ * texturebase_->data->baseWidth;
		float ry = anchory_ * texturebase_->data->baseHeight;
#if 0
		dx_ = floor(-rx + 0.5f);
		dy_ = floor(-ry + 0.5f);										// NOTE: -floor(ry + 0.5f) also gives the same result
#else
        dx_ = -rx;
        dy_ = -ry;
#endif
	}

	setCoords();
    updateBounds();
}

void GBitmap::getAnchorPoint(float* x, float* y) const
{
	if (x)
		*x = anchorx_;
	if (y)
		*y = anchory_;
}

void GBitmap::setTextureRegion(GTextureRegion *bitmapdata)
{
    GTextureRegion *originalbitmapdata = bitmapdata_;
    TextureBase *originaltexturebase = texturebase_;

    bitmapdata_ = bitmapdata;
    bitmapdata_->ref();
    texturebase_ = NULL;

    setAnchorPoint(anchorx_, anchory_);  // here setAnchorPoint updates geometry and bounds

    if (originalbitmapdata)
        originalbitmapdata->unref();
    if (originaltexturebase)
        originaltexturebase->unref();
}

void GBitmap::setTexture(TextureBase *texturebase)
{
    GTextureRegion *originalbitmapdata = bitmapdata_;
    TextureBase *originaltexturebase = texturebase_;

    bitmapdata_ = NULL;
    texturebase_ = texturebase;
    texturebase_->ref();

    setAnchorPoint(anchorx_, anchory_);  // here setAnchorPoint updates geometry and bounds

    if (originalbitmapdata)
        originalbitmapdata->unref();
    if (originaltexturebase)
        originaltexturebase->unref();
}
