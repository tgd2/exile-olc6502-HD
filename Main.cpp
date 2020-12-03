#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"

#include "Exile.h"
#include "newSN76489.h"

#include <string>
#include <queue>
#include <vector>

// O------------------------------------------------------------------------------O
// | Screen constants and global variables                                        |
// O------------------------------------------------------------------------------O
float fTimeSinceLastFrame = 0;
uint16_t nFrameCounter = 0;

const float SCREEN_WIDTH = 1920;   const float SCREEN_HEIGHT = 1080; // 1080p display
const float SCREEN_ZOOM_MAX = 1.0f;
const float SCREEN_ZOOM_MIN = 5.0f; 
const float SCREEN_BORDER_SCALE = 0.3f;

float fZoom = 2.0f;

float fCanvasX = 4511;
float fCanvasY = 1650;
float fCanvasX_ScrollShift = 0;
float fCanvasY_ScrollShift = 0;

float fCanvasWidth = SCREEN_WIDTH / fZoom;
float fCanvasHeight = SCREEN_HEIGHT / fZoom;

float fCanvasOffsetX = (SCREEN_WIDTH - fCanvasWidth) / 2.0f;
float fCanvasOffsetY = (SCREEN_HEIGHT - fCanvasHeight) / 2.0f;

std::unique_ptr<olc::Sprite> sprWater[2];
std::unique_ptr<olc::Decal> decWater[2];
std::unique_ptr<olc::Sprite> sprWaterSquare[2];
std::unique_ptr<olc::Decal> decWaterSquare[2];

bool bScreenFlash = false;
uint8_t nEarthQuakeOffset = 0;

class Exile_olc6502 : public olc::PixelGameEngine
{

public:
	Exile_olc6502() { sAppName = "Exile"; }

	Exile Game;

	float ScreenCoordinateX(float GameCoordinateX) {
		float x = GameCoordinateX - fCanvasX - fCanvasOffsetX - (fCanvasX_ScrollShift * fTimeSinceLastFrame / 0.025f);
		x += nEarthQuakeOffset * 4; // Shift screen in event of earthquake
		x = x * fZoom;
		return x;
	}

	float ScreenCoordinateY(float GameCoordinateY) {
		float y = GameCoordinateY - fCanvasY - fCanvasOffsetY - (fCanvasY_ScrollShift * fTimeSinceLastFrame / 0.025f);
		y = y * fZoom;
		return y;
	}

	bool OnUserCreate()
	{
		// Load Exile RAM from disassembly, and patch adjustments to run in HD:
		Game.LoadExileFromDisassembly("exile-disassembly.txt"); // www.level7.org.uk/miscellany/exile-disassembly.txt
		Game.PatchExileRAM();

		//Acquire all keys and equipment - optional! :)
		for (int i = 0; i < (0x0818 - 0x0806); i++) Game.BBC.ram[0x0806 + i] = 0xff;

		Game.Reset();

		// Set stack pointer and PC:
		Game.BBC.cpu.stkp = 0xff;  Game.BBC.cpu.pc = 0x19b6;

		// Setup water sprite:
		sprWater[0] = std::make_unique<olc::Sprite>(GAME_TILE_WIDTH, SCREEN_HEIGHT + GAME_TILE_HEIGHT);
		olc::PixelGameEngine::SetDrawTarget(sprWater[0].get());
		olc::PixelGameEngine::Clear(olc::BLUE);
		olc::PixelGameEngine::DrawLine(0, 0, GAME_TILE_WIDTH + 1, 0, olc::CYAN);
		decWater[0] = std::make_unique<olc::Decal>(sprWater[0].get());

		sprWater[1] = std::make_unique<olc::Sprite>(GAME_TILE_WIDTH, SCREEN_HEIGHT + GAME_TILE_HEIGHT);
		olc::PixelGameEngine::SetDrawTarget(sprWater[1].get());
		olc::PixelGameEngine::Clear(olc::Pixel(0, 0, 0xff, 0x40)); // slightly transparent blue
		olc::PixelGameEngine::DrawLine(0, 0, GAME_TILE_WIDTH + 1, 0, olc::CYAN);
		decWater[1] = std::make_unique<olc::Decal>(sprWater[1].get());

		sprWaterSquare[0] = std::make_unique<olc::Sprite>(GAME_TILE_WIDTH, GAME_TILE_HEIGHT);
		olc::PixelGameEngine::SetDrawTarget(sprWaterSquare[0].get());
		olc::PixelGameEngine::Clear(olc::BLUE);
		decWaterSquare[0] = std::make_unique<olc::Decal>(sprWaterSquare[0].get());

		sprWaterSquare[1] = std::make_unique<olc::Sprite>(GAME_TILE_WIDTH, GAME_TILE_HEIGHT);
		olc::PixelGameEngine::SetDrawTarget(sprWaterSquare[1].get());
		olc::PixelGameEngine::Clear(olc::Pixel(0, 0, 0xff, 0x40)); // slightly transparent blue
		decWaterSquare[1] = std::make_unique<olc::Decal>(sprWaterSquare[1].get());

		return true;
	}

