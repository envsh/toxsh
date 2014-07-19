
include_directories(../libtroh ../common)
add_definitions(-DSRUDP_TOX) # SRUDP_TOX SRUDP_STUN SRUDP_HTTP

add_executable(tunnel
  main.cpp
  ../libtroh/debugoutput.cpp
  tunneld.cpp
  tunnelc.cpp
  socksd.cpp
  srudp.cpp
)

qt5_use_modules(tunnel Core Network)

