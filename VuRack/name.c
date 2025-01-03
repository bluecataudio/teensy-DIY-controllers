// To give your project a unique name, this code must be
// placed into a .c file (its own tab).  It can not be in
// a .cpp file or your main sketch (the .ino file).

#include "usb_names.h"

// Edit these lines to create your own name.  The length must
// match the number of characters in your custom name.

#define MIDI_NAME   {'B','C','A',' ','V','U','M'}
#define MIDI_NAME_LEN  7
#define MIDI_MANUF_NAME   {'B','L','U','E',' ','C','A','T',' ','A','U','D','I','O'}
#define MIDI_MANUF_NAME_LEN  14

// Do not change this part.  This exact format is required by USB.

struct usb_string_descriptor_struct usb_string_product_name = {
        2 + MIDI_NAME_LEN * 2,
        3,
        MIDI_NAME
};


struct usb_string_descriptor_struct usb_string_manufacturer_name = {
        2 + MIDI_MANUF_NAME_LEN * 2,
        3,
        MIDI_MANUF_NAME
};
