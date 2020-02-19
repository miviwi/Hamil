#include "glcorearb.h"
#include <sysv/glcontext.h>
#include <sysv/x11.h>
#include <sysv/window.h>

#include <os/window.h>
#include <os/panic.h>

#include <config>

#include <cassert>

// X11/xcb headers
#if __sysv
#  include <xcb/xcb.h>
#  include <X11/Xlib.h>
#endif

// OpenGL headers
#if __sysv
#  include <GL/gl.h>
#  include <GL/glx.h>
#  include <GL/gl3w.h>
#endif

namespace sysv {

using x11_detail::x11;

static int GLX_VisualAttribs[] = {
#if __sysv
  GLX_X_RENDERABLE, True,
  GLX_X_VISUAL_TYPE, GLX_TRUE_COLOR,

  GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,

  GLX_RENDER_TYPE, GLX_RGBA_BIT,
  GLX_RED_SIZE,   8,
  GLX_GREEN_SIZE, 8,
  GLX_BLUE_SIZE,  8,
  GLX_ALPHA_SIZE, 8,

//  GLX_DEPTH_SIZE, 24,
//  GLX_STENCIL_SIZE, 8,

  GLX_DOUBLEBUFFER, True,

  None,
#else
  0,
#endif
};

static void ogl_debug_callback(GLenum source, GLenum type, GLuint id,
                               GLenum severity, GLsizei length, GLchar *msg, const void *user);

#if __sysv
// glXCreateContextAttribsARB is an extension
//   so it has to be defined manually
using glXCreateContextAttribsARBFn = GLXContext (*)(
    Display *, GLXFBConfig, GLXContext, Bool, const int *
  );


static bool g_createcontextattribs_queried = false;
static glXCreateContextAttribsARBFn glXCreateContextAttribsARB = nullptr;

// ...Same with glXSwapIntervalEXT
using glXSwapIntervalEXTFn = void (*)(Display *dpy, GLXDrawable drawable, int interval);

static bool g_swapinterval_queried = false;
static glXSwapIntervalEXTFn glXSwapIntervalEXT = nullptr;

static bool g_gl3w_init = false;
#endif

struct GLContextData {
#if __sysv
  Display *display = nullptr;

  GLXContext context = nullptr;
  GLXWindow window = 0;

  ~GLContextData();

