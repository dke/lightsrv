#include <syslog.h>

#include <cmath>

#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include <chrono>
#include <sstream>

#include "config.h"
#ifdef bcm2385_found
#include <bcm2835.h>
#else
#warning bcm2835 lib not found, skipping actual hardware stuff.
#endif

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
    syslog(LOG_DEBUG, "bcm2835_init()");
    #ifdef bcm2385_found
    if (!bcm2835_init()) {
        //syslog(LOG_ERR, "FATAL: bcm2835_init() failed.\n");
        return 1;
    }
    #else
    #endif
    return 0;
}

void BCM2835::close() {
    syslog(LOG_DEBUG, "bcm2835_close()");
    #ifdef bcm2385_found
    bcm2835_close();
    #else
    #endif
}

BCM2835::BCM2835(std::initializer_list<unsigned> c, std::initializer_list<unsigned> p, bool has_automode, bool inverted, bool debug):
    has_automode(has_automode), inverted(inverted), debug(debug), using_auto(has_automode), channels(c), channel_values(c.size(), inverted), pwms(p), pwm_values(p.size(), 50)
{}

void BCM2835::set_debug(bool d) { debug=d; }

void BCM2835::set_inverted(bool d) { inverted=d; }

void BCM2835::set_auto(bool a) { using_auto=a; }

bool BCM2835::get_auto() const { return using_auto; }

// todo: support multiple PWMs is halfway included down there
//#define PWM_PIN RPI_GPIO_P1_12
//#define PWM_CHANNEL 0

void BCM2835::setup() {
    init();
    #ifdef bcm2385_found
    // Set the pins to be output pins
    for(auto channel: channels) {
        syslog(LOG_DEBUG, "bcm2835_gpio_fsel(%d, %d)", channel, BCM2835_GPIO_FSEL_OUTP);
        bcm2835_gpio_fsel(channel, BCM2835_GPIO_FSEL_OUTP);
    }

    //for(auto pwm: pwms) {
    for(unsigned pwm_channel=0; pwm_channel<pwms.size(); pwm_channel++) {
        // Set the pwm pin to Alt Fun 5, to allow PWM channel 0 to be output there
        syslog(LOG_DEBUG, "bcm2835_gpio_fsel(%d, %d)", pwms[pwm_channel], BCM2835_GPIO_FSEL_ALT5);
        bcm2835_gpio_fsel(pwms[pwm_channel], BCM2835_GPIO_FSEL_ALT5);
        syslog(LOG_DEBUG, "bcm2835_pwm_set_clock(%d)", BCM2835_PWM_CLOCK_DIVIDER_16);
        bcm2835_pwm_set_clock(BCM2835_PWM_CLOCK_DIVIDER_16);
        syslog(LOG_DEBUG, "bcm2835_pwm_set_mode(%d, %d, %d)", pwm_channel, 1, 1);
        bcm2835_pwm_set_mode(pwm_channel, 1, 1);
        syslog(LOG_DEBUG, "bcm2835_pwm_set_range(%d, %d)", pwm_channel, 1024);
        bcm2835_pwm_set_range(pwm_channel, 1024);
        // init to pwm 50%
        syslog(LOG_DEBUG, "bcm2835_pwm_set_data(%d, %d)", pwm_channel, 512);
        bcm2835_pwm_set_data(pwm_channel, 512);
    }
    #else
    #endif

    close();
}

int BCM2835::switch_channel(unsigned channel, int value) {
    if(channel>channels.size())
        return -1;
    if(inverted) value = o_trsf(value);
    init();
    #ifdef bcm2385_found
    bcm2835_gpio_write(channels[channel], value);
    #else
    channel_values[channel]=value;
    #endif
    close();
    return 0;
}

int BCM2835::get_channel(unsigned channel) {
    if(channel>channels.size())
        return -1;
    init();
    #ifdef bcm2385_found
    int value = bcm2835_gpio_lev(channels[channel]);
    #else
    int value = channel_values[channel];
    #endif
    close();
    if(inverted) value=i_trsf(value);
    return value;
}

unsigned BCM2835::size() const {
    return channels.size();
}

unsigned BCM2835::get_pwm(unsigned channel) {
    return pwm_values[channel];
}

unsigned BCM2835::set_pwm(unsigned channel, unsigned p) {
    init();
    // TODO more rigid error handling
    if(channel>pwms.size()) {
        return 0;
    }
    //syslog(LOG_DEBUG, "bcm2835_pwm_set_data(%d, %d)", channel, pwm_trsf(p));
    #ifdef bcm2385_found
    bcm2835_pwm_set_data(channel, pwm_trsf(p));
    #else
    #endif
    pwm_values[channel]=p;
    close();
    return p;
}

