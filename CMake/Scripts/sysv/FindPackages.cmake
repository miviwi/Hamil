message ("-- Searching for packages...")

set (THREADS_PREFER_PTHREAD_FLAG ON)
find_package (Threads REQUIRED)
message ("-- Found 'Threads' package")

find_package (X11 REQUIRED)
message ("-- Found 'X11' package")

set (X11_XCB_LIBRARIES xcb X11-xcb xcb-util ${X11_LIBRARIES})

find_package (Freetype REQUIRED)
message ("-- Found 'Freetype' package")

find_package (LibYAML REQUIRED)
message ("-- Found 'libyaml' package")

find_package (Bullet REQUIRED)
message ("-- Found 'Bullet' package")

find_package (PythonLibs REQUIRED)
message ("-- Found 'PythonLibs' package")

message ("-- Found all!")
