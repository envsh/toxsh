
include_directories(../libtroh ../common)
add_definitions(-DSRUDP_TOX) # SRUDP_TOX SRUDP_STUN SRUDP_HTTP

add_executable(tunnel
  main.cpp
  ../libtroh/debugoutput.cpp
  tunneld.cpp
  toxnet.cpp
  tunnelc.cpp
  socksd.cpp
  srudp.cpp

  ../common/xtox.cpp
  ../common/core.cpp
  ../common/status.cpp
  ../common/Settings/settings.cpp
)

qt5_use_modules(tunnel Core Network)

