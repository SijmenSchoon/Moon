#include <cstdlib>
#include <cstdio>
#include <string>

#if defined __WIN32__
# include <Windows.h>
#endif
#include <SDL/SDL.h>
#include <SDL_mixer.h>

#define CTRL_PRESSED SDL_GetModState() & KMOD_LCTRL
#define bit bool

void error(int error, std::string message) {
#if defined __WIN32__
    MessageBox(NULL, message.c_str(), "Error", MB_ICONERROR);
#elif defined __linux__
    // TODO:Show a graphical error
    printf("Error %d: %s", error, message.c_str());
#endif
    exit(error);
}

char tiles[256*30];
uint16_t tiles_w = 256, tiles_h = 30;
int ply_x, ply_y;
float x_vel, y_vel;

char mapeditor_selectedtile = 0;
bool mapeditor_show = false;
bool mapeditor_drawterrain = true;
bool mapeditor_editmode = 0; // 0 = tiles, 1 = entities
bool mapeditor_filedialog_show = false;
bool mapeditor_filedialog_selected = 0;
bool mapeditor_filedialog_type = 0; // 0 == Load, 1 == Save
int mapeditor_camerax, mapeditor_cameray;
std::string mapeditor_filedialog_filename = "";


void draw_sprite(short srcx, short srcy, short dstx, short dsty, unsigned short w, unsigned short h, SDL_Surface *srcs, SDL_Surface *dsts) {
    SDL_Rect src = { .x = srcx, .y = srcy, .w = w, .h = h };
    SDL_Rect dst = { .x = dstx, .y = dsty };
    SDL_BlitSurface(srcs, &src, dsts, &dst);
}

void draw_string(std::string text, int x, int y, SDL_Surface *txt, SDL_Surface *screen) {
    for (int c : text) {
        draw_sprite((c % 16) * 24, (c / 16) * 24, x, y, 24, 24, txt, screen);
        x += 24;
    }
}

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
    if (fclose(file) != 0) return -4;
}

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
    if (fclose(file) != 0) return -4;
}