unsigned BCM2835::pwm_size() const {
    return pwms.size();
}

#include <ctime>


bool BCM2835::has_autom() {
    return has_automode;
}

bool BCM2835::autom() {
    if(!using_auto) {
        syslog(LOG_INFO, "not using_auto: skipping automatic stuff");
        return true;
    }
    #if 1
    time_t rawtime;
    struct tm * timeinfo;
    time ( &rawtime );
    timeinfo = localtime ( &rawtime );
    syslog(LOG_DEBUG, "reference: %s", asctime(timeinfo));
    #endif

    dot dotNow = dotFromNow();

    syslog(LOG_DEBUG, "dotNow=%d==%s", dotNow, formatDot(dotNow).c_str());

    std::vector<interval> on_times_light;
    on_times_light.push_back(interval(dotFromString("12:00:00"), dotFromString("16:00:00")));
    on_times_light.push_back(interval(dotFromString("18:00:00"), dotFromString("22:00:00")));

    std::vector<interval> on_times_co2;
    on_times_co2.push_back(interval(dotFromString("10:00:00"), dotFromString("14:00:00")));
    on_times_co2.push_back(interval(dotFromString("16:00:00"), dotFromString("20:00:00")));

    bool lightsOn=isOn(on_times_light, dotNow);
    bool co2On=isOn(on_times_co2, dotNow);
    double rel=envelope(dotNow)*noon(dotNow);
    unsigned abs=rel*(768-100)+100;

    syslog(LOG_INFO, "time %s: lightsOn: %d, co2On: %d, rel: %f, abs: %d", formatDot(dotNow).c_str(), lightsOn, co2On, rel, abs);
    syslog(LOG_DEBUG, "rel: %f=%f*%f", rel, envelope(dotNow), noon(dotNow));

    set_pwm(0, rel*100);
    // lights
    switch_channel(0, lightsOn);
    switch_channel(1, lightsOn);
    // filter/heating
    switch_channel(2, true);
    // co2
    switch_channel(3, co2On);
    return true;
}


double BCM2835::envelope(unsigned t) const {
    int t0=t-dotFromString("12:00:00");
    unsigned T=dotFromString("22:00:00")-dotFromString("12:00:00");
    double env=std::sin(t0*M_PI/T);
    syslog(LOG_DEBUG, "envelope: t0=%d, T=%d, phi=%f, env=%f", t0, T, t0*M_PI/T, env);
    return env;
}
double BCM2835::noon(unsigned t) const {
    if(t<dotFromString("15:30:00"))
        return 1;
    if(t<dotFromString("16:00:00")) {
        // somehow ramp down to 0
        unsigned t0=t-dotFromString("15:30:00");
        return 1-double(t0)/dotFromString("00:30:00");
    }
    if(t<dotFromString("18:00:00")) {
        return 0;
    }
    if(t<dotFromString("18:30:00")) {
        // somehow ramp up to 1
        unsigned t0=t-dotFromString("18:00:00");
        return double(t0)/dotFromString("00:30:00");
    }
    return 1;
}

unsigned BCM2835::dotFromTm(const tm &t) const {
    return 60*60*t.tm_hour + 60*t.tm_min + t.tm_sec;
}

unsigned BCM2835::dotFromNow() const {
    std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
    time_t tt = std::chrono::system_clock::to_time_t(now);
    tm local_tm = *localtime(&tt);
    return dotFromTm(local_tm);
}
unsigned BCM2835::dotFromString(const std::string &s) const {
    std::vector<std::string> tokens;
    boost::split(tokens, s, boost::is_any_of(":"));
    dot t = std::stoi(tokens[0])*3600+std::stoi(tokens[1])*60+std::stoi(tokens[2]);
    //syslog(LOG_DEBUG, "dotFromTm(t)=" << t << std::endl;
    return t;
}
bool BCM2835::isInInterval(const interval &i, unsigned dot) const {
    //syslog(LOG_DEBUG, "interval: (" << formatDot(i.first) << ", " << formatDot(i.second) << ")" << std::endl;
    bool on=(dot>=i.first && dot<i.second);
    syslog(LOG_DEBUG, "%d isInInterval (%d, %d): %d", dot, i.first, i.second, on);
    return on;
}
bool BCM2835::isOn(const std::vector<interval> &times, unsigned dot) const {
    for(auto &i: times) {
        if(isInInterval(i, dot)) return true;
    }
    return false;
}
std::string BCM2835::formatDot(dot t) const {
    unsigned h=t/3600;
    unsigned m=(t-h*3600)/60;
    unsigned s=t%60;
    std::ostringstream o;
    o << boost::format ("%02d:%02d:%02d") % h % m % s;
    return o.str();
}
