#ifndef X6_enums_H_
#define X6_enums_H_

enum DigitizerMode {DIGITIZE=0, AVERGAGE, FILTER, FILTER_AND_AVERAGE} ;

enum ClockSource {
    EXTERNAL_CLOCK = 0,   /**< External Input */
    INTERNAL_CLOCK        /**< Internal Generation */
};

enum ReferenceSource {
    EXTERNAL_REFERENCE = 0,   /**< External Input */
    INTERNAL_REFERENCE        /**< Internal Generation */
};

enum ExtSource {
    FRONT_PANEL = 0, /**< Front panel input */
    P16              /**< P16 input */
};

enum TriggerSource {
    SOFTWARE_TRIGGER = 0,    /**< Software generated trigger */
    EXTERNAL_TRIGGER         /**< External trigger */
};

struct ChannelTuple {
    int a;
    int b;
    int c;
};

#endif