int main(int argc, char** argv) {
    if (SDL_Init(SDL_INIT_EVERYTHING) < 0)
        error(1, std::string("Unable to initialize SDL - ") + SDL_GetError());

    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 4096) < 0)
        error(4, std::string("Unable to initialize SDL_mixer - ") + SDL_GetError());

    // Make sure SDL cleans up before exit
    atexit(SDL_Quit);

    // Create a new window
    SDL_Surface* screen = SDL_SetVideoMode(768, 720, 16, SDL_HWSURFACE | SDL_DOUBLEBUF);
    if (!screen)
        error(2, std::string("Unable to set 768x720 video - ") + SDL_GetError());

    // Load the textures
    SDL_Surface *tmp = SDL_LoadBMP("sprites/sprites.bmp");
    if (!tmp) error(3, std::string("Couldn't load texture sprites.bmp"));
    SDL_Surface *s_sprites = SDL_DisplayFormat(tmp); SDL_FreeSurface(tmp);
    SDL_SetColorKey(s_sprites, SDL_SRCCOLORKEY, SDL_MapRGB(s_sprites->format, 0xFF, 0x00, 0xFF));

    tmp = SDL_LoadBMP("sprites/map.bmp");
    if (!tmp) error(3, std::string("Couldn't load texture map.bmp"));
    SDL_Surface *s_map = SDL_DisplayFormat(tmp); SDL_FreeSurface(tmp);
    SDL_SetColorKey(s_map, SDL_SRCCOLORKEY, SDL_MapRGB(s_map->format, 0xFF, 0x00, 0xFF));

    tmp = SDL_LoadBMP("sprites/mapeditor.bmp");
    if (!tmp) error(3, std::string("Couldn't load texture mapeditor.bmp"));
    SDL_Surface *s_mapeditor = SDL_DisplayFormat(tmp); SDL_FreeSurface(tmp);
    SDL_SetColorKey(s_mapeditor, SDL_SRCCOLORKEY, SDL_MapRGB(s_mapeditor->format, 0xFF, 0x00, 0xFF));

    tmp = SDL_LoadBMP("sprites/gui.bmp");
    if (!tmp) error(3, std::string("Couldn't load texture font.bmp"));
    SDL_Surface *s_gui = SDL_DisplayFormat(tmp); SDL_FreeSurface(tmp);
    SDL_SetColorKey(s_gui, SDL_SRCCOLORKEY, SDL_MapRGB(s_gui->format, 0xFF, 0x00, 0xFF));

    tmp = SDL_LoadBMP("sprites/entities.bmp");
    if (!tmp) error(3, std::string("Couldn't load texture entities.bmp"));
    SDL_Surface *s_entities = SDL_DisplayFormat(tmp); SDL_FreeSurface(tmp);
    SDL_SetColorKey(s_entities, SDL_SRCCOLORKEY, SDL_MapRGB(s_entities->format, 0xFF, 0x00, 0xFF));

    Mix_Music *music = Mix_LoadMUS("music.wav");
    if (music) {
        Mix_PlayMusic(music, -1);
    }

    // Start the main loop
    bool done = false;
    while (!done)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_QUIT:
                done = true;
                break;

            case SDL_KEYDOWN:
                if (event.key.keysym.sym >= SDLK_a && event.key.keysym.sym <= SDLK_z) {
                    if (mapeditor_show && mapeditor_filedialog_show) {
                        mapeditor_filedialog_filename += event.key.keysym.sym + ((event.key.keysym.mod & KMOD_SHIFT) ? -32 : 0);
                    }
                }

                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    // TODO:Escape goes to the menu
                } else if (event.key.keysym.sym == SDLK_q) {
                    if (CTRL_PRESSED) done = true; // Control+Q quits the game
                } else if (event.key.keysym.sym == SDLK_e) {
                    if (CTRL_PRESSED) mapeditor_show = !mapeditor_show; // Control+E toggles the map editor
                } else if (event.key.keysym.sym == SDLK_s) {
                    if (mapeditor_show && !mapeditor_filedialog_show) {
                        // If the mapeditor is up and the filedialog isn't, open a save dialog
                        mapeditor_filedialog_type = 1;
                        mapeditor_filedialog_show = true;
                    }
                } else if (event.key.keysym.sym == SDLK_o) {
                    if (mapeditor_show && !mapeditor_filedialog_show) {
                        // If the mapeditor is up and the filedialog isn't, open an open dialog
                        mapeditor_filedialog_type = 0;
                        mapeditor_filedialog_show = true;
                    }
                } else if (event.key.keysym.sym == SDLK_RIGHT | event.key.keysym.sym == SDLK_LEFT) {
                    if (mapeditor_show && mapeditor_filedialog_show) {
                        mapeditor_filedialog_selected = !mapeditor_filedialog_selected;
                    }
                } else if (event.key.keysym.sym == SDLK_BACKSPACE) {
                    if (mapeditor_show && mapeditor_filedialog_show) {
                        if (mapeditor_filedialog_filename.size() > 0)
                            mapeditor_filedialog_filename.pop_back();
                    }
                } else if (event.key.keysym.sym == SDLK_SPACE) {
                    if (mapeditor_show && mapeditor_filedialog_show) {
                        mapeditor_filedialog_filename += ' ';
                    }
                } else if (event.key.keysym.sym == SDLK_RETURN) {
                    if (mapeditor_show && mapeditor_filedialog_show) {
                        if (mapeditor_filedialog_selected == 0) {
                            if (mapeditor_filedialog_type == 1) {
                                int i;
                                if ((i = export_file(mapeditor_filedialog_filename, tiles, tiles_w, tiles_h)) < 0) {
                                    char s[32]; sprintf(s, "Error %d", i);
                                    MessageBox(NULL, s, "Error", MB_ICONERROR);
                                } else {
                                    mapeditor_filedialog_show = false;
                                }
                            } else {
                                int i;
                                if ((i = import_file(mapeditor_filedialog_filename, tiles, &tiles_w, &tiles_h)) < 0) {
                                    char s[32]; sprintf(s, "Error %d", i);
                                    MessageBox(NULL, s, "Error", MB_ICONERROR);
                                } else {
                                    mapeditor_filedialog_show = false;
                                }
                            }
                        } else {
                            mapeditor_filedialog_show = false;
                        }
                    }
                }

                break;

            case SDL_MOUSEBUTTONDOWN:
                if (event.button.button == SDL_BUTTON_WHEELUP) {
                    if (mapeditor_show) {
                        if (--mapeditor_selectedtile == -1) mapeditor_selectedtile++;
                    }
                } else if (event.button.button == SDL_BUTTON_WHEELDOWN) {
                    if (mapeditor_show) {
                        if (++mapeditor_selectedtile == 0) mapeditor_selectedtile--;
                    }
                } else if (event.button.button == SDL_BUTTON_LEFT) {
                    if (mapeditor_show) {

                    }
                }

            case SDL_MOUSEMOTION:
                if (event.motion.state == SDL_BUTTON(SDL_BUTTON_MIDDLE) && mapeditor_show) {
                    if ((mapeditor_camerax + event.motion.xrel) <= 0 && (mapeditor_camerax + event.motion.xrel) > -tiles_w * 24 + 768) mapeditor_camerax += event.motion.xrel;
                    if ((mapeditor_cameray + event.motion.yrel) <= 0 && (mapeditor_cameray + event.motion.yrel) > -tiles_h * 24 + 720) mapeditor_cameray += event.motion.yrel;
                }
            }
        }

        // Clear screen
        SDL_FillRect(screen, 0, SDL_MapRGB(screen->format, 0, 0, 0));

        if (mapeditor_show) {
            draw_sprite(0, 96, 0, 0, 240, 24, s_map, screen);

            for (int j = 0; j < 256; j++) { // horizontal
                for (int i = 0; i < 30; i++) { // vertical
                    int tile = tiles[i*256+j];
                    draw_sprite((tile % 16)*24, (tile / 16)*24, (j * 24) + mapeditor_camerax, (i * 24) + mapeditor_cameray, 24, 24, s_mapeditor, screen);
                }
            }

            if (!mapeditor_filedialog_show) {
                int x, y, state = SDL_GetMouseState(&x, &y);
                if (state & SDL_BUTTON(SDL_BUTTON_LEFT))
                    tiles[(event.button.y-mapeditor_cameray)/24*256 + (event.button.x-mapeditor_camerax)/24] = mapeditor_selectedtile;
                else if (state & SDL_BUTTON(SDL_BUTTON_RIGHT))
                    tiles[(event.button.y-mapeditor_cameray)/24*256 + (event.button.x-mapeditor_camerax)/24] = 0;
                draw_sprite((mapeditor_selectedtile % 16)*24, (mapeditor_selectedtile / 16)*24, ((x-mapeditor_camerax) / 24 * 24) + mapeditor_camerax, ((y-mapeditor_cameray) / 24 * 24) + mapeditor_cameray, 24, 24, s_mapeditor, screen);
            } else {
                // Draw the background
                draw_sprite(0,  0, 72, 312, 24, 24, s_gui, screen);
                draw_sprite(24, 0, 672, 312, 24, 24, s_gui, screen);
                draw_sprite(48, 0, 72, 408, 24, 24, s_gui, screen);
                draw_sprite(72, 0, 672, 408, 24, 24, s_gui, screen);

                for (int i = 96; i <= 648; i += 24)
                    for (int j = 312; j <= 408; j += 24)
                        draw_sprite(96, 0, i, j, 24, 24, s_gui, screen);
                for (int i = 336; i <= 384; i += 24) {
                    draw_sprite(96, 0, 72, i, 24, 24, s_gui, screen);
                    draw_sprite(96, 0, 672, i, 24, 24, s_gui, screen);
                }

                // Draw the button and textbox backgrounds
                int tb_sprite, saveload_sprite, cancel_sprite;
                switch (mapeditor_filedialog_selected) {
                    case 0: saveload_sprite = 120; tb_sprite = cancel_sprite = 144; break;
                    case 1: cancel_sprite = 120; tb_sprite = saveload_sprite = 144; break;
                }

                for (int i = 96;  i <= 648; i += 24) draw_sprite(tb_sprite, 0, i, 360, 24, 24, s_gui, screen);
                for (int i = 120; i <= 240; i += 24) draw_sprite(saveload_sprite, 0, i, 408, 24, 24, s_gui, screen);
                for (int i = 456; i <= 624; i += 24) draw_sprite(cancel_sprite, 0, i, 408, 24, 24, s_gui, screen);

                // Draw the strings
                draw_string("Enter map name", 216, 312, s_gui, screen);
                draw_string(mapeditor_filedialog_type ? "Save" : "Load", 144, 408, s_gui, screen);
                draw_string("Cancel", 480, 408, s_gui, screen);
                draw_string(mapeditor_filedialog_filename, 120, 360, s_gui, screen);
            }
        }

        if (!mapeditor_show) {
            // TODO:Draw the background

            // Draw the terrain
            for (int j = 0; j < 256; j++) { // horizontal
                for (int i = 0; i < 30; i++) { // vertical
                    int tile = tiles[i*256+j];
                    draw_sprite((tile % 16)*24, (tile / 16)*24, j * 24, i * 24, 24, 24, s_map, screen);
                }
            }

            // Draw the player
            int target_x = ply_x + x_vel;
            int target_y = ply_y + y_vel;

            draw_sprite(0, 0, ply_x * 3, ply_y * 3, 48, 72, s_sprites, screen);

            // TODO:Draw the enemies

            // TODO:Draw the foreground
        }

        // Swap the buffers (and update the screen)
        SDL_Flip(screen);
    }

    // Free bitmaps
    SDL_FreeSurface(s_sprites);

    return 0;
}

// TODO:Make a game out of this
