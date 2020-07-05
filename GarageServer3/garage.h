// For communication protocol that should be standarized between both server and client 
#ifndef GARAGE_UDP
    #define GARAGE_UDP;
    #define UDP_PORT 4204
    #define MAX_UDP_SIZE 255
#endif

#ifndef DEVICE_TYPES
    #define DEVICE_TYPES;
    #define DEVICE_NONE 0
    #define DEVICE_ANY 99
    #define DEVICE_GARAGE 1
    #define DEVICE_THERMOMETOR 2
    #define DEVICE_RELAY 3
    #define DEVICE_IRSENSOR 4
    #define DEVICE_WATER 5
#endif