# Eigen library plugins:
# ===================================================
SET(EIGEN_MATRIXBASE_PLUGIN "<mrpt/math/eigen_plugins.h>" CACHE STRING "Eigen plugin header")
SET(EIGEN_MATRIXBASE_PLUGIN_POST_IMPL "<mrpt/math/eigen_plugins_impl.h>" CACHE STRING "Eigen plugin implementation header")
SET(EIGEN_EMBEDDED_INCLUDE_DIR "${MRPT_SOURCE_DIR}/otherlibs/eigen3/" CACHE PATH "Eigen path for embedded use")

MARK_AS_ADVANCED(EIGEN_MATRIXBASE_PLUGIN)
MARK_AS_ADVANCED(EIGEN_MATRIXBASE_PLUGIN_POST_IMPL)
MARK_AS_ADVANCED(EIGEN_EMBEDDED_INCLUDE_DIR)

# By default: Use system version if pkg-config says it exists:
SET(DEFAULT_EIGEN_USE_EMBEDDED_VERSION ON)
IF(PKG_CONFIG_FOUND)
	PKG_CHECK_MODULES(PKG_EIGEN3 ${_QUIET} eigen3)	# Find eigen3 with pkg-config:
	# We require Eigen 3.2 minimum:
	IF(PKG_EIGEN3_FOUND AND ${PKG_EIGEN3_VERSION} VERSION_GREATER "3.1.9")
		# Use system version:
		SET(DEFAULT_EIGEN_USE_EMBEDDED_VERSION OFF)
	ENDIF()
ENDIF(PKG_CONFIG_FOUND)

SET(EIGEN_USE_EMBEDDED_VERSION ${DEFAULT_EIGEN_USE_EMBEDDED_VERSION} CACHE BOOL "Use embedded Eigen3 version or system version")
IF (EIGEN_USE_EMBEDDED_VERSION)
	# Include embedded version headers:
	SET(MRPT_EIGEN_INCLUDE_DIR "${EIGEN_EMBEDDED_INCLUDE_DIR}")
ELSE(EIGEN_USE_EMBEDDED_VERSION)
	# Find Eigen headers in the system:
	IF(NOT PKG_CONFIG_FOUND)
		MESSAGE(SEND_ERROR "pkg-config is required for this operation!")
	ELSE(NOT PKG_CONFIG_FOUND)
		# Find eigen3 with pkg-config:
		PKG_CHECK_MODULES(PKG_EIGEN3 ${_QUIET} eigen3)
		IF(PKG_EIGEN3_FOUND)
			SET(MRPT_EIGEN_INCLUDE_DIR "${PKG_EIGEN3_INCLUDE_DIRS}")
		ELSE(PKG_EIGEN3_FOUND)
			MESSAGE(SEND_ERROR "pkg-config was unable to find eigen3: install libeigen3-dev or enable EIGEN_USE_EMBEDDED_VERSION")
		ENDIF(PKG_EIGEN3_FOUND)
	ENDIF(NOT PKG_CONFIG_FOUND)
ENDIF(EIGEN_USE_EMBEDDED_VERSION)

INCLUDE_DIRECTORIES("${MRPT_EIGEN_INCLUDE_DIR}")
IF(EXISTS "${MRPT_EIGEN_INCLUDE_DIR}/unsupported/")
	INCLUDE_DIRECTORIES("${MRPT_EIGEN_INCLUDE_DIR}/unsupported/")
ENDIF(EXISTS "${MRPT_EIGEN_INCLUDE_DIR}/unsupported/")

# Create variables just for the final summary of the configuration (see bottom of this file):
SET(CMAKE_MRPT_HAS_EIGEN 1)        # Always, it's a fundamental dep.!

# Create numeric (0/1) variable EIGEN_USE_EMBEDDED_VERSION_BOOL for the .cmake.in file:
IF(EIGEN_USE_EMBEDDED_VERSION)
	SET(EIGEN_USE_EMBEDDED_VERSION_BOOL 1)
	SET(CMAKE_MRPT_HAS_EIGEN_SYSTEM 0)
ELSE(EIGEN_USE_EMBEDDED_VERSION)
	SET(EIGEN_USE_EMBEDDED_VERSION_BOOL 0)
	SET(CMAKE_MRPT_HAS_EIGEN_SYSTEM 1)
ENDIF(EIGEN_USE_EMBEDDED_VERSION)


# Add directories as "-isystem" to avoid warnings with :
SET(AUX_EIGEN_INCL_DIR ${MRPT_EIGEN_INCLUDE_DIR})
ADD_DIRECTORIES_AS_ISYSTEM(AUX_EIGEN_INCL_DIR)

# Detect Eigen version (just to show it in the CMake config summary)
SET(EIGEN_VER_H "${MRPT_EIGEN_INCLUDE_DIR}/Eigen/src/Core/util/Macros.h")
IF (EXISTS ${EIGEN_VER_H})
	file(READ "${EIGEN_VER_H}" STR_EIGEN_VERSION)

	# Extract the Eigen version from the Macros.h file, lines "#define EIGEN_WORLD_VERSION  XX", etc...

	STRING(REGEX MATCH "EIGEN_WORLD_VERSION[ ]+[0-9]+" CMAKE_EIGEN_VERSION_NUMBER_MAJOR "${STR_EIGEN_VERSION}")
	STRING(REGEX MATCH "[0-9]+" CMAKE_EIGEN_VERSION_NUMBER_MAJOR "${CMAKE_EIGEN_VERSION_NUMBER_MAJOR}")

	STRING(REGEX MATCH "EIGEN_MAJOR_VERSION[ ]+[0-9]+" CMAKE_EIGEN_VERSION_NUMBER_MINOR "${STR_EIGEN_VERSION}")
	STRING(REGEX MATCH "[0-9]+" CMAKE_EIGEN_VERSION_NUMBER_MINOR "${CMAKE_EIGEN_VERSION_NUMBER_MINOR}")

	STRING(REGEX MATCH "EIGEN_MINOR_VERSION[ ]+[0-9]+" CMAKE_EIGEN_VERSION_NUMBER_PATCH "${STR_EIGEN_VERSION}")
	STRING(REGEX MATCH "[0-9]+" CMAKE_EIGEN_VERSION_NUMBER_PATCH "${CMAKE_EIGEN_VERSION_NUMBER_PATCH}")

	SET(MRPT_EIGEN_VERSION "${CMAKE_EIGEN_VERSION_NUMBER_MAJOR}.${CMAKE_EIGEN_VERSION_NUMBER_MINOR}.${CMAKE_EIGEN_VERSION_NUMBER_PATCH}")

	IF($ENV{VERBOSE})
		MESSAGE(STATUS "Eigen version detected: ${MRPT_EIGEN_VERSION}")
	ENDIF($ENV{VERBOSE})
ENDIF (EXISTS ${EIGEN_VER_H})
