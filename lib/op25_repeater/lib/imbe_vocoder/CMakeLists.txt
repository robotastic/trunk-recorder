cmake_minimum_required(VERSION 2.6)

SET(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
SET(BUILD_SHARED_LIBS OFF)
SET(CMAKE_EXE_LINKER_FLAGS "-static")
SET(Boost_USE_STATIC_LIBS   ON)
SET( MORE_FLAGS "-fPIC")
SET( CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${MORE_FLAGS}" )

list(APPEND imbe_vocoder_sources
    aux_sub.cc
    basicop2.cc
    ch_decode.cc
    ch_encode.cc
    dc_rmv.cc
    decode.cc
    dsp_sub.cc
    encode.cc
    imbe_vocoder.cc
    math_sub.cc
    pe_lpf.cc
    pitch_est.cc
    pitch_ref.cc
    qnt_sub.cc
    rand_gen.cc
    sa_decode.cc
    sa_encode.cc
    sa_enh.cc
    tbls.cc
    uv_synt.cc
    v_synt.cc
    v_uv_det.cc
)

add_library(imbe_vocoder ${imbe_vocoder_sources})
