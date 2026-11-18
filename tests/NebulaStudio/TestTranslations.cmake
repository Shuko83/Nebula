# Copyright (C) 2026 Fabien Di Grande
# SPDX-License-Identifier: MPL-2.0

# Every text of the translations of NebulaStudio is translated: cmake -D "TS_FILES=<file>;..." -P TestTranslations.cmake.
# A text added to the sources comes here once the target NebulaStudioUpdateTranslations has run.
foreach(_file IN LISTS TS_FILES)
    file(READ "${_file}" _content)
    string(REGEX MATCHALL "type=\"(unfinished|vanished|obsolete)\"" _left "${_content}")
    list(LENGTH _left _count)
    if(NOT _count EQUAL 0)
        message(FATAL_ERROR "${_file}: ${_count} texts are not translated")
    endif()
endforeach()
