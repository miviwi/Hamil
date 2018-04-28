#pragma once

#include <ui/uicommon.h>
#include <util/ref.h>

namespace ui {

struct pDrawable;

class Drawable : public Ref {
public:
  enum Type {
    Invalid,

    Text,
    Image,
  };

  ~Drawable();

private:
  friend class DrawableManager;

  Drawable(pDrawable *p);

  pDrawable *operator->() const { return get(); }
  pDrawable *get() const;

  pDrawable *m;
};

class DrawableManager {
public:

};

}