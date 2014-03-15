#include <Messenger.h>
#include <friend_requests.h>
#include <group_chats.h>

#include "xtox.h"


Tox *xtox_new(uint8_t ipv6enabled, uint8_t *pubkey)
{
    /*
    Messenger *m = (Messenger*)calloc(1, sizeof(Messenger));

    if ( ! m )
        return NULL;

    IP ip;
    ip_init(&ip, ipv6enabled);
    m->net = new_networking(ip, TOX_PORT_DEFAULT);

    if (m->net == NULL) {
        free(m);
        return NULL;
    }

    m->net_crypto = new_net_crypto(m->net);

    if (m->net_crypto == NULL) {
        kill_networking(m->net);
        free(m);
        return NULL;
    }

    m->dht = new_DHT(m->net_crypto);

    if (m->dht == NULL) {
        kill_net_crypto(m->net_crypto);
        kill_networking(m->net);
        free(m);
        return NULL;
    }

    m->onion = new_onion(m->dht);
    m->onion_a = new_onion_announce(m->dht);
    m->onion_c =  new_onion_client(m->dht);

    if (!(m->onion && m->onion_a && m->onion_c)) {
        kill_onion(m->onion);
        kill_onion_announce(m->onion_a);
        kill_onion_client(m->onion_c);
        kill_DHT(m->dht);
        kill_net_crypto(m->net_crypto);
        kill_networking(m->net);
        free(m);
        return NULL;
    }

    m_set_statusmessage(m, (uint8_t *)"Online", sizeof("Online"));

    friendreq_init(&(m->fr), m->onion_c);
    LANdiscovery_init(m->dht);
    set_nospam(&(m->fr), random_int());
    set_filter_function(&(m->fr), &friend_already_added, m);

    networking_registerhandler(m->net, NET_PACKET_GROUP_CHATS, &handle_group, m);

    return (Tox*)m;
    */
    return NULL;
}


