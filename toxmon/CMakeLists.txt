
include_directories(../libtroh)
add_definitions(-DLOGGING)
add_definitions(-DUSE_GEOIP)

set(VIDIR "vidalia-0.3.1")
include_directories(${VIDIR}/src/vidalia
  ${VIDIR}/src/common
  ${VIDIR}/src/torcontrol
)
link_directories(${VIDIR}/src/common
  ${VIDIR}/src/torcontrol
)

qt5_wrap_ui(UIS_toxmon
  dhtmon.ui

  ###

)

add_executable(toxmon
  ../libtroh/debugoutput.cpp
  main.cpp
  dhtmon.cpp
  dhtproc.cpp
  dummy.cpp
  ${UIS_toxmon}

  ###
  ${VIDIR}/src/vidalia/network/GeoIpRecord.cpp
  ${VIDIR}/src/vidalia/network/GeoIpDatabase.cpp
  ${VIDIR}/src/vidalia/network/GeoIpResolver.cpp
  ${VIDIR}/src/vidalia/network/CountryInfo.cpp
  ${VIDIR}/src/vidalia/network/ZImageView.cpp
#  ${VIDIR}/src/vidalia/network/TorMapImageView.cpp
)


target_link_libraries(toxmon toxcore GeoIP)
target_link_libraries(toxmon common torcontrol)
qt5_use_modules(toxmon Core Gui Widgets Network)