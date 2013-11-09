find_path(V8_INCLUDE_DIR v8.h NO_DEFAULT_PATH PATHS ${V8_ROOT}/include)

find_library(V8_LIBRARY_BASE NAMES v8_base v8_base.x64 v8_base.ia32 NO_DEFAULT_PATH PATHS ${V8_ROOT}/out/native/obj.target/tools/gyp)
find_library(V8_LIBRARY_SNAPSHOT NAMES v8_snapshot v8_snapshot.x64 v8_snapshot.ia32 NO_DEFAULT_PATH PATHS ${V8_ROOT}/out/native/obj.target/tools/gyp)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(V8 DEFAULT_MSG V8_LIBRARY_BASE V8_INCLUDE_DIR)

if(V8_FOUND)
  set(V8_LIBRARIES ${V8_LIBRARY_BASE} ${V8_LIBRARY_SNAPSHOT})
else(V8_FOUND)
  set(V8_LIBRARIES)
endif(V8_FOUND)

mark_as_advanced(V8_LIBRARY_BASE V8_LIBRARY_SNAPSHOT V8_INCLUDE_DIR)
