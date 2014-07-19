include_directories("../dtnat/client" "../dtnat/common" "../dtnat/server" "../dtnat" "../libtroh")
add_definitions(-DSRUDP_STUN)

add_executable(p2pshc
  cmain.cpp
  xshcli.cpp
  peersrv.cpp
  stunclient.cpp
  srudp.cpp
  ../libtroh/debugoutput.cpp
  ../dtnat/client/ns_turn_ioaddr.c
  ../dtnat/client/ns_turn_msg.c
  ../dtnat/client/ns_turn_msg_addr.c
  ../dtnat/common/stun_buffer.c
  ../dtnat/common/ns_turn_utils.c
  )

add_executable(p2pshd
  smain.cpp
  xshsrv.cpp
  peersrv.cpp
  stunclient.cpp
  srudp.cpp
  ../libtroh/debugoutput.cpp
  ../dtnat/client/ns_turn_ioaddr.c
  ../dtnat/client/ns_turn_msg.c
  ../dtnat/client/ns_turn_msg_addr.c
  ../dtnat/common/stun_buffer.c
  ../dtnat/common/ns_turn_utils.c
  )

qt5_use_modules(p2pshc Core Network)
target_link_libraries(p2pshc -lcrypto -lpthread)
qt5_use_modules(p2pshd Core Network)
target_link_libraries(p2pshd -lcrypto -lpthread)

add_executable(regsrv
  regsrv.cpp
  peersrv.cpp
  stunclient.cpp
  ../libtroh/debugoutput.cpp
  ../dtnat/client/ns_turn_ioaddr.c
  ../dtnat/client/ns_turn_msg.c
  ../dtnat/client/ns_turn_msg_addr.c
  ../dtnat/common/stun_buffer.c
  ../dtnat/common/ns_turn_utils.c
)
qt5_use_modules(regsrv Core Network)
target_link_libraries(regsrv -lcrypto -lpthread)
