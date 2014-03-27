add_executable(webshd
  main.cpp
  server.cpp
  cmdrunner.cpp
  cmdprovider.cpp
  cmdresponser.cpp
)
target_link_libraries(webshd -lutil)
qt5_use_modules(webshd Core Network)
