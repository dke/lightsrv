#include <bcm2835.h>

#include "BCM2835.h"

int BCM2835::o_trsf(int arg) {
    return !arg;
}

int BCM2835::i_trsf(int arg) {
    return !arg;
}

int BCM2835::init() {
    // if(debug) syslog(LOG_DEBUG, "bcm2835_init()\n");
    if (!bcm2835_init()) {
        //syslog(LOG_ERR, "FATAL: bcm2835_init() failed.\n");
        return 1;
    }
    return 0;
}

void BCM2835::close() {
    // if(debug) syslog(LOG_DEBUG, "bcm2835_close()\n");
    bcm2835_close();
}

BCM2835::BCM2835(std::initializer_list<unsigned> c): channels(c) {}

void BCM2835::set_debug(bool d) { debug=d; }

void BCM2835::setup() {
    init();
    // Set the pins to be output pins
    //for(unsigned channel=0; channel<channels.size(); channel++) {
    for(auto channel: channels) {
        bcm2835_gpio_fsel(channels[channel], BCM2835_GPIO_FSEL_OUTP);
    }
    close();
}

int BCM2835::switch_channel(unsigned channel, int value) {
    if(channel>channels.size())
        return -1;
    init();
    value = o_trsf(value);
    bcm2835_gpio_write(channels[channel], value);
    close();
    return 0;
}

int BCM2835::get_channel(unsigned channel) {
    if(channel>channels.size())
        return -1;
    init();
    int value = bcm2835_gpio_lev(channels[channel]);
    close();
    return i_trsf(value);
}

unsigned BCM2835::size() const {
    return channels.size();
}