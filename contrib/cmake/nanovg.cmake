if(NOT TARGET nanovg)
    add_lib_archive(nanovg STATIC
        ARCHIVE ${CONTRIB_DIR}/libs/nanovg.zip
        FILES    
            src/nanovg.c 
            src/nanovg.h
        INCLUDES 
            src
        FOLDER 3rdparty
    )
endif()
