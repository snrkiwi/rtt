########################################################################################################################
#
# CMake package use file for OROCOS-RTT.
# It is assumed that find_package(OROCOS-RTT ...) has already been invoked.
# See orocos-rtt-config.cmake for information on how to load OROCOS-RTT into your CMake project.
# To include this file from your CMake project, the OROCOS-RTT_USE_FILE_PATH variable is used:
#   include(${OROCOS-RTT_USE_FILE_PATH}/UseOROCOS-RTT.cmake) 
# or even shorter:
#   include(${OROCOS-RTT_USE_FILE})
#
########################################################################################################################

cmake_minimum_required(VERSION 2.8.3)

if(OROCOS-RTT_FOUND AND NOT USE_OROCOS_RTT)
  include(FindPkgConfig)
  include(${OROCOS-RTT_USE_FILE_PATH}/UseOROCOS-RTT-helpers.cmake)

  # CMake 2.8.8 added support for per-target INCLUDE_DIRECTORIES. The include directories will only be added to targets created
  # with the orocos_*() macros. For older versions we have to set INCLUDE_DIRECTORIES per-directory.
  # See https://github.com/orocos-toolchain/rtt/pull/85 for details.
  if(CMAKE_VERSION VERSION_LESS 2.8.8)
    include_directories(${OROCOS-RTT_INCLUDE_DIRS})
  endif()

  # Preprocessor definitions
  add_definitions(${OROCOS-RTT_DEFINITIONS})

  # Check for client meta-buildsystem tools
  # 
  # Tool support for:
  #   - catkin
  #   - rosbuild
  #
  # If the client is using rosbuild, and has called rosbuild_init(), then we
  # will assume that he or she wants to build targets with rosbuild libraries.
  # 
  # If the client has not called rosbuild_init() then we check if
  # `find_package(catkin ...)` has been called (explicitly by the user
  # or implicitly by building using `catkin_make`) or in the case of
  # `catkin_make_isolated` if CATKIN_DEVEL_PREFIX is set and if there
  # is a `package.xml` file in the.  project's source folder. If yes,
  # and catkin has been found, then we can assume this is a catkin
  # build.
  #
  # rosbuild- or catkin build-style build can be enforced or forbidden by setting
  # the ORO_USE_ROSBUILD or ORO_USE_CATKIN cmake variable explicitly.
  #
  # Note that within one build folder all packages have to use the same buildsystem.
  #
  if(ORO_USE_ROSBUILD OR (NOT DEFINED ORO_USE_ROSBUILD AND COMMAND rosbuild_init AND ROSBUILD_init_called))
    message(STATUS "[UseOrocos] Building package ${PROJECT_NAME} with rosbuild in-source support.")
    set(ORO_USE_ROSBUILD True CACHE BOOL "Build packages with rosbuild in-source support.")

    if ( NOT ROSBUILD_init_called )
      if ( NOT COMMAND rosbuild_init )
        include($ENV{ROS_ROOT}/core/rosbuild/rosbuild.cmake) # Prevent double inclusion ! This file is not robust against that !
      endif()
      rosbuild_init()
    endif()
  elseif(ORO_USE_CATKIN OR (NOT DEFINED ORO_USE_CATKIN AND (catkin_FOUND OR DEFINED CATKIN_DEVEL_PREFIX) AND EXISTS "${PROJECT_SOURCE_DIR}/package.xml"))
    if( NOT catkin_FOUND)
      find_package(catkin REQUIRED)
    endif()
    if (NOT catkin_FOUND)
      message(FATAL_ERROR "We are building with catkin support but catkin could not be found.")
    endif()
    message(STATUS "[UseOrocos] Building package ${PROJECT_NAME} with catkin develspace support.")
    set(ORO_USE_CATKIN True CACHE BOOL "Build packages with catkin develspace support.")
  else()
    message(STATUS "[UseOrocos] Building package ${PROJECT_NAME} without an external buildtool like rosbuild or catkin")
  endif()

  # This is for not allowing undefined symbols when using gcc
  if (CMAKE_COMPILER_IS_GNUCXX AND NOT APPLE)
    SET(USE_OROCOS_LDFLAGS_OTHER "-Wl,-z,defs")
  else (CMAKE_COMPILER_IS_GNUCXX AND NOT APPLE)
    SET(USE_OROCOS_LDFLAGS_OTHER " ")
  endif (CMAKE_COMPILER_IS_GNUCXX AND NOT APPLE)
  # Suppress API decoration warnings in Win32:
  if (MSVC)
    set(USE_OROCOS_CFLAGS_OTHER "/wd4251" )
  else (MSVC)
    set(USE_OROCOS_CFLAGS_OTHER " " )
  endif (MSVC)

  # On windows, the CMAKE_INSTALL_PREFIX is forced to the Orocos-RTT path.
  # There's two alternatives to disable this behavior:
  #
  # 1. Use the ORO_DEFAULT_INSTALL_PREFIX variable to modify the default
  #    installation path:
  #
  #     set(ORO_DEFAULT_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX})
  #     include(${OROCOS-RTT_USE_FILE_PATH}/UseOROCOS-RTT.cmake)
  #
  # 2. Force a non-default CMAKE_INSTALL_PREFIX prior to executing cmake:
  #
  #     cmake -DCMAKE_INSTALL_PREFIX="<your install prefix>" [...]
  #
  # In all cases, the Orocos macros will always honor any change to the cached
  # CMAKE_INSTALL_PREFIX variable.
  if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT AND NOT DEFINED ORO_DEFAULT_INSTALL_PREFIX)
    if(WIN32)
      set(ORO_DEFAULT_INSTALL_PREFIX "orocos")
    endif(WIN32)
  endif(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT AND NOT DEFINED ORO_DEFAULT_INSTALL_PREFIX)

  # For backwards compatibility. Was only used on WIN32 targets:
  if(DEFINED INSTALL_PATH)
    set(ORO_DEFAULT_INSTALL_PREFIX ${INSTALL_PATH})
  endif(DEFINED INSTALL_PATH)

  if(DEFINED ORO_DEFAULT_INSTALL_PREFIX)
    if(ORO_DEFAULT_INSTALL_PREFIX STREQUAL "orocos")
      set (CMAKE_INSTALL_PREFIX ${OROCOS-RTT_PATH} CACHE PATH "Install prefix forced to orocos by ORO_DEFAULT_INSTALL_PREFIX" FORCE)
    else(ORO_DEFAULT_INSTALL_PREFIX STREQUAL "orocos")
      set (CMAKE_INSTALL_PREFIX ${ORO_DEFAULT_INSTALL_PREFIX} CACHE PATH "Install prefix forced by ORO_DEFAULT_INSTALL_PREFIX" FORCE)
    endif(ORO_DEFAULT_INSTALL_PREFIX STREQUAL "orocos")
  endif(DEFINED ORO_DEFAULT_INSTALL_PREFIX)

  message(STATUS "[UseOrocos] Using Orocos RTT in ${PROJECT_NAME}")

  # Set to true to indicate that these macros are available.
  set(USE_OROCOS_RTT 1)

  # By default, install libs in /target/ subdir in order to allow
  # multi-target installs.
  if ( NOT DEFINED OROCOS_SUFFIX )
    set (OROCOS_SUFFIX "/${OROCOS_TARGET}")
  endif()

  # Enable auto-linking and installation
  set(OROCOS_NO_AUTO_LINKING OFF CACHE BOOL "Disable automatic linking to targets in orocos_use_package() or from dependencies in the package manifest. Auto-linking is enabled by default.")
  set(OROCOS_NO_AUTO_INSTALL OFF CACHE BOOL "Disable automatic installation of Orocos targets. Auto-installation is enabled by default.")

  # Set build system specific variables
  if (ORO_USE_ROSBUILD)
    # Infer package name from directory name.
    get_filename_component(ORO_ROSBUILD_PACKAGE_NAME ${PROJECT_SOURCE_DIR} NAME)

    # Modify default rosbuild output paths if using Eclipse
    if (CMAKE_EXTRA_GENERATOR STREQUAL "Eclipse CDT4")
      message(WARNING "[UseOrocos] Eclipse Generator detected. I'm setting EXECUTABLE_OUTPUT_PATH and LIBRARY_OUTPUT_PATH")
      message(WARNING "[UseOrocos] This will not affect the real output paths of libraries and executables!")
      #set the default path for built executables to the "bin" directory
      set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
      #set the default path for built libraries to the "lib" directory
      set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
    endif()

    # Set output directories for rosbuild in-source builds,
    # but respect deprecated LIBRARY_OUTPUT_PATH, EXECUTABLE_OUTPUT_PATH and ARCHIVE_OUTPUT_PATH variables
    # as they are set by rosbuild_init() and commonly used in rosbuild CMakeLists.txt files
    if(NOT CMAKE_LIBRARY_OUTPUT_DIRECTORY)
      if(DEFINED LIBRARY_OUTPUT_PATH)
        set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_PATH})
      else()
        set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)
      endif()
    endif()
    if(NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
      if(DEFINED EXECUTABLE_OUTPUT_PATH)
        set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${EXECUTABLE_OUTPUT_PATH})
      else()
        set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)
      endif()
    endif()
    if(NOT CMAKE_ARCHIVE_OUTPUT_DIRECTORY)
      if(DEFINED ARCHIVE_OUTPUT_PATH)
        set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${ARCHIVE_OUTPUT_PATH})
      else()
        set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)
      endif()
    endif()
    set(ORO_COMPONENT_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/orocos${OROCOS_SUFFIX}/${PROJECT_NAME})
    set(ORO_TYPEKIT_OUTPUT_DIRECTORY   ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/orocos${OROCOS_SUFFIX}/${PROJECT_NAME}/types)
    set(ORO_PLUGIN_OUTPUT_DIRECTORY    ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/orocos${OROCOS_SUFFIX}/${PROJECT_NAME}/plugins)

    # We only need the direct dependencies, the rest is resolved by the .pc
    # files.
    rosbuild_invoke_rospack(${ORO_ROSBUILD_PACKAGE_NAME} pkg DEPS depends1)
    string(REGEX REPLACE "\n" ";" pkg_DEPS2 "${pkg_DEPS}" )
    foreach(ROSDEP ${pkg_DEPS2})
      orocos_use_package( ${ROSDEP} OROCOS_ONLY)
    endforeach(ROSDEP ${pkg_DEPS2})

  elseif(ORO_USE_CATKIN)
     # Parse package.xml file in ${PROJECT_SOURCE_DIR}/package.xml to set ${PROJECT_NAME}_VERSION and ${PROJECT_NAME}_BUILD_DEPENDS
    if(NOT _CATKIN_CURRENT_PACKAGE)
      catkin_package_xml(DIRECTORY ${PROJECT_SOURCE_DIR})
    endif()

    # Set output directories for catkin
    catkin_destinations()
    set(ORO_COMPONENT_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/orocos${OROCOS_SUFFIX}/${PROJECT_NAME})
    set(ORO_TYPEKIT_OUTPUT_DIRECTORY   ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/orocos${OROCOS_SUFFIX}/${PROJECT_NAME}/types)
    set(ORO_PLUGIN_OUTPUT_DIRECTORY    ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/orocos${OROCOS_SUFFIX}/${PROJECT_NAME}/plugins)

    # Get catkin build_depend dependencies
    foreach(DEP ${${PROJECT_NAME}_BUILD_DEPENDS})
      # We use OROCOS_ONLY so that we only find .pc files with the orocos target on them
      orocos_use_package( ${DEP} OROCOS_ONLY) 
    endforeach(DEP ${DEPS}) 

  else()
    # Set output directories relative to CMAKE_LIBRARY_OUTPUT_DIRECTORY or built in the current binary directory (cmake default).
    if(CMAKE_LIBRARY_OUTPUT_DIRECTORY)
      set(ORO_COMPONENT_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/orocos${OROCOS_SUFFIX}/${PROJECT_NAME})
      set(ORO_TYPEKIT_OUTPUT_DIRECTORY   ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/orocos${OROCOS_SUFFIX}/${PROJECT_NAME}/types)
      set(ORO_PLUGIN_OUTPUT_DIRECTORY    ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/orocos${OROCOS_SUFFIX}/${PROJECT_NAME}/plugins)
    else()
      set(ORO_COMPONENT_OUTPUT_DIRECTORY orocos${OROCOS_SUFFIX}/${PROJECT_NAME})
      set(ORO_TYPEKIT_OUTPUT_DIRECTORY   orocos${OROCOS_SUFFIX}/${PROJECT_NAME}/types)
      set(ORO_PLUGIN_OUTPUT_DIRECTORY    orocos${OROCOS_SUFFIX}/${PROJECT_NAME}/plugins)
    endif()

    # Fall back to manually processing the Autoproj manifest.xml file.
    orocos_get_manifest_deps( DEPS )
    #message("orocos_get_manifest_deps are: ${DEPS}")
    foreach(DEP ${DEPS})
      orocos_use_package( ${DEP} OROCOS_ONLY) 
    endforeach(DEP ${DEPS}) 
  endif()

  # Output the library and runtime destinations
  if("$ENV{VERBOSE}")
    message(STATUS "[UseOrocos] Building library targets in ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")
    message(STATUS "[UseOrocos] Building runtime targets in ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
  endif()

  # Set default install destinations:
  set(ORO_LIBRARY_DESTINATION lib )
  if(NOT ORO_USE_CATKIN)
    set(ORO_RUNTIME_DESTINATION bin )
  else()
    set(ORO_RUNTIME_DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION} )
  endif()
  set(ORO_INCLUDE_DESTINATION include/orocos/${PROJECT_NAME} )
  set(ORO_COMPONENT_DESTINATION lib/orocos${OROCOS_SUFFIX}/${PROJECT_NAME} )
  set(ORO_EXECUTABLE_DESTINATION ${ORO_RUNTIME_DESTINATION} )
  set(ORO_TYPEKIT_DESTINATION lib/orocos${OROCOS_SUFFIX}/${PROJECT_NAME}/types )
  set(ORO_PLUGIN_DESTINATION lib/orocos${OROCOS_SUFFIX}/${PROJECT_NAME}/plugins )

  # Necessary for correctly building mixed libraries on win32.
  if(OROCOS_TARGET STREQUAL "win32")
    set(CMAKE_DEBUG_POSTFIX "d")
  endif(OROCOS_TARGET STREQUAL "win32")

  # Internal macro to configure a target for Orocos RTT.
  #
  # Usage: _orocos_target( target (COMPONENT|LIBRARY|EXECTUABLE|TYPEKIT|PLUGIN) [INSTALL custom/install/destination] [SKIP_INSTALL] [VERSION x.y.z] )
  #
  macro( _orocos_target target target_type )
    cmake_parse_arguments(_orocos_target
      "SKIP_INSTALL"
      "INSTALL;VERSION"
      ""
      ${ARGN}
      )

    # Set library output name:
    if ( ${OROCOS_TARGET} STREQUAL "gnulinux" OR ${OROCOS_TARGET} STREQUAL "lxrt" OR ${OROCOS_TARGET} STREQUAL "xenomai" OR ${OROCOS_TARGET} STREQUAL "win32" OR ${OROCOS_TARGET} STREQUAL "macosx")
      if( NOT target MATCHES ".*-${OROCOS_TARGET}$")
        set(_orocos_target_OUTPUT_NAME OUTPUT_NAME ${target}-${OROCOS_TARGET})
      else()
        set(_orocos_target_OUTPUT_NAME OUTPUT_NAME ${target})
      endif()
    else()
      set(_orocos_target_OUTPUT_NAME OUTPUT_NAME ${target})
    endif()

    # Set output and installation directory and depending on target type:
    if(ORO_${target_type}_OUTPUT_DIRECTORY)
      set(_orocos_target_OUTPUT_DIRECTORY
        LIBRARY_OUTPUT_DIRECTORY ${ORO_${target_type}_OUTPUT_DIRECTORY}
        ARCHIVE_OUTPUT_DIRECTORY ${ORO_${target_type}_OUTPUT_DIRECTORY}
      )
    else()
      set(_orocos_target_OUTPUT_DIRECTORY)
    endif()
    set(_orocos_target_LIBRARY_DESTINATION ${ORO_${target_type}_DESTINATION})
    set(_orocos_target_RUNTIME_DESTINATION ${ORO_RUNTIME_DESTINATION})
    if(_orocos_target_INSTALL)
      set(_orocos_target_LIBRARY_DESTINATION ${_orocos_target_INSTALL})
      set(_orocos_target_RUNTIME_DESTINATION ${_orocos_target_INSTALL})
    endif()

    # Set library version:
    if (_orocos_target_VERSION)
      set(_orocos_target_VERSION VERSION ${_orocos_target_VERSION})
    elseif(COMPONENT_VERSION)
      set(_orocos_target_VERSION VERSION ${COMPONENT_VERSION})
    else()
      set(_orocos_target_VERSION)
    endif()

    # Prepare lib for out-of-the-ordinary lib directories:
    set_target_properties( ${target} PROPERTIES
      ${_orocos_target_OUTPUT_NAME}
      ${_orocos_target_OUTPUT_DIRECTORY}
      ${_orocos_target_VERSION}
    )

    orocos_add_include_directories( ${target} ${OROCOS-RTT_INCLUDE_DIRS} ${USE_OROCOS_INCLUDE_DIRECTORIES})
    orocos_add_compile_flags( ${target} ${USE_OROCOS_CFLAGS_OTHER})
    orocos_add_link_flags( ${target} ${USE_OROCOS_LDFLAGS_OTHER})
    orocos_set_install_rpath( ${target} ${USE_OROCOS_LIBRARY_DIRS})

    target_link_libraries( ${target}
      ${OROCOS-RTT_LIBRARIES}
      #${OROCOS-RTT_TYPEKIT_LIBRARIES}
      )

    # Only link in case there is something *and* the user didn't opt-out:
    if(NOT OROCOS_NO_AUTO_LINKING AND USE_OROCOS_LIBRARIES)
      target_link_libraries( ${target} ${USE_OROCOS_LIBRARIES} )
      if("$ENV{VERBOSE}" OR ORO_USE_VERBOSE)
        message(STATUS "[UseOrocos] Linking target '${target}' with libraries from packages '${USE_OROCOS_PACKAGES}'. To disable this, set OROCOS_NO_AUTO_LINKING to true.")
      endif()
    endif()

    # Install:
    if((NOT OROCOS_NO_AUTO_INSTALL AND NOT _orocos_target_SKIP_INSTALL) OR _orocos_target_INSTALL)
      install(TARGETS ${target}
        LIBRARY DESTINATION ${_orocos_target_LIBRARY_DESTINATION}
        ARCHIVE DESTINATION ${_orocos_target_LIBRARY_DESTINATION}
        RUNTIME DESTINATION ${_orocos_target_RUNTIME_DESTINATION}
      )
      if("$ENV{VERBOSE}" OR ORO_USE_VERBOSE)
        if(WIN32 OR target_type STREQUAL "EXECUTABLE")
          message(STATUS "[UseOrocos] Installing target '${target}' to destination ${_orocos_target_RUNTIME_DESTINATION}. To disable this, set OROCOS_NO_AUTO_INSTALL to true.")
        else()
          message(STATUS "[UseOrocos] Installing target '${target}' to destination ${_orocos_target_LIBRARY_DESTINATION}. To disable this, set OROCOS_NO_AUTO_INSTALL to true.")
        endif()
      endif()
    endif()

    # Export target for orocos_find_package() calls in the same workspace:
    list(APPEND ${PROJECT_NAME}_EXPORTED_TARGETS "${target}")
    list(APPEND ${PROJECT_NAME}_EXPORTED_LIBRARY_DIRS "${CMAKE_INSTALL_PREFIX}/${_orocos_target_LIBRARY_DESTINATION}")

    # Unset temporary variables
    unset(_orocos_target_INSTALL)
    unset(_orocos_target_SKIP_INSTALL)
    unset(_orocos_target_OUTPUT_NAME)
    unset(_orocos_target_OUTPUT_DIRECTORY)
    unset(_orocos_target_LIBRARY_DESTINATION)
    unset(_orocos_target_RUNTIME_DESTINATION)
    unset(_orocos_target_VERSION)

  endmacro()

  # Components should add themselves by calling 'OROCOS_COMPONENT' 
  # instead of 'add_library' in CMakeLists.txt.
  # You can set a variable COMPONENT_VERSION x.y.z to set a version or 
  # specify the optional VERSION parameter. For ros builds, the version
  # number is ignored.
  #
  # Usage:
  #   orocos_component( componentname src1 [src2 ...] [INSTALL custom/install/destination] [SKIP_INSTALL] [VERSION x.y.z] )
  #      or
  #   add_library( target SHARED src1 [src2 ...] )
  #   orocos_component( target [INSTALL custom/install/destination] [SKIP_INSTALL] [VERSION x.y.z] )
  #
  macro( orocos_component target )
    cmake_parse_arguments(_orocos_component
      "SKIP_INSTALL"
      "INSTALL;VERSION"
      ""
      ${ARGN}
      )

    set(_orocos_component_SOURCES ${_orocos_component_UNPARSED_ARGUMENTS} )

    # Clear the dependencies such that a target switch can be detected:
    unset( ${target}_LIB_DEPENDS )

    # Build the target:
    if(_orocos_component_SOURCES)
      # Use rosbuild in ros environments:
      if (ORO_USE_ROSBUILD)
        message( STATUS "[UseOrocos] Building component ${target} in library ${target} in rosbuild source tree." )
        rosbuild_add_library(${target} ${_orocos_component_SOURCES} )
      else()
        message( STATUS "[UseOrocos] Building component ${target} in library ${target}" )
        add_library( ${target} SHARED ${_orocos_component_SOURCES} )
      endif()
    endif()

    # Check if the target exists:
    if(NOT TARGET ${target})
      message(FATAL_ERROR "[UseOrocos] Target '${target}' does not exist in orocos_component()." )
    endif()

    # Configure the target as a component library:
    _orocos_target(${target} COMPONENT ${ARGN})
    set_target_properties(${target} PROPERTIES DEFINE_SYMBOL "RTT_COMPONENT")

    # Necessary for .pc file generation
    get_target_property(_orocos_component_OUTPUT_NAME ${target} OUTPUT_NAME)
    list(APPEND OROCOS_DEFINED_COMPS " -l${_orocos_component_OUTPUT_NAME}")

    # Unset temporary variables
    unset(_orocos_component_SOURCES)
    unset(_orocos_component_INSTALL)
    unset(_orocos_component_SKIP_INSTALL)
    unset(_orocos_component_VERSION)
    unset(_orocos_component_OUTPUT_NAME)

  endmacro( orocos_component )

  # Utility libraries should add themselves by calling 'orocos_library()'
  # instead of 'add_library' in CMakeLists.txt.
  # You can set a variable COMPONENT_VERSION x.y.z to set a version or
  # specify the optional VERSION parameter. For ros builds, the version
  # number is ignored.
  #
  # Usage:
  #   orocos_library( libraryname src1 [src2 ...] [INSTALL custom/install/destination] [SKIP_INSTALL] [VERSION x.y.z] )
  #      or
  #   add_library( target SHARED src1 [src2 ...] )
  #   orocos_library( target [INSTALL custom/install/destination] [SKIP_INSTALL] [VERSION x.y.z] )
  #
  macro( orocos_library target )
    cmake_parse_arguments(_orocos_library
      "SKIP_INSTALL"
      "INSTALL;VERSION"
      ""
      ${ARGN}
      )

    set(_orocos_library_SOURCES ${_orocos_library_UNPARSED_ARGUMENTS} )

    # Clear the dependencies such that a target switch can be detected:
    unset( ${target}_LIB_DEPENDS )

    # Build the target:
    if(_orocos_library_SOURCES)
      # Use rosbuild in ros environments:
      if (ORO_USE_ROSBUILD)
        message( STATUS "[UseOrocos] Building library ${target} in rosbuild source tree." )
        rosbuild_add_library(${target} ${_orocos_library_SOURCES} )
      else()
        message( STATUS "[UseOrocos] Building library ${target}" )
        add_library( ${target} SHARED ${_orocos_library_SOURCES} )
      endif()
    endif()

    # Check if the target exists:
    if(NOT TARGET ${target})
      message(FATAL_ERROR "[UseOrocos] Target '${target}' does not exist in orocos_library().")
    endif()

    # Configure the target as a library:
    _orocos_target(${target} LIBRARY ${ARGN})

    # Necessary for .pc file generation
    get_target_property(_orocos_library_OUTPUT_NAME ${target} OUTPUT_NAME)
    list(APPEND OROCOS_DEFINED_LIBS " -l${_orocos_library_OUTPUT_NAME}")

    # Unset temporary variables
    unset(_orocos_library_SOURCES)
    unset(_orocos_library_INSTALL)
    unset(_orocos_library_SKIP_INSTALL)
    unset(_orocos_library_VERSION)
    unset(_orocos_library_OUTPUT_NAME)

  endmacro( orocos_library )

  # Executables should add themselves by calling 'orocos_executable()'
  # instead of 'ADD_EXECUTABLE' in CMakeLists.txt.
  #
  # Usage:
  #   orocos_executable( executablename src1 [src2 ...] [INSTALL custom/install/destination] [SKIP_INSTALL] )
  #      or
  #   add_executable( target src1 [src2 ...] )
  #   orocos_executable( target [INSTALL custom/install/destination] [SKIP_INSTALL] )
  #
  macro( orocos_executable target )
    cmake_parse_arguments(_orocos_executable
      "SKIP_INSTALL"
      "INSTALL"
      ""
      ${ARGN}
      )

    set(_orocos_executable_SOURCES ${_orocos_executable_UNPARSED_ARGUMENTS} )

    # Clear the dependencies such that a target switch can be detected:
    unset( ${target}_LIB_DEPENDS )

    # Build the target:
    if(_orocos_executable_SOURCES)
      # Use rosbuild in ros environments:
      if (ORO_USE_ROSBUILD)
        message( STATUS "[UseOrocos] Building executable ${target} in rosbuild source tree." )
        rosbuild_add_executable(${target} ${_orocos_executable_SOURCES} )
      else()
        message( STATUS "[UseOrocos] Building executable ${target}" )
        add_executable( ${target} ${_orocos_executable_SOURCES} )
      endif()
    endif()

    # Check if the target exists:
    if(NOT TARGET ${target})
      message(FATAL_ERROR "[UseOrocos] Target '${target}' does not exist in orocos_executable().")
    endif()

    # Configure the target as an executable:
    _orocos_target(${target} EXECUTABLE ${ARGN})

    # Note: CMAKE_DEBUG_POSTFIX is only automatically applied to non-executable targets.
    if(CMAKE_DEBUG_POSTFIX)
      set_target_properties( ${target} PROPERTIES DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX} )
    endif(CMAKE_DEBUG_POSTFIX)

    # Unset temporary variables
    unset(_orocos_executable_SOURCES)
    unset(_orocos_executable_INSTALL)
    unset(_orocos_executable_SKIP_INSTALL)
    unset(_orocos_executable_VERSION)

  endmacro( orocos_executable )

  # Type headers should add themselves by calling 'orocos_typegen_headers()'
  # They will be processed by typegen to generate a typekit from it, with the
  # name of the current project. You may also pass additional options to typegen
  # before listing your header files. 
  # 
  # Use 'DEPENDS <packagename> ...' to add dependencies on other (typegen) packages.
  # This macro passes the -x OROCOS_TARGET flag to typegen automatically, so there
  # is no need to include the -OROCOS_TARGET suffix in the <packagename>
  #
  # NOTE: if you use a subdir for your headers, e.g. include/robotdata.hpp, it
  # will install this header into pkgname/include/robotdata.hpp ! Most likely
  # not what you want. So call this macro from the include dir itself.
  #
  # Usage: orocos_typegen_headers( robotdata.hpp sensordata.hpp DEPENDS orocos_kdl [VERSION x.y.z] [INSTALL custom/install/destination] [SKIP_INSTALL] )
  #
  macro( orocos_typegen_headers )
    cmake_parse_arguments(_orocos_typegen_headers
      "SKIP_INSTALL"
      "DEPENDS;INSTALL;VERSION"
      ""
      ${ARGN}
      )

    set(_orocos_typegen_headers_HEADERS ${_orocos_typegen_headers_UNPARSED_ARGUMENTS} )

    # Save ARGN for orocos_typekit() called from the typekit subdirectory:
    set(_orocos_typegen_headers_ARGN ${ARGN})

    if(_orocos_typegen_headers_DEPENDS)
      set(_orocos_typegen_headers_DEP_INFO_MSG "using: ${_orocos_typegen_headers_DEPENDS}")
    endif()
    message(STATUS "[UseOrocos] Generating typekit for ${PROJECT_NAME} ${_orocos_typegen_headers_DEP_INFO_MSG}..." )

    # Works in top level source dir:
    set(TYPEGEN_EXE typegen-NOTFOUND) #re-check for typegen each time !
    find_program(TYPEGEN_EXE typegen)
    if(NOT TYPEGEN_EXE)
      message(FATAL_ERROR "'typegen' not found in path. Can't build typekit. Did you 'source env.sh' ?")
    endif()

    foreach(_imp ${_orocos_typegen_headers_DEPENDS})
      set(_orocos_typegen_headers_IMPORTS  ${_orocos_typegen_headers_IMPORTS} -i${_imp})
    endforeach()

    # Working directory is necessary to be able to find the source files.
    execute_process( COMMAND ${TYPEGEN_EXE} --output ${PROJECT_BINARY_DIR}/typekit ${_orocos_typegen_headers_IMPORTS} ${PROJECT_NAME} ${_orocos_typegen_headers_HEADERS}
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
    # work around generated manifest.xml file:
    #execute_process( COMMAND ${CMAKE_COMMAND} -E remove -f ${CMAKE_SOURCE_DIR}/typekit/manifest.xml )
    add_subdirectory(${PROJECT_BINARY_DIR}/typekit ${PROJECT_BINARY_DIR}/typekit)

    get_target_property(_orocos_typegen_headers_OUTPUT_NAME ${PROJECT_NAME}-typekit OUTPUT_NAME)
    list(APPEND OROCOS_DEFINED_TYPES " -l${_orocos_typegen_headers_OUTPUT_NAME}")
    list(APPEND ${PROJECT_NAME}_EXPORTED_TARGETS "${PROJECT_NAME}-typekit")
    list(APPEND ${PROJECT_NAME}_EXPORTED_INCLUDE_DIRS "${PROJECT_BINARY_DIR}/typekit")
    list(APPEND ${PROJECT_NAME}_EXPORTED_LIBRARY_DIRS "${CMAKE_INSTALL_PREFIX}/lib/orocos${OROCOS_SUFFIX}/${PROJECT_NAME}/types")

    # Unset temporary variables
    unset(_orocos_typegen_headers_HEADERS)
    unset(_orocos_typegen_headers_ARGN)
    unset(_orocos_typegen_headers_DEPENDS)
    unset(_orocos_typegen_headers_INSTALL)
    unset(_orocos_typegen_headers_SKIP_INSTALL)
    unset(_orocos_typegen_headers_VERSION)
    unset(_orocos_typegen_headers_OUTPUT_NAME)

  endmacro( orocos_typegen_headers )

  # typekit libraries should add themselves by calling 'orocos_typekit()' 
  # instead of 'add_library' in CMakeLists.txt.
  # You can set a variable COMPONENT_VERSION x.y.z to set a version or 
  # specify the optional VERSION parameter. For ros builds, the version
  # number is ignored.
  #
  # Usage:
  #   orocos_typekit( libraryname src1 [src2 ...] [INSTALL custom/install/destination] [SKIP_INSTALL] [VERSION x.y.z] )
  #      or
  #   add_library( target SHARED src1 [src2 ...] )
  #   orocos_typekit( target [INSTALL custom/install/destination] [SKIP_INSTALL] [VERSION x.y.z] )
  #
  macro( orocos_typekit target )
    cmake_parse_arguments(_orocos_typekit
      "SKIP_INSTALL"
      "INSTALL;VERSION"
      ""
      ${ARGN}
      )

    set(_orocos_typekit_SOURCES ${_orocos_typekit_UNPARSED_ARGUMENTS} )

    # Clear the dependencies such that a target switch can be detected:
    unset( ${target}_LIB_DEPENDS )

    # Build the target:
    if(_orocos_typekit_SOURCES)
      # Use rosbuild in ros environments:
      if (ORO_USE_ROSBUILD)
        message( STATUS "[UseOrocos] Building typekit ${target} in library '${target}' in rosbuild source tree." )
        rosbuild_add_library(${target} ${_orocos_typekit_SOURCES} )
      else()
        message( STATUS "[UseOrocos] Building typekit ${target} in library '${target}'" )
        add_library( ${target} SHARED ${_orocos_typekit_SOURCES} )
      endif()
    endif()

    # Check if the target exists:
    if(NOT TARGET ${target})
      message(FATAL_ERROR "[UseOrocos] Target '${target}' does not exist in orocos_typekit().")
    endif()

    # Configure the target as a typekit library:
    _orocos_target(${target} TYPEKIT ${_orocos_typegen_headers_ARGN} ${ARGN})

    # Necessary for .pc file generation
    get_target_property(_orocos_typekit_OUTPUT_NAME ${target} OUTPUT_NAME)
    list(APPEND OROCOS_DEFINED_TYPES " -l${_orocos_typekit_OUTPUT_NAME}")

    # Unset temporary variables
    unset(_orocos_typekit_SOURCES)
    unset(_orocos_typekit_INSTALL)
    unset(_orocos_typekit_SKIP_INSTALL)
    unset(_orocos_typekit_VERSION)
    unset(_orocos_typekit_OUTPUT_NAME)

  endmacro( orocos_typekit )

  # plugin libraries should add themselves by calling 'orocos_plugin()' 
  # instead of 'add_library' in CMakeLists.txt.
  # You can set a variable COMPONENT_VERSION x.y.z to set a version or 
  # specify the optional VERSION parameter. For ros builds, the version
  # number is ignored.
  #
  # Usage:
  #   orocos_plugin( pluginname src1 [src2 ...] [INSTALL custom/install/destination] [SKIP_INSTALL] [VERSION x.y.z] )
  #      or
  #   add_library( target SHARED src1 [src2 ...] )
  #   orocos_plugin( target [INSTALL custom/install/destination] [SKIP_INSTALL] [VERSION x.y.z] )
  #
  macro( orocos_plugin target )
    cmake_parse_arguments(_orocos_plugin
      "SKIP_INSTALL"
      "INSTALL;VERSION"
      ""
      ${ARGN}
      )

    set(_orocos_plugin_SOURCES ${_orocos_plugin_UNPARSED_ARGUMENTS} )

    # Clear the dependencies such that a target switch can be detected:
    unset( ${target}_LIB_DEPENDS )

    # Build the target:
    if(_orocos_plugin_SOURCES)
      # Use rosbuild in ros environments:
      if (ORO_USE_ROSBUILD)
        message( STATUS "[UseOrocos] Building plugin ${target} in library '${target}' in rosbuild source tree." )
        rosbuild_add_library(${target} ${_orocos_plugin_SOURCES} )
      else()
        message( STATUS "[UseOrocos] Building plugin ${target} in library '${target}'" )
        add_library( ${target} SHARED ${_orocos_plugin_SOURCES} )
      endif()
    endif()

    # Check if the target exists:
    if(NOT TARGET ${target})
      message(FATAL_ERROR "[UseOrocos] orocos_plugin() has been called without source file arguments and target '${target}' does not exist." )
    endif()

    # Configure the target as a plugin library:
    _orocos_target(${target} PLUGIN ${ARGN})

    # Necessary for .pc file generation
    get_target_property(_orocos_plugin_OUTPUT_NAME ${target} OUTPUT_NAME)
    list(APPEND OROCOS_DEFINED_PLUGINS " -l${_orocos_plugin_OUTPUT_NAME}")

    # Unset temporary variables
    unset(_orocos_plugin_SOURCES)
    unset(_orocos_plugin_INSTALL)
    unset(_orocos_plugin_SKIP_INSTALL)
    unset(_orocos_plugin_VERSION)
    unset(_orocos_plugin_OUTPUT_NAME)

  endmacro( orocos_plugin )

  # service libraries should add themselves by calling 'orocos_service()' 
  # instead of 'add_library' in CMakeLists.txt.
  #
  #   orocos_service( servicename src1 [src2 ...] [INSTALL custom/install/destination] [SKIP_INSTALL] [VERSION x.y.z] )
  #      or
  #   add_library( target SHARED src1 [src2 ...] )
  #   orocos_service( target [INSTALL custom/install/destination] [SKIP_INSTALL] [VERSION x.y.z] )
  #
  macro( orocos_service target )
    orocos_plugin( ${target} ${ARGN} )
  endmacro( orocos_service )

  #
  # Components supply header files and directories which should be included when
  # using these components. Each component should use this macro
  # to install its header-files. They are installed by default
  # in include/orocos/${PROJECT_NAME}
  #
  # Usage example: orocos_install_header(
  #                  FILES hardware.hpp control.hpp
  #                  DIRECTORY include/${PROJECT_NAME}
  #                )
  #
  macro( orocos_install_headers )
    cmake_parse_arguments(_orocos_install_headers
      ""
      "INSTALL"
      "FILES;DIRECTORY"
      ${ARGN}
      )

    set( _orocos_install_headers_FILES ${_orocos_install_headers_FILES} ${_orocos_install_headers_UNPARSED_ARGUMENTS} )
    if ( _orocos_install_headers_INSTALL )
      set(_orocos_install_headers_DESTINATION ${_orocos_install_headers_INSTALL})
    else()
      set(_orocos_install_headers_DESTINATION ${ORO_INCLUDE_DESTINATION} )
    endif()

    if( _orocos_install_headers_FILES )
      install( FILES ${_orocos_install_headers_FILES} DESTINATION ${_orocos_install_headers_DESTINATION} )
    endif()

    if( _orocos_install_headers_DIRECTORY )
      install( DIRECTORY ${_orocos_install_headers_DIRECTORY} DESTINATION ${_orocos_install_headers_DESTINATION} )
    endif()

  endmacro( orocos_install_headers )

  #
  # Adds the uninstall target, not present by default in CMake.
  #
  # Usage example: orocos_uninstall_target()
  macro( orocos_uninstall_target )
    if (NOT OROCOS_UNINSTALL_DONE AND NOT TARGET uninstall)
      CONFIGURE_FILE(
        "${OROCOS-RTT_USE_FILE_PATH}/cmake_uninstall.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
        IMMEDIATE @ONLY)

      ADD_CUSTOM_TARGET(uninstall
        "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake")
    endif (NOT OROCOS_UNINSTALL_DONE AND NOT TARGET uninstall)
    set(OROCOS_UNINSTALL_DONE)
  endmacro( orocos_uninstall_target )

  #
  # Generate package files for the whole project. Do this as the very last
  # step in your project's CMakeLists.txt file.
  #
  # Allows to set a name for the .pc file (without extension)
  # and a version (defaults to 1.0). The name and version you provide will
  # be used unmodified.
  #
  # If you didn't specify VERSION but COMPONENT_VERSION has been set,
  # that variable will be used to set the version number.
  #
  # You may specify a dependency list of .pc files to depend on with DEPENDS. You will need this
  # to set the include paths correctly if a public header of
  # this package includes a header of another (non-Orocos) package. This dependency
  # will end up in the Requires: field of the .pc file.
  #
  # You may specify a dependency list of .pc files of Orocos packages with DEPENDS_TARGETS
  # This is similar to DEPENDS, but the -<target> suffix is added for every package name.
  # This dependency will end up in the Requires: field of the .pc file.
  #
  # orocos_generate_package( [name] [VERSION version] [DEPENDS packagenames....])
  #
  macro( orocos_generate_package )

    cmake_parse_arguments(_orocos_generate_package
      ""
      "VERSION"
      "DEPENDS;DEPENDS_TARGETS;INCLUDE_DIRS"
      ${ARGN}
      )

    # Check version
    if (NOT _orocos_generate_package_VERSION)
      if (COMPONENT_VERSION)
        set( _orocos_generate_package_VERSION ${COMPONENT_VERSION})
        message(STATUS "[UseOrocos] Generating package version ${_orocos_generate_package_VERSION} from COMPONENT_VERSION.")
      elseif (${PROJECT_NAME}_VERSION)
        set( _orocos_generate_package_VERSION ${${PROJECT_NAME}_VERSION})
        message(STATUS "[UseOrocos] Generating package version ${_orocos_generate_package_VERSION} from ${PROJECT_NAME}_VERSION (package.xml).")
      else ()
        set( _orocos_generate_package_VERSION "1.0")
        message(STATUS "[UseOrocos] Generating package version ${_orocos_generate_package_VERSION} (default version).")
      endif (COMPONENT_VERSION)
    else (NOT _orocos_generate_package_VERSION)
      message(STATUS "[UseOrocos] Generating package version ${_orocos_generate_package_VERSION}.")
    endif (NOT _orocos_generate_package_VERSION)

    # Create filename
    if ( _orocos_generate_package_UNPARSED_ARGUMENTS )
      set(PC_NAME ${_orocos_generate_package_UNPARSED_ARGUMENTS})
    else ( _orocos_generate_package_UNPARSED_ARGUMENTS )
      set(PACKAGE_NAME ${PROJECT_NAME} )
      if ( NOT CMAKE_CURRENT_SOURCE_DIR STREQUAL ${PROJECT_NAME}_SOURCE_DIR )
        # Append -subdir-subdir-... to pc name:
        file(RELATIVE_PATH RELPATH ${${PROJECT_NAME}_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR} )
        string(REPLACE "/" "-" PC_NAME_SUFFIX ${RELPATH} )
        set(PACKAGE_NAME ${PACKAGE_NAME}-${PC_NAME_SUFFIX})
      endif ( NOT CMAKE_CURRENT_SOURCE_DIR STREQUAL ${PROJECT_NAME}_SOURCE_DIR )
      set(PC_NAME ${PACKAGE_NAME}-${OROCOS_TARGET})
    endif ( _orocos_generate_package_UNPARSED_ARGUMENTS )

    # Create dependency list
    set(PC_DEPENDS ${_orocos_generate_package_DEPENDS})
    foreach( DEP ${_orocos_generate_package_DEPENDS_TARGETS})
      list(APPEND PC_DEPENDS ${DEP}-${OROCOS_TARGET})
    endforeach()
    string(REPLACE ";" " " PC_DEPENDS "${PC_DEPENDS}")

    # Create lib-path list
    set(PC_LIBS "Libs: ")
    if (OROCOS_DEFINED_LIBS)
      set(PC_LIBS "${PC_LIBS} -L\${libdir} ${OROCOS_DEFINED_LIBS}")
    endif (OROCOS_DEFINED_LIBS)
    if (OROCOS_DEFINED_COMPS)
      set(PC_LIBS "${PC_LIBS} -L\${orocos_libdir} ${OROCOS_DEFINED_COMPS}")
    endif (OROCOS_DEFINED_COMPS)
    if (OROCOS_DEFINED_PLUGINS)
      set(PC_LIBS "${PC_LIBS} -L\${orocos_libdir}/plugins ${OROCOS_DEFINED_PLUGINS}")
    endif (OROCOS_DEFINED_PLUGINS)
    if (OROCOS_DEFINED_TYPES)
      set(PC_LIBS "${PC_LIBS} -L\${orocos_libdir}/types ${OROCOS_DEFINED_TYPES}")
    endif (OROCOS_DEFINED_TYPES)

    set(PC_PREFIX ${CMAKE_INSTALL_PREFIX})
    set(PC_LIB_DIR "\${libdir}/orocos${OROCOS_SUFFIX}/${PROJECT_NAME}")
    set(PC_EXTRA_INCLUDE_DIRS "")
    set(PC_COMMENT "# This pkg-config file is for use in an installed system")

    set(PC_CONTENTS "# Orocos pkg-config file generated by orocos_generate_package() 
\@PC_COMMENT\@
prefix=\@PC_PREFIX\@
libdir=\${prefix}/lib
includedir=\${prefix}/include/orocos
orocos_libdir=\@PC_LIB_DIR\@

Name: \@PC_NAME\@
Description: \@PC_NAME\@ package for Orocos
Requires: orocos-rtt-\@OROCOS_TARGET\@ \@PC_DEPENDS@
Version: \@_orocos_generate_package_VERSION\@
\@PC_LIBS\@
Cflags: -I\${includedir} \@PC_EXTRA_INCLUDE_DIRS\@
")

    string(CONFIGURE "${PC_CONTENTS}" INSTALLED_PC_CONTENTS @ONLY)
    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/${PC_NAME}.pc ${INSTALLED_PC_CONTENTS})

    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${PC_NAME}.pc DESTINATION lib/pkgconfig )
    #install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/manifest.xml DESTINATION  lib/orocos${OROCOS_SUFFIX}/level0 )

    # Add _orocos_generate_package_INCLUDE_DIRS arguments to ${PROJECT_NAME}_EXPORTED_INCLUDE_DIRS
    if(_orocos_generate_package_INCLUDE_DIRS)
      foreach(include_dir ${_orocos_generate_package_INCLUDE_DIRS})
        if(IS_ABSOLUTE ${include_dir})
          list(APPEND ${PROJECT_NAME}_EXPORTED_INCLUDE_DIRS "${include_dir}")
        else()
          list(APPEND ${PROJECT_NAME}_EXPORTED_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/${include_dir}")
        endif()
      endforeach()

    else()
      # If the directory ${PROJECT_SOURCE_DIR}/include/orocos exists, always export it as a fallback
      if(EXISTS "${PROJECT_SOURCE_DIR}/include/orocos")
        list(APPEND ${PROJECT_NAME}_EXPORTED_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}/include/orocos")
      endif()
    endif()

    # Generate additional pkg-config files for other build toolchains
    if (ORO_USE_ROSBUILD)
      message(STATUS "[UseOrocos] Generating pkg-config file for rosbuild package.")

      # For ros package trees, we install the .pc file also next to the manifest file:
      set(PC_PREFIX ${PROJECT_SOURCE_DIR})
      #set(PC_LIB_DIR "\${libdir}/orocos${OROCOS_SUFFIX}") # Without package name suffix !
      set(PC_EXTRA_INCLUDE_DIRS "-I\${prefix}/..")
      foreach(include_dir ${${PROJECT_NAME}_EXPORTED_INCLUDE_DIRS})
        if(NOT include_dir STREQUAL "${PC_PREFIX}/include/orocos")
          set(PC_EXTRA_INCLUDE_DIRS "${PC_EXTRA_INCLUDE_DIRS} -I${include_dir}")
        endif()
      endforeach()
        
      set(PC_COMMENT "# This pkg-config file is for use in a rosbuild source tree\n"
        "# Rationale:\n"
        "# - The prefix is equal to the package directory.\n"
        "# - The libdir is where the libraries were built, ie, package/lib\n"
        "# - The include dir in cflags allows top-level headers and in package/include/package/header.h\n"
        "# - If this doesn't fit your package layout, don't use orocos_generate_package() and write the .pc file yourself")

      string(CONFIGURE "${PC_CONTENTS}" ROSBUILD_PC_CONTENTS @ONLY)
      file(WRITE ${PROJECT_SOURCE_DIR}/lib/pkgconfig/${PC_NAME}.pc ${ROSBUILD_PC_CONTENTS})

    elseif (ORO_USE_CATKIN)
      message(STATUS "[UseOrocos] Generating pkg-config file for package in catkin devel space.")

      # For catkin workspaces we also install a pkg-config file in the develspace
      set(PC_COMMENT "# This pkg-config file is for use in a catkin devel space")
      set(PC_PREFIX ${CATKIN_DEVEL_PREFIX})
      set(PC_EXTRA_INCLUDE_DIRS "")
      foreach(include_dir ${${PROJECT_NAME}_EXPORTED_INCLUDE_DIRS})
        if(NOT include_dir STREQUAL "${PC_PREFIX}/include/orocos")
          set(PC_EXTRA_INCLUDE_DIRS "${PC_EXTRA_INCLUDE_DIRS} -I${include_dir}")
        endif()
      endforeach()
      #set(PC_LIB_DIR "\${libdir}/orocos${OROCOS_SUFFIX}/${PROJECT_NAME}")

      string(CONFIGURE "${PC_CONTENTS}" CATKIN_PC_CONTENTS @ONLY)
      file(WRITE ${CATKIN_DEVEL_PREFIX}/lib/pkgconfig/${PC_NAME}.pc ${CATKIN_PC_CONTENTS})

      # Create install target for orocos installed package directory
      FILE(MAKE_DIRECTORY ${CATKIN_DEVEL_PREFIX}/lib/orocos${OROCOS_SUFFIX}/${PROJECT_NAME})
    endif()

    # Append exported targets, libraries and include directories of all dependencies
    set(${PROJECT_NAME}_EXPORTED_LIBRARIES ${${PROJECT_NAME}_EXPORTED_TARGETS})
    foreach(_depend ${_orocos_generate_package_DEPENDS} ${_orocos_generate_package_DEPENDS_TARGETS})
      list(APPEND ${PROJECT_NAME}_EXPORTED_TARGETS      ${${_depend}_EXPORTED_TARGETS})
      list(APPEND ${PROJECT_NAME}_EXPORTED_LIBRARIES    ${${_depend}_LIBRARIES})
      list(APPEND ${PROJECT_NAME}_EXPORTED_INCLUDE_DIRS ${${_depend}_INCLUDE_DIRS})
      list(APPEND ${PROJECT_NAME}_EXPORTED_LIBRARY_DIRS ${${_depend}_LIBRARY_DIRS})
    endforeach()

    if(${PROJECT_NAME}_EXPORTED_TARGETS)
      list(REMOVE_DUPLICATES ${PROJECT_NAME}_EXPORTED_TARGETS)
    endif()
    if(${PROJECT_NAME}_EXPORTED_LIBRARIES)
      list(REMOVE_DUPLICATES ${PROJECT_NAME}_EXPORTED_LIBRARIES)
    endif()
    if(${PROJECT_NAME}_EXPORTED_INCLUDE_DIRS)
      list(REMOVE_DUPLICATES ${PROJECT_NAME}_EXPORTED_INCLUDE_DIRS)
    endif()
    if(${PROJECT_NAME}_EXPORTED_LIBRARY_DIRS)
      list(REMOVE_DUPLICATES ${PROJECT_NAME}_EXPORTED_LIBRARY_DIRS)
    endif()

    # Store a list of exported targets, libraries and include directories on the cache so that other packages within the same workspace can use them.
    set(${PC_NAME}_OROCOS_PACKAGE True CACHE INTERNAL "Mark ${PC_NAME} package as an Orocos package built in this workspace")
    if(${PROJECT_NAME}_EXPORTED_TARGETS)
      message(STATUS "[UseOrocos] Exporting targets ${${PROJECT_NAME}_EXPORTED_TARGETS}.")
      set(${PC_NAME}_EXPORTED_OROCOS_TARGETS ${${PROJECT_NAME}_EXPORTED_TARGETS} CACHE INTERNAL "Targets exported by package ${PC_NAME}")
    endif()
    if(${PROJECT_NAME}_EXPORTED_LIBRARIES)
      message(STATUS "[UseOrocos] Exporting libraries ${${PROJECT_NAME}_EXPORTED_LIBRARIES}.")
      set(${PC_NAME}_EXPORTED_OROCOS_LIBRARIES ${${PROJECT_NAME}_EXPORTED_LIBRARIES} CACHE INTERNAL "Libraries exported by package ${PC_NAME}")
    endif()
    if(${PROJECT_NAME}_EXPORTED_INCLUDE_DIRS)
      message(STATUS "[UseOrocos] Exporting include directories ${${PROJECT_NAME}_EXPORTED_INCLUDE_DIRS}.")
      set(${PC_NAME}_EXPORTED_OROCOS_INCLUDE_DIRS ${${PROJECT_NAME}_EXPORTED_INCLUDE_DIRS} CACHE INTERNAL "Include directories exported by package ${PC_NAME}")
    endif()
    if(${PROJECT_NAME}_EXPORTED_LIBRARY_DIRS)
      message(STATUS "[UseOrocos] Exporting library directories ${${PROJECT_NAME}_EXPORTED_LIBRARY_DIRS}.")
      set(${PC_NAME}_EXPORTED_OROCOS_LIBRARY_DIRS ${${PROJECT_NAME}_EXPORTED_LIBRARY_DIRS} CACHE INTERNAL "Library directories exported by package ${PC_NAME}")
    endif()

    # Also set the uninstall target:
    orocos_uninstall_target()

    # Create install target for orocos installed package directory
    install(CODE "FILE(MAKE_DIRECTORY \$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/orocos${OROCOS_SUFFIX}/${PROJECT_NAME})")

    # Call catkin_package() here if the user has not called it before.
    if( ORO_USE_CATKIN
        AND NOT ${PROJECT_NAME}_CATKIN_PACKAGE
        AND NOT _orocos_generate_package_UNPARSED_ARGUMENTS # no package name given in orocos_generate_package()
        AND CMAKE_CURRENT_SOURCE_DIR STREQUAL ${PROJECT_NAME}_SOURCE_DIR
        AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/package.xml" )

      # Always assume that catkin is a buildtool_depend. This silently disables a FATAL_ERROR in catkin_package().
      # See https://github.com/ros/catkin/commit/7482dda520e94db5b532b57220dfefb10eeda15b
      list(APPEND ${PROJECT_NAME}_BUILDTOOL_DEPENDS catkin)

      catkin_package()
    endif()

  endmacro( orocos_generate_package )

elseif(NOT OROCOS-RTT_FOUND)
  message(FATAL_ERROR "UseOrocos.cmake file included, but OROCOS-RTT_FOUND not set ! Be sure to run first find_package(OROCOS-RTT) before including this file.")
endif()
