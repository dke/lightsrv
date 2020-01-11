#ifndef LIGHTSRV_BCM2835_H
#define LIGHTSRV_BCM2835_H

#include <vector>
#include <bcm2835.h>

// use:
//     BCM backend { 17, 27 };
//     backend.setup();

class BCM2835 {
    bool debug;
    std::vector<unsigned> channels;
    int o_trsf(int arg);
    int i_trsf(int arg);
    int init();
    void close();
public:
    BCM2835(std::initializer_list<unsigned> c);
    void set_debug(bool d);
    void setup();
    int switch_channel(unsigned channel, int value);
    int get_channel(unsigned channel);
    unsigned size() const;
};

#endif












