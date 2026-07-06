# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "")
  file(REMOVE_RECURSE
  "CMakeFiles\\Open-LightSync_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\Open-LightSync_autogen.dir\\ParseCache.txt"
  "Open-LightSync_autogen"
  )
endif()