  bool createContext(const GLXFBConfig& fb_config, Window *window, gx::GLContext *share);
  bool createContextLegacy(const GLXFBConfig& fb_config, Window *window, gx::GLContext *share);
#endif
};

GLContextData::~GLContextData()
{
#if __sysv
  if(window) glXDestroyWindow(display, window);
  if(context) glXDestroyContext(display, context);
#endif
}

bool GLContextData::createContext(
    const GLXFBConfig& fb_config, Window *window, gx::GLContext *share)
{
#if __sysv
  if(!g_createcontextattribs_queried) {
    glXCreateContextAttribsARB = (glXCreateContextAttribsARBFn)
      glXGetProcAddressARB((const GLubyte *)"glXCreateContextAttribsARB");
  }

  if(!g_swapinterval_queried) {
    glXSwapIntervalEXT = (glXSwapIntervalEXTFn)
      glXGetProcAddressARB((const GLubyte *)"glXSwapIntervalEXT");
  }

  // Only old-style contexts are available
  if(!glXCreateContextAttribsARB) return false;

#if !defined(NDEBUG)
  int context_flags = GLX_CONTEXT_DEBUG_BIT_ARB | GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB;
#else
  int context_flags = GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB;
#endif

  const int context_attribs[] = {
    GLX_CONTEXT_MAJOR_VERSION_ARB, 3,
    GLX_CONTEXT_MINOR_VERSION_ARB, 3,
    GLX_CONTEXT_PROFILE_MASK_ARB, GLX_CONTEXT_CORE_PROFILE_BIT_ARB,

    GLX_CONTEXT_FLAGS_ARB, context_flags,

    None,
  };

  context = glXCreateContextAttribsARB(
      display, fb_config,
      /* shareList */ share ? (GLXContext)share->nativeHandle() : nullptr,
      /* direct */ True,
      context_attribs
  );
  if(!context) return false;

  return true;
#else
  return false;
#endif
}

bool GLContextData::createContextLegacy(
    const GLXFBConfig& fb_config, Window *window, gx::GLContext *share)
{
#if __sysv
  context = glXCreateNewContext(
      display, fb_config, GLX_RGBA_TYPE,
      /* shareList */ share ? (GLXContext)share->nativeHandle() : nullptr,
      /* direct */ True
  );

  return context;
#else
  return false;
#endif
}

GLContext::GLContext() :
  gx::GLContext(),
  p(nullptr), m_was_acquired(false)
{
}

GLContext::~GLContext()
{
  release();
}

void *GLContext::nativeHandle() const
{
#if __sysv
  return p ? p->context : nullptr;
#else
  return nullptr;
#endif
}

gx::GLContext& GLContext::acquire(os::Window *window_, gx::GLContext *share)
{
  auto window = (sysv::Window *)window_;

#if __sysv
  auto display = x11().xlibDisplay<Display>();

  int num_fb_configs = 0;
  auto fb_configs = glXChooseFBConfig(
      display, x11().defaultScreen(),
      GLX_VisualAttribs, &num_fb_configs
  );
  if(!fb_configs || !num_fb_configs) os::panic("No siutable GLXFBConfig found!", os::GLXError);

  // TODO: smarter way of choosing the FBConfig (?)
  auto fb_config = fb_configs[0];
  XVisualInfo *visual_info = glXGetVisualFromFBConfig(display, fb_config);

  auto cleanup_x_structures = [&]() -> void {
    XFree(visual_info);
    XFree(fb_configs);
  };

  auto depth  = visual_info->depth;
  auto visual = visual_info->visualid;

  // The visual of the window must match that of the
  //   FBConfig, so make sure that's the case before
  //   assigning the context to it
  if(!window->recreateWithVisual(depth, visual)) {
    cleanup_x_structures();

    os::panic("failed to create X11 window with GLX Visual", os::XCBError);
  }

  // The XID of the window
  auto x11_window_handle = (u32)(uintptr_t)window->nativeHandle();

  p = new GLContextData();

  p->display = display;

  // Try to create a new-style context first...
  if(!p->createContext(fb_config, window, share)) {
    // ...and fallback to old-style glXCreateNewContext
    //    if it fails
    p->createContextLegacy(fb_config, window, share);
  }

  // Check if context creation was successful
  if(!p->context) throw AcquireError();

  p->window = glXCreateWindow(
      display, fb_config, x11_window_handle, nullptr
  );
  if(!p->window) {
    glXDestroyContext(display, p->context);
    cleanup_x_structures();

    delete p;
    p = nullptr;

    throw AcquireError();
  }

  // It is the responsibility of the first-created context to intialize
  //   OpenGL extension function pointers (via gl3w)
  if(!g_gl3w_init) {
    auto drawable = (GLXDrawable)p->window;
    auto make_current_success = glXMakeContextCurrent(display, drawable, drawable, p->context);
    if(!make_current_success) throw AcquireError();

    auto gl3w_init_result = gl3wInit();
    if(gl3w_init_result != GL3W_OK) os::panic("Failed to initialize gl3w!", os::GL3WInitError);

    if(!gl3wIsSupported(3, 3)) os::panic("OpenGL version >= 3.3 required!", os::OpenGL3_3NotSupportedError);

    // XXX: does gl3w need to be initialied for every new context?
    //g_gl3w_init = true;
  }

#if !defined(NDEBUG)
  glEnable(GL_DEBUG_OUTPUT);
  glDebugMessageCallback((GLDEBUGPROC)ogl_debug_callback, nullptr);
#endif

  cleanup_x_structures();

  // Mark the context as successfully acquired
  m_was_acquired = true;
#endif

  return *this;
}

void GLContext::doMakeCurrent()
{
#if __sysv
  assert(m_was_acquired && "the context must've been acquire()'d to makeCurrent()!");

  auto display  = p->display;
  auto drawable = (GLXDrawable)p->window;
  auto context  = p->context;

  auto success = glXMakeContextCurrent(display, drawable, drawable, context);
  if(!success) throw AcquireError();

//  postMakeCurrentHook();
#endif
}

void GLContext::doRelease()
{
#if __sysv
  // This method will only be called when the
  //   GLContext's backing resources ACTUALLY
  //   need to be released
  delete p;

  p = nullptr;
  m_was_acquired = false;
#endif
}

bool GLContext::wasInit() const
{
  return m_was_acquired;
}

void GLContext::swapBuffers()
{
  assert(m_was_acquired && "the context must've been acquire()'d to swapBuffers()!");

#if __sysv
  auto drawable = (GLXDrawable)p->window;
  glXSwapBuffers(p->display, drawable);
#endif
}

void GLContext::swapInterval(unsigned interval)
{
  assert(m_was_acquired && "the context must've been acquire()'d to change the swapInterval()!");

  glXSwapIntervalEXT(p->display, p->window, interval);
}

static void ogl_debug_callback(GLenum source, GLenum type, GLuint id,
                               GLenum severity, GLsizei length, GLchar *msg, const void *user)
{
  const char *source_str = "";
  switch(source) {
  case GL_DEBUG_SOURCE_API:             source_str = "GL_API"; break;
  case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   source_str = "GL_WINDOW_SYSTEM"; break;
  case GL_DEBUG_SOURCE_SHADER_COMPILER: source_str = "GL_SHADER_COMPILER"; break;
  case GL_DEBUG_SOURCE_APPLICATION:     source_str = "GL_APPLICATION"; break;
  case GL_DEBUG_SOURCE_THIRD_PARTY:     source_str = "GL_THIRD_PARTY"; break;
  case GL_DEBUG_SOURCE_OTHER:           source_str = "GL_OTHER"; break;
  }

  const char *type_str = "";
  switch(type) {
  case GL_DEBUG_TYPE_ERROR:               type_str = "error!"; break;
  case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: type_str = "deprecated"; break;
  case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  type_str = "undefined!"; break;
  case GL_DEBUG_TYPE_PORTABILITY:         type_str = "portability"; break;
  case GL_DEBUG_TYPE_PERFORMANCE:         type_str = "performance"; break;
  case GL_DEBUG_TYPE_OTHER:               type_str = "other"; break;
  }

  const char *severity_str = "";
  switch(severity) {
  case GL_DEBUG_SEVERITY_HIGH:         severity_str = "!!!"; break;
  case GL_DEBUG_SEVERITY_MEDIUM:       severity_str = "!!"; break;
  case GL_DEBUG_SEVERITY_LOW:          severity_str = "!"; break;
  case GL_DEBUG_SEVERITY_NOTIFICATION: severity_str = "?"; break;
  }

  printf("%s (%s, %s): %s\n", source_str, severity_str, type_str, msg);
}

}
