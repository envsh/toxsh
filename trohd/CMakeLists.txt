add_executable(trohd
  main.cpp
  cmdsender.cpp
  server.cpp
  virttcpd.cpp
#  cmdrunner.cpp
#  cmdprovider.cpp
#  cmdresponser.cpp
)
target_link_libraries(trohd -lutil -lreadline)
qt5_use_modules(trohd Core Network)
