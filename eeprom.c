
#include <pci/pci.h>

#include "eeprom.h"

char *eeprom_get_idrom_type(u8 idrom) {
    switch (idrom) {
        case ID_EEPROM_1M:  return "(size 1Mb)";
        case ID_EEPROM_2M:  return "(size 2Mb)";
        case ID_EEPROM_4M:  return "(size 4Mb)";
        case ID_EEPROM_8M:  return "(size 8Mb)";
        case ID_EEPROM_16M: return "(size 16Mb)";
        default:            return "(unknown size)";
    }
}
