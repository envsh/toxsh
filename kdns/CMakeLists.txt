include_directories("../libtroh")
add_executable(kdns
  main.cpp
  kdns.cpp
  ../libtroh/debugoutput.cpp
)

qt5_use_modules(kdns Core Network)
target_link_libraries(kdns cares ldns)