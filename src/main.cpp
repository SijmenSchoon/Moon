#include <cstdlib>
#include <cstdio>
#include <string>

#if defined __WIN32__
# include <Windows.h>
#endif
#include <SDL/SDL.h>
#include <SDL/SDL_mixer.h>

#define CTRL_PRESSED SDL_GetModState() & KMOD_LCTRL
#define bit bool

#include "io.h"

void msg(std::string message, std::string title) {
#if defined __WIN32__
	MessageBox(NULL, message.c_str(), title.c_str());
#elif defined __linux__
	// TODO Show a graphical error
	printf("%s: %s", title.c_str(), message.c_str());
#endif
}

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
float ply_x, ply_y;
float x_vel, y_vel;

int prev_ticks;

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

int main(int argc, char** argv) {
	if (SDL_Init(SDL_INIT_EVERYTHING) < 0)
		error(1, std::string("Unable to initialize SDL - ") + SDL_GetError());

	if (Mix_OpenAudio(48000, MIX_DEFAULT_FORMAT, 2, 4096) < 0)
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
		int delta_ticks = SDL_GetTicks() - prev_ticks;
		prev_ticks = SDL_GetTicks();
		printf("Delta ticks: %d\n", delta_ticks);

		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
			case SDL_QUIT:
				done = true;
				break;

			case SDL_KEYDOWN:
				if (CTRL_PRESSED) {
					switch (event.key.keysym.sym) {
					case SDLK_q:
						done = true;	// Ctrl+Q quits the game
						break;

					case SDLK_e:
						mapeditor_show = !mapeditor_show;	// Ctrl+E toggles the map editor
						break;
					}
				}

				if (mapeditor_show) {
					if (mapeditor_filedialog_show) {
						// If both mapeditor and filedialog are up
						switch (event.key.keysym.sym) {
						case SDLK_RIGHT:
						case SDLK_LEFT:
							mapeditor_filedialog_selected = !mapeditor_filedialog_selected;
							break;

						case SDLK_BACKSPACE:
							// Remove the last character from the filename
							if (mapeditor_filedialog_filename.size() > 0)
								mapeditor_filedialog_filename.pop_back();
							break;

						case SDLK_SPACE:
							// Add a space to the filename
							mapeditor_filedialog_filename += ' ';
							break;

						case SDLK_RETURN:
							if (mapeditor_filedialog_selected == 0) {	// If the save/load button is selected
								int i;
								if (mapeditor_filedialog_type == 1) {		// If it's a save dialog
									if ((i = export_file(mapeditor_filedialog_filename, tiles, tiles_w, tiles_h)) < 0)
										msg("Couldn't export: Error " + i, "Error");
									else
										mapeditor_filedialog_show = false;
								} else {									// If it's a load dialog
									if ((i = import_file(mapeditor_filedialog_filename, tiles, &tiles_w, &tiles_h)) < 0)
										msg("Couldn't import: error " + i, "Error");
									else
										mapeditor_filedialog_show = false;
								}
							} else {									// If the cancel button is selected
								mapeditor_filedialog_show = false;			// Just close the dialog
							}
						}
						
						if ((event.key.keysym.sym >= SDLK_a) && (event.key.keysym.sym <= SDLK_z))
							mapeditor_filedialog_filename += event.key.keysym.sym + ((event.key.keysym.mod & KMOD_SHIFT) ? -32 : 0);
					} else {
						// If mapeditor is up and filedialog isn't
						switch (event.key.keysym.sym) {
						case SDLK_s:
							mapeditor_filedialog_type = 1;
							mapeditor_filedialog_show = true;
							break;

						case SDLK_o:
							mapeditor_filedialog_type = 0;
							mapeditor_filedialog_show = true;
							break;
						}
					}
				} else {	// if (!mapeditor_show)
					switch (event.key.keysym.sym) {
					case SDLK_d:
						x_vel += 1;
						break;

					case SDLK_a:
						x_vel -= 1;
						break;
					}
				}

				break;

			case SDL_KEYUP:
				if (mapeditor_show) {

				} else {	// if (!mapeditor_show)
					switch (event.key.keysym.sym) {
					case SDLK_d:
						x_vel -= 1;
						break;

					case SDLK_a:
						x_vel += 1;
						break;
					}
				}

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
			float target_x = ply_x + (x_vel * (delta_ticks / 30.0f));
			float target_y = ply_y + (y_vel * (delta_ticks / 30.0f));

			// TODO:Collision detection

			ply_x = target_x;
			ply_y = target_y;

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