	bool OnUserUpdate(float fElapsedTime)
	{
		nFrameCounter = (nFrameCounter + 1) % 0xFFFF;

		// O------------------------------------------------------------------------------O
		// | Blank screen + draw screen flash                                             |
		// O------------------------------------------------------------------------------O
		// Process view scaling (To do - delete this?):
		if (GetKey(olc::NP_ADD).bHeld) fZoom = fZoom * 1.005f;
		if (GetKey(olc::NP_SUB).bHeld) fZoom = fZoom / 1.005f;
		if (fZoom > SCREEN_ZOOM_MIN) fZoom = SCREEN_ZOOM_MIN;
		if (fZoom < SCREEN_ZOOM_MAX) fZoom = SCREEN_ZOOM_MAX;

		fCanvasWidth = SCREEN_WIDTH / fZoom;
		fCanvasHeight = SCREEN_HEIGHT / fZoom;

		fCanvasOffsetX = (SCREEN_WIDTH - fCanvasWidth) / 2.0f;
		fCanvasOffsetY = (SCREEN_HEIGHT - fCanvasHeight) / 2.0f;
		// O------------------------------------------------------------------------------O

		// O------------------------------------------------------------------------------O
		// | Process keys - part 1 (capture key press with every PGE frame)               |
		// O------------------------------------------------------------------------------O
		//To do: don't pass on key if it's ESCAPE
		//To do: check SHIFT and CONTROL? keys work
		olc::Key Keys[39] = { olc::ESCAPE, olc::F10, olc::F1, olc::F2, olc::F3, olc::F4, olc::F5, olc::F6, olc::F7, olc::F8, olc::F9, olc::F9,
					  olc::Key::G, olc::SPACE, olc::Key::I, olc::ESCAPE, olc::ESCAPE, olc::ESCAPE, olc::ESCAPE, olc::Key::K, olc::Key::O, olc::Key::OEM_4 /* '[' */,
					  olc::CTRL, olc::TAB, olc::Key::Y, olc::Key::U, olc::Key::T, olc::Key::R, olc::PERIOD, olc::Key::M, olc::Key::COMMA,
					  olc::Key::S, olc::Key::V, olc::Key::Q, olc::Key::W, olc::Key::P, olc::Key::P, olc::Key::L, olc::SHIFT }; //To do: why is "P" in twice?
		for (int nKey = 0; nKey < 39; nKey++) {
			if (GetKey(Keys[nKey]).bPressed || GetKey(Keys[nKey]).bHeld) {
				Game.BBC.ram[0x126b + nKey] = Game.BBC.ram[0x126b + nKey] | 0x80;
			}
		}

		// O++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++O
		// O++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++O
		// + PROCESS GAME LOOP EVERY 0.025ms                                              +
		// O++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++O
		// O++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++O
		if ((fTimeSinceLastFrame += fElapsedTime) > 0.025) {
			fTimeSinceLastFrame = 0.0f;

			// Debug - CTRL + CURSOR MOVES PLAYER THROUGH WALLS:
			if (GetKey(olc::CTRL).bHeld) {
				if (GetKey(olc::LEFT).bPressed || GetKey(olc::LEFT).bHeld) Game.BBC.ram[0x9900] = Game.BBC.ram[0x9900] - 1;
				if (GetKey(olc::RIGHT).bPressed || GetKey(olc::RIGHT).bHeld) Game.BBC.ram[0x9900] = Game.BBC.ram[0x9900] + 1;

				if (GetKey(olc::UP).bPressed || GetKey(olc::UP).bHeld) Game.BBC.ram[0x9b00] = Game.BBC.ram[0x9b00] - 1;
				if (GetKey(olc::DOWN).bPressed || GetKey(olc::DOWN).bHeld) Game.BBC.ram[0x9b00] = Game.BBC.ram[0x9b00] + 1;
			}

			// O------------------------------------------------------------------------------O
			// | Run BBC game loop                                                            |
			// O------------------------------------------------------------------------------O
			Game.BBC.cpu.pc = 0x19b6;
			do {
				do Game.BBC.cpu.clock();
				while (!Game.BBC.cpu.complete());

				// Check special pc registers:
				if (Game.BBC.cpu.pc == 0x1fa6) bScreenFlash = (Game.BBC.cpu.a == 0); // Screen flash if A = 0
				if (Game.BBC.cpu.pc == 0x260a) nEarthQuakeOffset = (Game.BBC.cpu.a & 1); // Screen shfit if A = 1

			} while (Game.BBC.cpu.pc != 0x19b6); //0x19b6 = Start of game loop
			// O------------------------------------------------------------------------------O

			// O------------------------------------------------------------------------------O
			// | Process keys - part 2 (mark "held", or reset)                                |
			// O------------------------------------------------------------------------------O
			for (int nKey = 0; nKey < 39; nKey++) {
				if (GetKey(Keys[nKey]).bHeld) {
					Game.BBC.ram[0x126b + nKey] = 0x40;
				}
				else Game.BBC.ram[0x126b + nKey] = 0x00;
			}
			// O------------------------------------------------------------------------------O

			// O------------------------------------------------------------------------------O
			// | Process scrolling                                                            |
			// O------------------------------------------------------------------------------O
			fCanvasX += fCanvasX_ScrollShift;
			fCanvasY += fCanvasY_ScrollShift;
			fCanvasX_ScrollShift = 0;
			fCanvasY_ScrollShift = 0;

			Obj O = Game.Object(0);
			if (Game.BBC.ram[0x19b5] == 0) {  // Not teleporting
				float fCanvasBorderX = fCanvasWidth * SCREEN_BORDER_SCALE; // Scroll if player moves within border
				float fCanvasBorderY = fCanvasHeight * SCREEN_BORDER_SCALE; // Scroll if player moves within border

				if (O.GameX < fCanvasX + fCanvasOffsetX + fCanvasBorderX) fCanvasX_ScrollShift = (O.GameX - fCanvasX - fCanvasOffsetX - fCanvasBorderX);
				if (O.GameX > fCanvasX + fCanvasOffsetX + (fCanvasWidth - fCanvasBorderX - 20)) fCanvasX_ScrollShift = (O.GameX - fCanvasX - fCanvasOffsetX - (fCanvasWidth - fCanvasBorderX - 20));

				if (O.GameY < fCanvasY + fCanvasOffsetY + fCanvasBorderY) fCanvasY_ScrollShift = (O.GameY - fCanvasY - fCanvasOffsetY - fCanvasBorderY);
				if (O.GameY > fCanvasY + fCanvasOffsetY + (fCanvasHeight - fCanvasBorderY - 20)) fCanvasY_ScrollShift = (O.GameY - fCanvasY - fCanvasOffsetY - (fCanvasHeight - fCanvasBorderY - 20));
			}

		}
		// O++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++O

		// O++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++O
		// O++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++O
		// + DRAW SCREEN EVERY PGE LOOP                                                   +
		// O++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++O
		// O++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++O

		// O------------------------------------------------------------------------------O
		// | Blank screen + draw screen flash                                             |
		// O------------------------------------------------------------------------------O
		Clear(olc::BLANK);
		if (bScreenFlash) Clear(olc::WHITE);
		// O------------------------------------------------------------------------------O

		// O------------------------------------------------------------------------------O
		// | Draw background water                                                        |
		// O------------------------------------------------------------------------------O
		int nTileOffsetX = int((fCanvasX + fCanvasOffsetX) / GAME_TILE_WIDTH);
		int nTileOffsetY = int((fCanvasY + fCanvasOffsetY) / GAME_TILE_HEIGHT);

		for (int i = -1; i < (fCanvasWidth / GAME_TILE_WIDTH + 2); i++) {
			int x = i + nTileOffsetX;
			float fScreenX = ScreenCoordinateX(x * GAME_TILE_WIDTH);
			float fScreenY = ScreenCoordinateY(Game.WaterLevel(x));
			if (fScreenY < -GAME_TILE_HEIGHT) fScreenY = -GAME_TILE_HEIGHT;
			olc::PixelGameEngine::DrawDecal(olc::vf2d(fScreenX, fScreenY), decWater[0].get(), olc::vf2d(fZoom, fZoom));
		}
		for (int i = 0; i < Game.WaterTiles.size(); i++) {
			float fScreenX = ScreenCoordinateX(Game.WaterTiles[i].GameX * GAME_TILE_WIDTH);
			float fScreenY = ScreenCoordinateY(Game.WaterTiles[i].GameY * GAME_TILE_HEIGHT);
			olc::PixelGameEngine::DrawDecal(olc::vf2d(fScreenX, fScreenY), decWaterSquare[0].get(), olc::vf2d(fZoom, fZoom));
		}
		// O------------------------------------------------------------------------------O

		// O------------------------------------------------------------------------------O
		// | Draw particles                                                               |
		// O------------------------------------------------------------------------------O
		uint16_t nCountParticles = Game.BBC.ram[0x1e58]; // number_of_particles

		if (nCountParticles != 0xFF) {
			for (int i = 0; i < nCountParticles + 1; i++) {
				uint16_t nXLow = Game.BBC.ram[0x8800 + i]; // particle_stack_x_low
				uint16_t nYLow = Game.BBC.ram[0x8900 + i]; // particle_stack_y_low
				uint16_t nXHigh = Game.BBC.ram[0x8a00 + i]; // particle_stack_x
				uint16_t nYHigh = Game.BBC.ram[0x8b00 + i]; // particle_stack_y
				uint8_t nType = Game.BBC.ram[0x8d00 + i]; // particle_stack_type

				uint16_t nX = (nXLow | (nXHigh << 8)) / 8;
				uint16_t nY = (nYLow | (nYHigh << 8)) / 8;
				uint8_t nCol = nType & 0x07;

				Game.DrawExileParticle(
					this, 
					ScreenCoordinateX(nX), 
					ScreenCoordinateY(nY), 
					fZoom, 
					(nType >> 6) & 1, 
					olc::Pixel(((nCol >> 0) & 1) * 0xFF, ((nCol >> 1) & 1) * 0xFF, ((nCol >> 2) & 1) * 0xFF));
			}
		}
		// O------------------------------------------------------------------------------O

		// O------------------------------------------------------------------------------O
		// | Draw game objects                                                            |
		// O------------------------------------------------------------------------------O
		for (int nObjectID = 0; nObjectID < 128; nObjectID++) {
			//Get object info

			Obj O;
			O = Game.Object(nObjectID);

			if ((nObjectID == 0) || (nObjectID == Game.BBC.ram[0xdd])) {
				// Offset the scroll shift for player (and objects held) to avoid judder
				O.GameX += fCanvasX_ScrollShift * fTimeSinceLastFrame / 0.025;
				O.GameY += fCanvasY_ScrollShift * fTimeSinceLastFrame / 0.025;
			}

			Game.DrawExileSprite(
				this,
				O.SpriteID,
				ScreenCoordinateX(O.GameX),
				ScreenCoordinateY(O.GameY),
				fZoom,
				O.Palette,
				O.HorizontalFlip,
				O.VerticalFlip,
				O.Teleporting,
				O.Timer);
		}
		// O------------------------------------------------------------------------------O

		// O------------------------------------------------------------------------------O
		// | Draw "foreground" water (transparent)                                        |
		// O------------------------------------------------------------------------------O
		for (int i = -1; i < (fCanvasWidth / GAME_TILE_WIDTH + 2); i++) {
			int x = i + nTileOffsetX;
			float fScreenX = ScreenCoordinateX(x * GAME_TILE_WIDTH);
			float fScreenY = ScreenCoordinateY(Game.WaterLevel(x));
			if (fScreenY < -GAME_TILE_HEIGHT) fScreenY = -GAME_TILE_HEIGHT;
			olc::PixelGameEngine::DrawDecal(olc::vf2d(fScreenX, fScreenY), decWater[1].get(), olc::vf2d(fZoom, fZoom));
		}
		for (int i = 0; i < Game.WaterTiles.size(); i++) {
			float fScreenX = ScreenCoordinateX(Game.WaterTiles[i].GameX * GAME_TILE_WIDTH);
			float fScreenY = ScreenCoordinateY(Game.WaterTiles[i].GameY * GAME_TILE_HEIGHT);
			olc::PixelGameEngine::DrawDecal(olc::vf2d(fScreenX, fScreenY), decWaterSquare[1].get(), olc::vf2d(fZoom, fZoom));
		}
		// O------------------------------------------------------------------------------O

		// O------------------------------------------------------------------------------O
		// | Draw background map                                                          |
		// O------------------------------------------------------------------------------O
		for (int j = 0; j < (fCanvasWidth / GAME_TILE_HEIGHT + 2); j++) {
			for (int i = 0; i < (fCanvasWidth / GAME_TILE_WIDTH + 2); i++) {

				int x = i + nTileOffsetX; // Set x and clamp
				float fTileShiftX = 0;
				if (x < 0) { fTileShiftX = (x - 1) * GAME_TILE_WIDTH; x = 1; } // x = 1, rather than 0 to avoid repeating bush
				if (x > 0xFF) { fTileShiftX = (x - 0xFF) * GAME_TILE_WIDTH; x = 0xFF; }

				int y = j + nTileOffsetY; // Set y and clamp
				float fTileShiftY = 0;
				if (y < 0) { fTileShiftY = y * GAME_TILE_HEIGHT; y = 0; }
				if (y > 0xFF) { fTileShiftY = (y - 0xFF) * GAME_TILE_HEIGHT; y = 0xFF; }

				float fScreenX = ScreenCoordinateX(Game.BackgroundGrid(x, y).GameX + fTileShiftX + 1); // Why need "+1"?
				float fScreenY = ScreenCoordinateY(Game.BackgroundGrid(x, y).GameY + fTileShiftY);

				if (Game.BackgroundGrid(x, y).TileID != 0x19) { // ID 0x19 is a blank tile
					int VerticalFlip = (Game.BackgroundGrid(x, y).Orientation >> 6) & 1; // Check bit 6 
					int HorizontalFlip = (Game.BackgroundGrid(x, y).Orientation >> 7) & 1; // Check bit 7

					Game.DrawExileSprite(
						this,
						Game.BackgroundGrid(x, y).SpriteID,
						fScreenX, fScreenY, fZoom,
						Game.BackgroundGrid(x, y).Palette,
						HorizontalFlip,
						VerticalFlip);

					Game.DetermineBackground(x, y, nFrameCounter); // Called to ensure background objects are activated when in view
				}
			}
		}

		return true;
	}
};

int main()
{
	Exile_olc6502 exile;
	exile.Construct(SCREEN_WIDTH, SCREEN_HEIGHT, 1, 1, false, true);
	exile.Start();
	return 0;
}
