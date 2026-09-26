if(NOT TARGET glad)
    add_lib_archive(glad STATIC
        ARCHIVE ${CONTRIB_DIR}/libs/glad.tar.gz
        FILES    
            src/glad.c 
            include/glad/glad.h
        INCLUDES 
            include
        FOLDER 3rdparty
    )
endif()
