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
    std::vector<unsigned> pwms;
    std::vector<unsigned> pwm_values;
    int o_trsf(int arg);
    int i_trsf(int arg);
    int init();
    void close();
public:
    BCM2835(std::initializer_list<unsigned> c, std::initializer_list<unsigned> p);
    void set_debug(bool d);
    void setup();
    int switch_channel(unsigned channel, int value);
    int get_channel(unsigned channel);
    unsigned size() const;
    unsigned get_pwm(int channel);
    unsigned set_pwm(int channel, unsigned p);
    unsigned pwm_size() const;
};

#endif












