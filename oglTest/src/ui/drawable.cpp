#include <ui/drawable.h>
#include <ft/font.h>

namespace ui {

struct pDrawable {
  Drawable::Type type;
};

struct pDrawableText : public pDrawable {
  ft::Font::Ptr font;
  ft::String string;
};

struct pDrawableImage : public pDrawable {
};

Drawable::Drawable(pDrawable *p) :
  m(p)
{
}

Drawable::~Drawable()
{
  if(!deref()) delete m;
}

pDrawable *Drawable::get() const
{
  return m;
}

}