# Finds ftw.h
# Sets: FTW_INCLUDE_DIR

if (FTW_INCLUDE_DIR)
  # Already in cache, be silent
  set(FTW_FIND_QUIETLY TRUE)
endif (FTW_INCLUDE_DIR)

find_path(FTW_INCLUDE_DIR ftw.h
  /opt/local/include
  /usr/local/include
  /usr/include
)

if (FTW_INCLUDE_DIR)
   set(FTW_FOUND TRUE)
else (FTW_INCLUDE_DIR)
   set(FTW_FOUND FALSE)
endif (FTW_INCLUDE_DIR)

if (FTW_FOUND)
   if (NOT FTW_FIND_QUIETLY)
	   message(STATUS "Found FTW: ${FTW_INCLUDE_DIR}")
   endif (NOT FTW_FIND_QUIETLY)
else (FTW_FOUND)
   if (FTW_FIND_REQUIRED)
      message(STATUS "Looked for FTW file named ftw.h.")
      message(STATUS "Include file detected: [${FTW_INCLUDE_DIR}].")
      message(FATAL_ERROR "=========> Could NOT find FTW library")
   endif (FTW_FIND_REQUIRED)
endif (FTW_FOUND)

mark_as_advanced(
	FTW_INCLUDE_DIR
  )

