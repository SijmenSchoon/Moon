#include "io.hpp"

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
 *  * -15    Couldn't close file
 **************************************************************************/
int export_file(std::string name, char *map, uint16_t w, uint16_t h) { 
    if (name.find('.') == std::string::npos) name.append(".map");
    name = "maps/" + name;
    FILE *file = fopen(name.c_str(), "wb");
    if (!file) return -1;

    // Write the dimensions
    if (!fwrite(&w, sizeof(uint16_t), 1, file)) return -2;
    if (!fwrite(&h, sizeof(uint16_t), 1, file)) return -2;

    // Write the map data
    if (!fwrite(map, 1, w*h, file)) return -3;

    // Close the file
    if (fclose(file) != 0) return -15;

	return 1;
}

/* int import_file(std::string filename, char *map, uint16_t *w, uint16_t *h)
 ****************************************************************************
 * Exports the map data to file.
 ****************************************************************************
 * Parameters:
 *  * in std::string filename      The filename to import from.
 *  * out char *map                The map data to import into.
 *  * out uint16_t *w              16-bit short for the width of the map.
 *  * out uint16_t *h              16-bit short for the height of the map.
 ****************************************************************************
 * Returns: A success value:
 *  *  1     No errors occured
 *  * -1     Couldn't open file
 *  * -2     Couldn't read width/height from file
 *  * -3     Couldn't read map data from file
 *  * -15    Couldn't close file
 ****************************************************************************/
int import_file(std::string name, char *map, uint16_t *w, uint16_t *h) {
    if (name.find('.') == std::string::npos) name.append(".map");
    name = "maps/" + name;
    FILE *file = fopen(name.c_str(), "rb");
    if (!file) return -1;

    // Read the dimensions
    if (!fread(w, sizeof(uint16_t), 1, file)) return -2;
    if (!fread(h, sizeof(uint16_t), 1, file)) return -2;

    // Read the map data
    uint16_t x = *w, y = *h;
    if (!fread(map, 1, x*y, file)) return -3;

    // Close the file
    if (fclose(file) != 0) return -15;

	return 1;
}
