include_directories("../libtroh")

add_executable(trohc
  main.cpp
  server.cpp
  ../libtroh/cmdprovider.cpp
  virttcpc.cpp
  ../libtroh/cmdsender.cpp
  ../libtroh/debugoutput.cpp
)
target_link_libraries(trohc -lutil)
qt5_use_modules(trohc Core Network)
