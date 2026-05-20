set(NOTEPAD_VENDOR_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lib" CACHE PATH
    "Directory containing vendored third-party libraries")
set(NOTEPAD_COPY_RUNTIME_DLLS ON CACHE BOOL
    "Copy vendored runtime DLLs next to the built executable on Windows")

function(notepad_copy_runtime_dll target dll_path)
    if(WIN32 AND NOTEPAD_COPY_RUNTIME_DLLS AND EXISTS "${dll_path}")
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${dll_path}"
                    "$<TARGET_FILE_DIR:${target}>"
            VERBATIM
        )
    endif()
endfunction()

function(notepad_configure_ela target)
    set(ela_root "${NOTEPAD_VENDOR_DIR}/ElaWidgetTools")
    set(ela_include "${ela_root}/include")

    if(NOT EXISTS "${ela_include}")
        message(FATAL_ERROR "ElaWidgetTools headers were not found: ${ela_include}")
    endif()

    find_library(ELA_WIDGET_TOOLS_LIBRARY
        NAMES ElaWidgetTools libElaWidgetTools
        PATHS "${ela_root}/lib"
        NO_DEFAULT_PATH
    )

    if(NOT ELA_WIDGET_TOOLS_LIBRARY)
        message(FATAL_ERROR "ElaWidgetTools library was not found under ${ela_root}/lib")
    endif()

    target_include_directories(${target} PRIVATE "${ela_include}")
    target_link_libraries(${target} PRIVATE "${ELA_WIDGET_TOOLS_LIBRARY}")
    notepad_copy_runtime_dll(${target} "${ela_root}/bin/ElaWidgetTools.dll")
endfunction()

function(notepad_configure_qscintilla target)
    set(qsci_root "${NOTEPAD_VENDOR_DIR}/QScintilla")
    set(qsci_include "${qsci_root}/include")

    if(NOT EXISTS "${qsci_include}")
        message(FATAL_ERROR "QScintilla headers were not found: ${qsci_include}")
    endif()

    find_library(QSCINTILLA_RELEASE_LIBRARY
        NAMES qscintilla2_qt6
        PATHS "${qsci_root}/lib"
        NO_DEFAULT_PATH
    )
    find_library(QSCINTILLA_DEBUG_LIBRARY
        NAMES qscintilla2_qt6d
        PATHS "${qsci_root}/lib"
        NO_DEFAULT_PATH
    )

    if(QSCINTILLA_RELEASE_LIBRARY AND QSCINTILLA_DEBUG_LIBRARY)
        target_link_libraries(${target} PRIVATE
            optimized "${QSCINTILLA_RELEASE_LIBRARY}"
            debug "${QSCINTILLA_DEBUG_LIBRARY}"
        )
    elseif(QSCINTILLA_DEBUG_LIBRARY)
        message(WARNING "Only debug QScintilla library found; Release builds need qscintilla2_qt6.")
        target_link_libraries(${target} PRIVATE "${QSCINTILLA_DEBUG_LIBRARY}")
    elseif(QSCINTILLA_RELEASE_LIBRARY)
        message(WARNING "Only release QScintilla library found; Debug builds need qscintilla2_qt6d.")
        target_link_libraries(${target} PRIVATE "${QSCINTILLA_RELEASE_LIBRARY}")
    else()
        message(FATAL_ERROR "QScintilla library was not found under ${qsci_root}/lib")
    endif()

    target_include_directories(${target} PRIVATE "${qsci_include}")
    notepad_copy_runtime_dll(${target} "${qsci_root}/lib/qscintilla2_qt6.dll")
    notepad_copy_runtime_dll(${target} "${qsci_root}/lib/qscintilla2_qt6d.dll")
endfunction()

function(notepad_configure_third_party target)
    notepad_configure_ela(${target})
    notepad_configure_qscintilla(${target})
endfunction()
