add_executable(trohc
  main.cpp
  server.cpp
  cmdrunner.cpp
  cmdprovider.cpp
  cmdresponser.cpp
  virttcpc.cpp
)
target_link_libraries(trohc -lutil)
qt5_use_modules(trohc Core Network)
