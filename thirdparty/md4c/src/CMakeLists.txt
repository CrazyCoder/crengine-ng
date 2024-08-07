
add_library(cre_md4c OBJECT md4c.c md4c.h)
set_target_properties(cre_md4c PROPERTIES
    COMPILE_FLAGS "-DMD4C_USE_UTF8"
)

# Build rules for HTML renderer library

add_library(cre_md4c-html OBJECT md4c-html.c md4c-html.h entity.c entity.h md4c.c md4c.h)
set_target_properties(cre_md4c-html PROPERTIES
    COMPILE_FLAGS "-DMD4C_USE_UTF8"
)
