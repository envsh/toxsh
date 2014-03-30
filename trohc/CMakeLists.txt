include_directories("../libtroh")

add_executable(trohc
  main.cpp
  server.cpp
  cmdrunner.cpp
  cmdprovider.cpp
  virttcpc.cpp
  ../libtroh/cmdsender.cpp
)
target_link_libraries(trohc -lutil)
qt5_use_modules(trohc Core Network)
