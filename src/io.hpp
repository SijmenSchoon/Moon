#pragma once

#include <string>
#include <cstdint>

/* int export_file(std::string filename, char *map, uint16_t w, uint16_t h)
 **************************************************************************
 * Exports the map data to file.
 **************************************************************************
 * Parameters:
 *  * in std::string filename      The filename to export to.
 *  * in char *map                 The map data to export.
 *  * in uint16_t w                16-bit short with the width of the map.
 *  * in uint16_t h                16-bit short with the height of the map.
 **************************************************************************
 * Returns: A success value:
 *  *  1     No errors occured
 *  * -1     Couldn't open file
 *  * -2     Couldn't write width/height to file
 *  * -3     Couldn't write map data to file
 *  * -4     Couldn't close file
 **************************************************************************/
int export_file(std::string name, char *map, uint16_t w, uint16_t h);

/* int import_file(std::string filename, char *map, uint16_t *w, uint16_t *h)
 ****************************************************************************
 * Exports the map data to file.
 ****************************************************************************
 * Parameters:
 *  * in std::string filename      The filename to import from.
 *  * out char *map                The map data to import into.
 *  * out uint16_t w               16-bit short for the width of the map.
 *  * out uint16_t h               16-bit short for the height of the map.
 ****************************************************************************
 * Returns: A success value:
 *  *  1     No errors occured
 *  * -1     Couldn't open file
 *  * -2     Couldn't read width/height from file
 *  * -3     Couldn't read map data from file
 *  * -4     Couldn't close file
 ****************************************************************************/
int import_file(std::string name, char *map, uint16_t *w, uint16_t *h);
