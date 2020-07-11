// For communication protocol that should be standarized between both server and client
#ifndef GARAGE_UDP
    #define GARAGE_UDP;
    #define UDP_PORT 4204
    #define MAX_UDP_SIZE 255

    #define UDP_HEADER "ESP8266_GARAGE_"

    #define SEARCH_MESSAGE UDP_HEADER "SEARCH"
    #define LINK_MESSAGE UDP_HEADER "LINK"
    #define LINKED_MESSAGE UDP_HEADER "LINKED"
    #define ACTION_MESSAGE UDP_HEADER "ACTION"
#endif

#ifndef DEVICE_TYPES
    #define DEVICE_TYPES;
    #define DEVICE_NONE 0
    #define DEVICE_ANY 99
    #define DEVICE_GARAGE 1
    #define DEVICE_THERMOMETER 2
    #define DEVICE_RELAY 3
    #define DEVICE_IRSENSOR 4
    #define DEVICE_WATER 5
#endif