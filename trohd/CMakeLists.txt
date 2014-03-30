include_directories("../libtroh")
add_executable(trohd
  main.cpp
  server.cpp
  virttcpd.cpp
  ../libtroh/cmdprovider.cpp
  ../libtroh/cmdsender.cpp
)
target_link_libraries(trohd -lutil -lreadline)
qt5_use_modules(trohd Core Network)
