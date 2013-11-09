#  Find Half
#  Find Half headers and libraries.
#
#  HALF_INCLUDE_DIRS - where to find Half includes.
#  HALF_LIBRARIES    - List of libraries when using Half.
#  HALF_FOUND        - True if Half found.

# Look for the header file.
FIND_PATH( HALF_INCLUDE_DIR NAMES OpenEXR/half.h)

# Look for the libraries.
FIND_LIBRARY( HALF_LIBRARY NAMES Half)

# handle the QUIETLY and REQUIRED arguments and set HALF_FOUND to TRUE if
# all listed variables are TRUE
INCLUDE( FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS( HALF DEFAULT_MSG HALF_LIBRARY
                                                    HALF_INCLUDE_DIR
                                                    )
# Copy the results to the output variables.
IF( HALF_FOUND)
    SET( HALF_LIBRARIES ${HALF_LIBRARY})
    SET( HALF_INCLUDE_DIRS ${HALF_INCLUDE_DIR})
ELSE()
    SET( HALF_LIBRARIES)
    SET( HALF_INCLUDE_DIRS)
ENDIF()

MARK_AS_ADVANCED( HALF_LIBRARY HALF_INCLUDE_DIR)
