llvm_map_components_to_libnames(llvm_libs bitwriter core ipo irreader instcombine instrumentation target linker analysis scalaropts support )
file (GLOB SOURCES
   *.cpp
)



set(JAVA_AWT_LIBRARY NotNeeded)
set(JAVA_JVM_LIBRARY NotNeeded)
set(JAVA_AWT_INCLUDE_PATH NotNeeded)

find_package(JNI)

if (JNI_FOUND)
    message (STATUS "JNI_INCLUDE_DIRS=${JNI_INCLUDE_DIRS}")
    message (STATUS "JNI_LIBRARIES=${JNI_LIBRARIES}")
else()
    message (STATUS "Could not find JNI includes. Is JAVA_HOME set?")
endif()
include_directories(${JNI_INCLUDE_DIRS})


# uncomment to generate native executable. 
add_executable(svf-ex ${SOURCES}
        detectJNICalls.cpp
        detectJNICalls.h
        CustomAndersen.cpp
        CustomAndersen.h
        svfjava_interface.cpp
        svf-ex.h)
target_compile_options(svf-ex PRIVATE
    $<$<CXX_COMPILER_ID:Clang>:-g>
    )
add_library(svf-lib SHARED ${SOURCES})
target_link_libraries(svf-ex ${SVF_LIB} ${llvm_libs})
target_link_libraries(svf-ex ${Z3_LIBRARIES})
target_link_libraries(svf-lib ${SVF_LIB} ${llvm_libs})
target_link_libraries(svf-lib ${Z3_LIBRARIES})
if (SVF_INSTALL_EXTAPI_FILE)
else ()
    set(SVF_INSTALL_EXTAPI_FILE "${SVF_DIR}/Release-build/svf-llvm/extapi.bc")
endif ()
message (STATUS "Setting SVF_INSTALL_EXTAPI_FILE=${SVF_INSTALL_EXTAPI_FILE}")
target_compile_definitions(svf-ex PUBLIC SVF_INSTALL_EXTAPI_FILE=\"${SVF_INSTALL_EXTAPI_FILE}\")
target_compile_definitions(svf-lib PUBLIC SVF_INSTALL_EXTAPI_FILE=\"${SVF_INSTALL_EXTAPI_FILE}\")
set_target_properties( svf-ex PROPERTIES
                       RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin )
