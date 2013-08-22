#include <cstdlib>
#include <cstdio>
#include <string>

#if defined __WIN32__
# include <Windows.h>
#elif defined __unix__
# include <unistd.h>
#endif
#include <SDL/SDL.h>
#include <SDL/SDL_mixer.h>

#define CTRL_PRESSED SDL_GetModState() & KMOD_LCTRL
#define bit bool

#include "io.h"
#include "spritesheet.h"

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
	SpriteSheet ss_sprites("sprites/sprites.bmp");
	SpriteSheet ss_map("sprites/map.bmp");
	SpriteSheet ss_mapeditor("sprites/mapeditor.bmp");
	SpriteSheet ss_gui("sprites/gui.bmp");
	//auto s_entities  = load_texture("sprites/entities.bmp");

	// Start the main loop
	bool done = false;
	while (!done)
	{
		int delta_ticks = SDL_GetTicks() - prev_ticks;
		prev_ticks = SDL_GetTicks();

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

				else if (mapeditor_show) {
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
									else {
										mapeditor_filedialog_show = false;
										ply_x = x_vel = 0;
										ply_y = y_vel = 0;
									}
								} else {									// If it's a load dialog
									if ((i = import_file(mapeditor_filedialog_filename, tiles, &tiles_w, &tiles_h)) < 0)
										msg("Couldn't import: error " + i, "Error");
									else {
										mapeditor_filedialog_show = false;
										ply_x = x_vel = 0;
										ply_y = y_vel = 0;
									}
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
			ss_map.DrawSprite(0, 96, 0, 0, 240, 24, screen);

			for (int j = 0; j < 256; j++) { // horizontal
				for (int i = 0; i < 30; i++) { // vertical
					int tile = tiles[i*256+j];
					ss_mapeditor.DrawSprite(tile, (j * 24) + mapeditor_camerax, (i * 24) + mapeditor_cameray, 24, 24, screen);
				}
			}

			if (!mapeditor_filedialog_show) {
				int x, y, state = SDL_GetMouseState(&x, &y);
				if (state & SDL_BUTTON(SDL_BUTTON_LEFT))
					tiles[(event.button.y-mapeditor_cameray)/24*256 + (event.button.x-mapeditor_camerax)/24] = mapeditor_selectedtile;
				else if (state & SDL_BUTTON(SDL_BUTTON_RIGHT))
					tiles[(event.button.y-mapeditor_cameray)/24*256 + (event.button.x-mapeditor_camerax)/24] = 0;
				ss_mapeditor.DrawSprite(mapeditor_selectedtile, ((x - mapeditor_camerax) / 24 * 24) + mapeditor_camerax, ((y - mapeditor_cameray) / 24 * 24) + mapeditor_cameray, 24, 24, screen);
			} else {
				// Draw the background
				ss_gui.DrawSprite(0,  0, 72,  312, 24, 24, screen);
				ss_gui.DrawSprite(24, 0, 672, 312, 24, 24, screen);
				ss_gui.DrawSprite(48, 0, 72,  408, 24, 24, screen);
				ss_gui.DrawSprite(72, 0, 672, 408, 24, 24, screen);

				for (int i = 96; i <= 648; i += 24)
					for (int j = 312; j <= 408; j += 24)
						ss_gui.DrawSprite(96, 0, i, j, 24, 24, screen);
				for (int i = 336; i <= 384; i += 24) {
					ss_gui.DrawSprite(96, 0, 72,  i, 24, 24, screen);
					ss_gui.DrawSprite(96, 0, 672, i, 24, 24, screen);
				}

				// Draw the button and textbox backgrounds
				int tb_sprite, saveload_sprite, cancel_sprite;
				switch (mapeditor_filedialog_selected) {
					case 0: saveload_sprite = 120; tb_sprite = cancel_sprite = 144; break;
					case 1: cancel_sprite = 120; tb_sprite = saveload_sprite = 144; break;
				}

				for (int i = 96;  i <= 648; i += 24) ss_gui.DrawSprite(tb_sprite, 0, i, 360, 24, 24, screen);
				for (int i = 120; i <= 240; i += 24) ss_gui.DrawSprite(saveload_sprite, 0, i, 408, 24, 24, screen);
				for (int i = 456; i <= 624; i += 24) ss_gui.DrawSprite(cancel_sprite, 0, i, 408, 24, 24, screen);

				// Draw the strings
				ss_gui.DrawString("Enter map name", 216, 312, screen);
				ss_gui.DrawString(mapeditor_filedialog_type ? "Save" : "Load", 144, 408, screen);
				ss_gui.DrawString("Cancel", 480, 408, screen);
				ss_gui.DrawString(mapeditor_filedialog_filename, 120, 360, screen);
			}
		}

		if (!mapeditor_show) {
			// TODO:Draw the background

			// Draw the terrain
			for (int j = 0; j < 256; j++) { // horizontal
				for (int i = 0; i < 30; i++) { // vertical
					int tile = tiles[i*256+j];
					ss_map.DrawSprite(tile, j * 24, i * 24, 24, 24, screen);
				}
			}

			// Draw the player
			float target_x = ply_x + (x_vel * (delta_ticks / 30.0f));
			float target_y = ply_y + (y_vel * (delta_ticks / 30.0f));
			if (target_x/24 > tiles_w) target_x = 0;
			if (target_y/24 > tiles_h) target_y = 0;

			ply_x = target_x;
			ply_y = target_y;

			y_vel += 0.00; // Gravity

			ss_sprites.DrawSprite(0, 0, ply_x * 3, ply_y * 3, 48, 72, screen);

			// TODO:Draw the enemies

			// TODO:Draw the foreground
		}

		// Swap the buffers (and update the screen)
		SDL_Flip(screen);
	}

	return 0;
}

// TODO:Make a game out of this
