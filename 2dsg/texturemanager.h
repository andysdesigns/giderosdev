#ifndef _TEXTUREMANAGER_H_
#define _TEXTUREMANAGER_H_

//#include <gtexture.h>
#include <map>
#include <vector>
#include <ggl.h>

class Application;
class TextureData;
class Dib;

enum Filter
{
    eNearest,
    eLinear,
};

enum Wrap
{
    eClamp,
    eRepeat,
};

enum Format
{
    eRGBA8888,
    eRGB888,
    eRGB565,
    eRGBA4444,
    eRGBA5551,
};

struct TextureParameters
{
    TextureParameters()
    {
        filter = eNearest;
        wrap = eClamp;
        format = eRGBA8888;
        maketransparent = false;
        transparentcolor = 0x00000000;
        grayscale = false;
    }

    Filter filter;
    Wrap wrap;
    Format format;
    bool maketransparent;
    unsigned int transparentcolor;
    bool grayscale;
};

struct TextureData
{
    int refcount;
    std::vector<char> sig;

    GLuint id();
    GLuint gid;

    TextureParameters parameters;
    int width, height;
    int exwidth, exheight;
    int baseWidth, baseHeight;
};


class TextureManager
{
public:
    TextureManager(Application* application);
    ~TextureManager();

    TextureData* createTextureFromFile(const char* filename, const TextureParameters& parameters);
    TextureData* createTextureFromDib(const Dib& dib, const TextureParameters& parameters);
    TextureData* createRenderTarget(int width, int height, const TextureParameters& parameters);
    void destroyTexture(TextureData* texture);

private:
    Application *application_;

    std::map<std::vector<char>, TextureData*> textureCache_;
};


#endif
