cmake_minimum_required(VERSION 3.16)
project(B1 LANGUAGES CXX)
find_package(Geant4 REQUIRED)
file(GLOB sources CONFIGURE_DEPENDS ${PROJECT_SOURCE_DIR}/src/*.cc)
file(GLOB headers CONFIGURE_DEPENDS ${PROJECT_SOURCE_DIR}/include/*.hh)
add_executable(exampleB1 exampleB1.cc ${sources} ${headers})
target_compile_features(exampleB1 PRIVATE cxx_std_17)
target_include_directories(exampleB1 PRIVATE include)
target_link_libraries(exampleB1 PRIVATE ${Geant4_LIBRARIES})
foreach(macro reference.mac run_block.mac run1.mac run2.mac)
  configure_file(${PROJECT_SOURCE_DIR}/${macro} ${PROJECT_BINARY_DIR}/${macro} COPYONLY)
endforeach()

# Distance study: copy user-run macros and ROOT analysis to build.
foreach(extra
  reference_11cm.mac block_11cm.mac run_11cm.sh
  reference_8cm.mac block_8cm.mac run_8cm.sh
  reference_10cm.mac block_10cm.mac
  reference_15cm.mac block_15cm.mac
  reference_20cm.mac block_20cm.mac
  run_mesafe.sh mesafe_analiz.C MESAFE_KULLANIM.md)
  configure_file(${PROJECT_SOURCE_DIR}/${extra} ${PROJECT_BINARY_DIR}/${extra} COPYONLY)
endforeach()

# Centered 10x7 cm overlap study (started manually by the user).
foreach(extra reference_10x7.mac block_10x7.mac run_alan_10x7.sh analiz_alan.C)
  configure_file(${PROJECT_SOURCE_DIR}/${extra} ${PROJECT_BINARY_DIR}/${extra} COPYONLY)
endforeach()

# Independent repeat: restored 10x10 cm overlap and 10 cm face gap.
foreach(extra reference_10x10.mac block_10x10.mac run_alan_10x10.sh)
  configure_file(${PROJECT_SOURCE_DIR}/${extra} ${PROJECT_BINARY_DIR}/${extra} COPYONLY)
endforeach()

# Gap scan limited to 30 cm; existing 10 cm data are reused.
foreach(extra
  sweep_reference_20cm.mac
  sweep_block_20cm.mac
  sweep_reference_25cm.mac
  sweep_block_25cm.mac
  sweep_reference_30cm.mac
  sweep_block_30cm.mac
  run_aralik.sh
  aralik_analiz.C
)
  configure_file(${PROJECT_SOURCE_DIR}/${extra} ${PROJECT_BINARY_DIR}/${extra} COPYONLY)
endforeach()

# Wider lead block immediately below upper scintillator; manually started.
foreach(extra ust_reference.mac ust_block.mac run_ust_blok.sh ust_blok_analiz.C)
  configure_file(${PROJECT_SOURCE_DIR}/${extra} ${PROJECT_BINARY_DIR}/${extra} COPYONLY)
endforeach()
configure_file(${PROJECT_SOURCE_DIR}/ust20_reference.mac ${PROJECT_BINARY_DIR}/ust20_reference.mac COPYONLY)
configure_file(${PROJECT_SOURCE_DIR}/ust20_block.mac ${PROJECT_BINARY_DIR}/ust20_block.mac COPYONLY)
configure_file(${PROJECT_SOURCE_DIR}/run_ust20_blok.sh ${PROJECT_BINARY_DIR}/run_ust20_blok.sh COPYONLY)
configure_file(${PROJECT_SOURCE_DIR}/ust20_blok_analiz.C ${PROJECT_BINARY_DIR}/ust20_blok_analiz.C COPYONLY)
