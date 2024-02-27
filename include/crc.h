#ifndef CRC_H
#define CRC_H

#include <EEPROM.h>

void openMemory();
void saveStatus();
boolean loadStatus();
void saveConfig();
boolean loadConfig();
void copyConfig(configuration* from,configuration* to);
void defaultConfig(configuration* c);

#endif
