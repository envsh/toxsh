
include_directories(../libtroh)
add_definitions(-DLOGGING)

qt5_wrap_ui(UIS_toxmon
  dhtmon.ui
)

add_executable(toxmon
  ../libtroh/debugoutput.cpp
  main.cpp
  dhtmon.cpp
  dhtproc.cpp
  ${UIS_toxmon}
)

target_link_libraries(toxmon toxcore)
qt5_use_modules(toxmon Core Network Gui Widgets)

