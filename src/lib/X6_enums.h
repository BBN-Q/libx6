#ifndef X6_enums_H_
#define X6_enums_H_

enum ClockSource {
    EXTERNAL_CLOCK = 0,   /**< External Input */
    INTERNAL_CLOCK        /**< Internal Generation */
};

enum X6_REFERENCE_SOURCE {
    EXTERNAL_REFERENCE = 0,   /**< External Input */
    INTERNAL_REFERENCE        /**< Internal Generation */
};

enum ExtSource {
    FRONT_PANEL = 0, /**< Front panel input */
    P16              /**< P16 input */
};

enum X6_TRIGGER_SOURCE {
    SOFTWARE_TRIGGER = 0,    /**< Software generated trigger */
    EXTERNAL_TRIGGER         /**< External trigger */
};

enum X6_DIGITIZER_MODE {
    DIGITIZER,
    AVERAGER
};

struct ChannelTuple {
    int a;
    int b;
    int c;
};

#endif
