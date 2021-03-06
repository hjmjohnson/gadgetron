if (WIN32)
  ADD_DEFINITIONS(-D__BUILD_GADGETRON_CMR__)
endif (WIN32)

include_directories(
    ${Boost_INCLUDE_DIR} 
    ${ARMADILLO_INCLUDE_DIRS} 
    ${ACE_INCLUDE_DIR} 
    ${ISMRMRD_INCLUDE_DIR}
    ${FFTW3_INCLUDE_DIR}
    ${MKL_INCLUDE_DIR}
    ${CMAKE_SOURCE_DIR}/toolboxes/core
    ${CMAKE_SOURCE_DIR}/toolboxes/core/cpu
    ${CMAKE_SOURCE_DIR}/toolboxes/core/cpu/image
    ${CMAKE_SOURCE_DIR}/toolboxes/fft/cpu
    ${CMAKE_SOURCE_DIR}/toolboxes/klt/cpu
    ${CMAKE_SOURCE_DIR}/toolboxes/dwt/cpu
    ${CMAKE_SOURCE_DIR}/toolboxes/core/cpu/hostutils
    ${CMAKE_SOURCE_DIR}/toolboxes/core/cpu/math
    ${CMAKE_SOURCE_DIR}/toolboxes/core/cpu/image
    ${CMAKE_SOURCE_DIR}/toolboxes/core/cpu/algorithm
    ${CMAKE_SOURCE_DIR}/toolboxes/registration/optical_flow/cpu
    ${CMAKE_SOURCE_DIR}/toolboxes/registration/optical_flow/cpu/application
    ${CMAKE_SOURCE_DIR}/toolboxes/registration/optical_flow/cpu/dissimilarity
    ${CMAKE_SOURCE_DIR}/toolboxes/registration/optical_flow/cpu/register
    ${CMAKE_SOURCE_DIR}/toolboxes/registration/optical_flow/cpu/solver
    ${CMAKE_SOURCE_DIR}/toolboxes/registration/optical_flow/cpu/transformation
    ${CMAKE_SOURCE_DIR}/toolboxes/registration/optical_flow/cpu/warper
    ${CMAKE_SOURCE_DIR}/toolboxes/operators
    ${CMAKE_SOURCE_DIR}/toolboxes/operators/cpu
    ${CMAKE_SOURCE_DIR}/toolboxes/solvers
    ${CMAKE_SOURCE_DIR}/toolboxes/solvers/cpu
    ${CMAKE_SOURCE_DIR}/toolboxes/mri_core
    ${CMAKE_SOURCE_DIR}/toolboxes/image_io
  )

set(cmr_header_fiels cmr_export.h 
                    cmr_kspace_binning.h
                    cmr_time_stamp.h 
                    cmr_motion_correction.h 
                    cmr_spirit_recon.h )

set(cmr_src_fiels cmr_kspace_binning.cpp 
                cmr_time_stamp.cpp 
                cmr_motion_correction.cpp 
                cmr_spirit_recon.cpp )

add_library(gadgetron_toolbox_cmr SHARED 
            ${cmr_header_fiels} 
            ${cmr_src_fiels} )

set_target_properties(gadgetron_toolbox_cmr PROPERTIES VERSION ${GADGETRON_VERSION_STRING} SOVERSION ${GADGETRON_SOVERSION})

target_link_libraries(gadgetron_toolbox_cmr
                    ${ISMRMRD_LIBRARIES}
                    gadgetron_toolbox_cpucore
                    gadgetron_toolbox_log
                    gadgetron_toolbox_cpucore_math
                    gadgetron_toolbox_cpufft 
                    gadgetron_toolbox_cpuklt 
                    gadgetron_toolbox_image_analyze_io 
                    gadgetron_toolbox_mri_core 
                    gadgetron_toolbox_cpudwt 
                    gadgetron_toolbox_cpuoperator )

install(TARGETS gadgetron_toolbox_cmr DESTINATION lib COMPONENT main)

install(FILES ${cmr_header_fiels} 
      DESTINATION ${GADGETRON_INSTALL_INCLUDE_PATH} COMPONENT main)
