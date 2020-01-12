//#include <syslog.h>
#include <bcm2835.h>

#include "BCM2835.h"

int BCM2835::o_trsf(int arg) {
    return !arg;
}

int BCM2835::i_trsf(int arg) {
    return !arg;
}

int BCM2835::pwm_trsf(int arg) {
    return arg*1024/100.0;
}

int BCM2835::init() {
    //syslog(LOG_DEBUG, "bcm2835_init()\n");
    if (!bcm2835_init()) {
        //syslog(LOG_ERR, "FATAL: bcm2835_init() failed.\n");
        return 1;
    }
    return 0;
}

void BCM2835::close() {
    //syslog(LOG_DEBUG, "bcm2835_close()\n");
    bcm2835_close();
}

BCM2835::BCM2835(std::initializer_list<unsigned> c, std::initializer_list<unsigned> p): debug(false), inverted(false), channels(c), pwms(p), pwm_values(p.size(), 512) {}

void BCM2835::set_debug(bool d) { debug=d; }
void BCM2835::set_inverted(bool d) { inverted=d; }

// todo: support multiple PWMs is halfway included down there
#define PWM_PIN RPI_GPIO_P1_12
#define PWM_CHANNEL 0

void BCM2835::setup() {
    init();
    // Set the pins to be output pins
    //for(unsigned channel=0; channel<channels.size(); channel++) {
    for(auto channel: channels) {
        //syslog(LOG_DEBUG, "bcm2835_gpio_fsel(%d, %d)", channels[channel], BCM2835_GPIO_FSEL_OUTP);
        bcm2835_gpio_fsel(channels[channel], BCM2835_GPIO_FSEL_OUTP);
    }

    for(auto pwm: pwms) {
        // Set the pwm pin to Alt Fun 5, to allow PWM channel 0 to be output there
        bcm2835_gpio_fsel(PWM_PIN, BCM2835_GPIO_FSEL_ALT5);
        bcm2835_pwm_set_clock(BCM2835_PWM_CLOCK_DIVIDER_16);
        bcm2835_pwm_set_mode(PWM_CHANNEL, 1, 1);
        bcm2835_pwm_set_range(PWM_CHANNEL, 1024);
        // init to pwm 50%
        bcm2835_pwm_set_data(PWM_CHANNEL, 512);
    }

    close();
}

int BCM2835::switch_channel(unsigned channel, int value) {
    if(channel>channels.size())
        return -1;
    if(inverted) value = o_trsf(value);
    init();
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
    if(inverted) value=i_trsf(value);
    return value;
}

unsigned BCM2835::size() const {
    return channels.size();
}

unsigned BCM2835::get_pwm(int channel) {
    return pwm_values[channel];
}

unsigned BCM2835::set_pwm(int channel, unsigned p) {
    init();
    //syslog(LOG_DEBUG, "bcm2835_pwm_set_data(%d, %d)", PWM_CHANNEL, pwm_trsf(p));
    bcm2835_pwm_set_data(PWM_CHANNEL, pwm_trsf(p));
    pwm_values[channel]=p;
    close();
    return p;
}

unsigned BCM2835::pwm_size() const {
    return pwms.size();
}
