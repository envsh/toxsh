include_directories("client" "common" "server" "../libtroh/")
add_executable(dtnat
  main.cpp
  dtnat.cpp
  ../libtroh/debugoutput.cpp
  client/ns_turn_ioaddr.c
  client/ns_turn_msg.c
  client/ns_turn_msg_addr.c
  common/stun_buffer.c
  common/ns_turn_utils.c
  )

qt5_use_modules(dtnat Core Network)
target_link_libraries(dtnat -lcrypto -lpthread)