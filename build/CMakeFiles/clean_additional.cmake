# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "CMakeFiles/QtVirtualKeyboard_autogen.dir/AutogenUsed.txt"
  "CMakeFiles/QtVirtualKeyboard_autogen.dir/ParseCache.txt"
  "QtVirtualKeyboard_autogen"
  )
endif()
