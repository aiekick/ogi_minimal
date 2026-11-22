# THE CORE IS A TARGET, AND ITS EXISTENCE IS TESTED : when a parent project
# already provides ogi (the one that develops it builds every part against its
# single core), that one wins and nothing is brought twice ; otherwise an
# explicit -DOGI_ROOT_DIR=<path to ogi> is built ; otherwise the frozen commit
# below is fetched into the build tree (_deps) ; and a missing core is refused
# by name. moving to another ogi = changing OGI_FETCH_TAG
set(OGI_FETCH_REPOSITORY "https://forge.funparadigm.com/ogi/ogi.git" CACHE STRING "the ogi repository fetched when neither the ogi target nor OGI_ROOT_DIR exist")
set(OGI_FETCH_TAG "73911602b1123b8857b310280b52b915ff731d7e" CACHE STRING "the frozen commit of ogi fetched")

if(NOT TARGET ogi)
    include(FetchContent)

    # a local checkout replaces the download ; the fetched copy that ogi
    # itself records in OGI_ROOT_DIR is not one
    if(OGI_ROOT_DIR AND NOT OGI_ROOT_DIR STREQUAL "${FETCHCONTENT_BASE_DIR}/ogi-src")
        if(NOT EXISTS ${OGI_ROOT_DIR}/CMakeLists.txt)
            message(FATAL_ERROR "ogi_minimal : OGI_ROOT_DIR=${OGI_ROOT_DIR} holds no ogi repository (no CMakeLists.txt there)")
        endif()
        set(FETCHCONTENT_SOURCE_DIR_OGI ${OGI_ROOT_DIR})
    endif()

    FetchContent_Declare(ogi
        GIT_REPOSITORY ${OGI_FETCH_REPOSITORY}
        GIT_TAG ${OGI_FETCH_TAG}
    )
    FetchContent_MakeAvailable(ogi)

    if(NOT TARGET ogi)
        message(FATAL_ERROR "ogi_minimal needs the ogi target : ${OGI_FETCH_REPOSITORY} at ${OGI_FETCH_TAG} gave none, pass -DOGI_ROOT_DIR=<path to ogi>")
    endif()
    set(OGI_ROOT_DIR ${ogi_SOURCE_DIR})
endif()
