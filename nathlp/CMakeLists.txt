set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS}")  #-std=c++11 -stdlib=libc++ -lc++abi

add_executable(nathlp
  main.cpp
)

target_link_libraries(nathlp -levent)


add_executable(rlysrv
  rlysrv.cpp
)

target_link_libraries(rlysrv -levent)
