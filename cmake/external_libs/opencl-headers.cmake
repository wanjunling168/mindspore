if(ENABLE_GITEE_EULER)
    set(GIT_REPOSITORY "git@gitee.com:src-openeuler/opencl-clhpp.git")
    set(GIT_TAG "7347fa1bb52ebee9f3d6c44ff65ef3c4253cab79")
    set(SHA256 "d41d8cd98f00b204e9800998ecf8427e")

    if(EXISTS "${CMAKE_BINARY_DIR}/_deps/opencl-clhpp-src")
        # Extracting tarball into git repository would make git-status tainted, and case cmake rebuild error.
        # Here we clean source dir before rebuild to fix this error.
        file(REMOVE_RECURSE "${CMAKE_BINARY_DIR}/_deps/opencl-clhpp-src")
        file(REMOVE_RECURSE "${CMAKE_BINARY_DIR}/_deps/opencl-clhpp-build")
        file(REMOVE_RECURSE "${CMAKE_BINARY_DIR}/_deps/opencl-clhpp-subbuild")
    endif()

    __download_pkg_with_git(OpenCL-CLHPP ${GIT_REPOSITORY} ${GIT_TAG} ${SHA256})
    set(OPENCL_CLHPP_SRC "${CMAKE_BINARY_DIR}/_deps/opencl-clhpp-src")
    execute_process(COMMAND tar -xf ${OPENCL_CLHPP_SRC}/v2.0.12.tar.gz --strip-components 1 -C ${OPENCL_CLHPP_SRC})

    set(OPENCL_HEADER_SRC "${CMAKE_BINARY_DIR}/_deps/opencl-headers-src")
    file(MAKE_DIRECTORY "${OPENCL_HEADER_SRC}")
    execute_process(COMMAND tar -xf ${OPENCL_CLHPP_SRC}/v2020.12.18.tar.gz --strip-components 1 -C ${OPENCL_HEADER_SRC})
elseif(ENABLE_GITEE)
    set(REQ_URL "https://gitee.com/mirrors/OpenCL-Headers/repository/archive/v2020.12.18.tar.gz")# VER v2020.12.18
    set(SHA256 "076251b94284b931399ee525527bc9aef3f5f6f3f3b1964ae485218cc88956ba")
    __download_pkg(OpenCL-Headers ${REQ_URL} ${SHA256})
else()
    set(REQ_URL "https://github.com/KhronosGroup/OpenCL-Headers/archive/v2020.12.18.tar.gz")
    set(SHA256 "5dad6d436c0d7646ef62a39ef6cd1f3eba0a98fc9157808dfc1d808f3705ebc2")
    __download_pkg(OpenCL-Headers ${REQ_URL} ${SHA256})
endif()

function(gene_opencl CL_SRC_DIR)
    message(STATUS "**********gene opencl********* cl path: " "${CL_SRC_DIR}")
    if(NOT EXISTS ${CL_SRC_DIR})
        return()
    endif()
    file(GLOB_RECURSE CL_LIST ${CL_SRC_DIR}/*.cl)
    foreach(file_path ${CL_LIST})
        set(out_file_path "${file_path}.inc")
        file(REMOVE ${out_file_path})

        string(REGEX REPLACE ".+/(.+)\\..*" "\\1" kernel_name "${file_path}")
        file(READ ${file_path} cl_program)
        string(CONCAT cl_str "static const std::string ${kernel_name}_source = R\"(\n" "${cl_program}" ")\";")
        file(WRITE ${out_file_path} "${cl_str}")
    endforeach()
endfunction()
