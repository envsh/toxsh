add_executable(webshc
  main.cpp
#  server.cpp
#  cmdrunner.cpp
#  cmdprovider.cpp
#  cmdresponser.cpp
)
target_link_libraries(webshc -lutil -lreadline)
qt5_use_modules(webshc Core Network)
