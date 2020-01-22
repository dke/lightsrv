#ifndef LIGHTSRV_BCM2835_H
#define LIGHTSRV_BCM2835_H

#include <string>
#include <vector>

// use:
//     BCM backend { 17, 27 };
//     backend.setup();

class BCM2835 {
    bool inverted;
    bool debug;
    bool using_auto;
    std::vector<unsigned> channels;
    // "cache" of the actual values we only use for reading if
    // compiled without bcm2385_found, like mockup backend
    std::vector<unsigned> channel_values;
    std::vector<unsigned> pwms;
    // this is actually also used if we have the real backend because the
    // real backend does not offer reading pwm values
    std::vector<unsigned> pwm_values;
    int o_trsf(int arg);
    int i_trsf(int arg);
    int pwm_trsf(int arg);
    int init();
    void close();
public:
    typedef unsigned dot;
    typedef std::pair<dot, dot> interval;
    BCM2835(std::initializer_list<unsigned> c, std::initializer_list<unsigned> p, bool inverted=false, bool debug=false);
    void set_debug(bool d);
    void set_inverted(bool d);
    void set_auto(bool a);
    bool get_auto() const;
    void setup();
    int switch_channel(unsigned channel, int value);
    int get_channel(unsigned channel);
    unsigned size() const;
    unsigned get_pwm(unsigned channel);
    unsigned set_pwm(unsigned channel, unsigned p);
    unsigned pwm_size() const;
    bool has_autom();
    bool autom();
private:
    double envelope(unsigned t) const;
    double noon(unsigned t) const;
    unsigned dotFromTm(const tm &t) const ;
    unsigned dotFromNow() const;
    unsigned dotFromString(const std::string &s) const;
    bool isInInterval(const interval &i, unsigned dot) const;
    bool isOn(const std::vector<interval> &times, unsigned dot) const;
    std::string formatDot(dot t) const;
};

#endif












