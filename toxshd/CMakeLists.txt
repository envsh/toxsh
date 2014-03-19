add_executable(toxshd
  main.cpp
  server.cpp
  commandhandler.cpp
  ../common/xtox.cpp
  ../common/core.cpp
  ../common/status.cpp
  ../common/Settings/settings.cpp
)

qt5_use_modules(toxshd Core)


