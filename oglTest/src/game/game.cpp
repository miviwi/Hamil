#include <game/game.h>
#include <game/cursor.h>

namespace game {

void init()
{
  CursorDriver::init();
}

void finalize()
{
  CursorDriver::finalize();
}

}