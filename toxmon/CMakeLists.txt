
include_directories(../libtroh)
add_definitions(-DLOGGING) ## for toxcore
# add_definitions(-DUSE_GEOIP)  ## for vidalia, but not needed now

set(VIDIR "vidalia-0.3.1")
include_directories(
  ${VIDIR}/
  ${VIDIR}/src/vidalia
  ${VIDIR}/src/vidalia/config
  ${VIDIR}/src/common
  ${VIDIR}/src/torcontrol
)
link_directories(${VIDIR}/src/common
  ${VIDIR}/src/torcontrol
  ${VIDIR}/src/vidalia
  ${VIDIR}/src/miniupnpc/
)

include_directories(/usr/include/marble)

## Add the resource files (icons, etc.)
qt5_add_resources(vidalia_SRCS
  ${VIDIR}/src/vidalia/res/vidalia.qrc
  ${VIDIR}/src/vidalia/help/content/content.qrc
#  ${CMAKE_CURRENT_BINARY_DIR}/i18n/vidalia_i18n.qrc
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
  ${vidalia_SRCS}
  # ${VIDIR}/src/vidalia/network/GeoIpRecord.cpp
  # ${VIDIR}/src/vidalia/network/GeoIpDatabase.cpp
  # ${VIDIR}/src/vidalia/network/GeoIpResolver.cpp
  # ${VIDIR}/src/vidalia/network/CountryInfo.cpp
  # ${VIDIR}/src/vidalia/network/ZImageView.cpp
  # ${VIDIR}/src/vidalia/network/TorMapImageView.cpp
)


target_link_libraries(toxmon toxcore GeoIP)
target_link_libraries(toxmon vidalia torcontrol common miniupnpc)
qt5_use_modules(toxmon Core Gui Widgets Network Script Xml)