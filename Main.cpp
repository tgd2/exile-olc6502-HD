#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"

#include "Exile.h"
#include <chrono>

// O------------------------------------------------------------------------------O
// | Screen constants and global variables                                        |
// O------------------------------------------------------------------------------O
uint32_t nFrameCounter = 0;
float fGlobalTime = 0;
float fTimeSinceLastFrame = 0;
std::chrono::duration<double> Time_GameLoop;
std::chrono::duration<double> Time_DrawScreen;

const float SCREEN_WIDTH = 1280;        const float SCREEN_HEIGHT = 720; // 720p display
const bool SCREEN_FULLSCREEN = false;   const bool SCREEN_VSYNC = true;
const float SCREEN_ZOOM = 2.0f;
const float SCREEN_BORDER_SCALE = 0.3f; // To trigger scrolling

float fCanvasX = 4350;
float fCanvasY = 1570;

float fCanvasWidth = SCREEN_WIDTH / SCREEN_ZOOM;
float fCanvasHeight = SCREEN_HEIGHT / SCREEN_ZOOM;
float fCanvasOffsetX = (SCREEN_WIDTH - fCanvasWidth) / 2.0f;
float fCanvasOffsetY = (SCREEN_HEIGHT - fCanvasHeight) / 2.0f;

float fScrollShiftX = 0; // For smooth scrolling
float fScrollShiftY = 0;

std::unique_ptr<olc::Sprite> sprWater[2];        std::unique_ptr<olc::Decal> decWater[2];
std::unique_ptr<olc::Sprite> sprWaterSquare[2];  std::unique_ptr<olc::Decal> decWaterSquare[2];

bool bScreenFlash = false;
uint8_t nEarthQuakeOffset = 0;

olc::Key Keys[39] = { olc::Key::D /* D = Dummy Key */, olc::ESCAPE, olc::F1, olc::F2, olc::F3, olc::F4, olc::F5, olc::F6, olc::F7, olc::F8, olc::Key::D, olc::Key::D,
			  olc::Key::G, olc::SPACE, olc::Key::I, olc::Key::D, olc::Key::D, olc::Key::D, olc::Key::D, olc::Key::K, olc::Key::O, olc::Key::OEM_4 /* '[' */,
			  olc::CTRL, olc::TAB, olc::Key::Y, olc::Key::U, olc::Key::T, olc::Key::R, olc::PERIOD, olc::Key::M, olc::Key::COMMA,
			  olc::Key::S, olc::Key::V, olc::Key::Q, olc::Key::W, olc::Key::P, olc::Key::P, olc::Key::L, olc::SHIFT };

// For debugging overlay:
bool bShowDebugGrid = false;
bool bShowDebugOverlay = false;
int nObjectCountMax = 0;
int nParticleCountMax = 0;

auto hex = [](uint32_t n, uint8_t d)
{
	std::string s(d, '0');
	for (int i = d - 1; i >= 0; i--, n >>= 4)
		s[i] = "0123456789ABCDEF"[n & 0xF];
	return s;
};
// O------------------------------------------------------------------------------O

// O------------------------------------------------------------------------------O
// | Main Exile class                                                             |
// O------------------------------------------------------------------------------O
class Exile_olc6502_HD : public olc::PixelGameEngine
{

public:
	Exile_olc6502_HD() { sAppName = "Exile"; }

	Exile Game;

	float ScreenCoordinateX(float GameCoordinateX) {
		float x = GameCoordinateX - fCanvasX - fCanvasOffsetX - (fScrollShiftX * fTimeSinceLastFrame / 0.025f);
		x += nEarthQuakeOffset * 4; // Shift screen in event of earthquake
		x = x * SCREEN_ZOOM;
		return x;
	}

	float ScreenCoordinateY(float GameCoordinateY) {
		float y = GameCoordinateY - fCanvasY - fCanvasOffsetY - (fScrollShiftY * fTimeSinceLastFrame / 0.025f);
		y = y * SCREEN_ZOOM;
		return y;
	}

	bool OnUserCreate()
	{
		Clear(olc::BLACK);

		// Setup water sprites:
		for (int i = 0; i < 2; i++) {
			olc::Pixel nCol;
			if (i == 0) nCol = olc::Pixel(0, 0, 0xff); // Blue for background
			else nCol = olc::Pixel(0, 0, 0xff, 0x60); // Transparent blue for foreground

			sprWater[i] = std::make_unique<olc::Sprite>(GAME_TILE_WIDTH, SCREEN_HEIGHT + GAME_TILE_HEIGHT);
			olc::PixelGameEngine::SetDrawTarget(sprWater[i].get());
			olc::PixelGameEngine::Clear(nCol);
			if (i == 0) olc::PixelGameEngine::DrawLine(0, 0, GAME_TILE_WIDTH, 0, olc::CYAN);
			decWater[i] = std::make_unique<olc::Decal>(sprWater[i].get());

			sprWaterSquare[i] = std::make_unique<olc::Sprite>(GAME_TILE_WIDTH, GAME_TILE_HEIGHT);
			olc::PixelGameEngine::SetDrawTarget(sprWaterSquare[i].get());
			olc::PixelGameEngine::Clear(nCol);
			decWaterSquare[i] = std::make_unique<olc::Decal>(sprWaterSquare[i].get());
		}
		int nNull = 0;
		olc::PixelGameEngine::SetDrawTarget(nNull);

		// Load Exile RAM from disassembly, and patch to run in HD:
		Game.LoadExileFromDisassembly("exile-disassembly.txt"); // www.level7.org.uk/miscellany/exile-disassembly.txt
		//Game.PatchExileRAM();
		Game.Initialise();

		return true;
	}

	bool OnUserUpdate(float fElapsedTime)
	{
		// Process cheat and debug keys:
		if (GetKey(olc::K1).bPressed) Game.Cheat_GetAllEquipment();
		if (GetKey(olc::K2).bPressed) Game.Cheat_StoreAnyObject();
		if (GetKey(olc::K3).bPressed) bShowDebugGrid = !bShowDebugGrid;
		if (GetKey(olc::K4).bPressed) bShowDebugOverlay = !bShowDebugOverlay;

		fGlobalTime = +fElapsedTime;
		nFrameCounter = (nFrameCounter + 1) % 0xFFFF;

		// O------------------------------------------------------------------------------O
		// | Process keys - Part 1 (capture key presses with every PGE frame)             |
		// O------------------------------------------------------------------------------O
		for (int nKey = 0; nKey < 39; nKey++) {
			if (GetKey(Keys[nKey]).bPressed || GetKey(Keys[nKey]).bHeld) {
				if (Keys[nKey] != olc::D) { // Ignore D (dummy) key
					Game.BBC.ram[GAME_RAM_INPUTS + nKey] = Game.BBC.ram[GAME_RAM_INPUTS + nKey] | 0x80;
				}
			}
		}
		// O------------------------------------------------------------------------------O


		// O++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++O
		// O++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++O
		// + PROCESS GAME LOOP EVERY 0.025ms                                              +
		// O++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++O
		// O++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++O
		if ((fTimeSinceLastFrame += fElapsedTime) > 0.025f) {
			fTimeSinceLastFrame = 0.0f;

			// For debugging - time game loop:
			auto TimeStart_GameLoop = std::chrono::high_resolution_clock::now();

			// For debugging - CTRL + CURSOR MOVES PLAYER THROUGH WALLS:
			if (GetKey(olc::CTRL).bHeld) {
				if (GetKey(olc::LEFT).bPressed || GetKey(olc::LEFT).bHeld) Game.BBC.ram[0x9900] = Game.BBC.ram[0x9900] - 1;
				if (GetKey(olc::RIGHT).bPressed || GetKey(olc::RIGHT).bHeld) Game.BBC.ram[0x9900] = Game.BBC.ram[0x9900] + 1;

				if (GetKey(olc::UP).bPressed || GetKey(olc::UP).bHeld) Game.BBC.ram[0x9b00] = Game.BBC.ram[0x9b00] - 1;
				if (GetKey(olc::DOWN).bPressed || GetKey(olc::DOWN).bHeld) Game.BBC.ram[0x9b00] = Game.BBC.ram[0x9b00] + 1;
			}

			// O------------------------------------------------------------------------------O
			// | Run BBC game loop                                                            |
			// O------------------------------------------------------------------------------O
			bScreenFlash = false;

			Game.BBC.cpu.pc = GAME_RAM_STARTGAMELOOP;
			do {
				do Game.BBC.cpu.clock();
				while (!Game.BBC.cpu.complete());

				// Check for special program counters:
				if (Game.BBC.cpu.pc == GAME_RAM_SCREENFLASH) bScreenFlash = (Game.BBC.cpu.a == 0); // Screen flash if A = 0
				if (Game.BBC.cpu.pc == GAME_RAM_EARTHQUAKE) nEarthQuakeOffset = (Game.BBC.cpu.a & 1); // Screen shift if A = 1

			} while (Game.BBC.cpu.pc != GAME_RAM_STARTGAMELOOP);
			// O------------------------------------------------------------------------------O

			// O------------------------------------------------------------------------------O
			// | Process keys - Part 2 (mark "held", or reset)                                |
			// O------------------------------------------------------------------------------O
			for (int nKey = 0; nKey < 39; nKey++) {
				if (GetKey(Keys[nKey]).bHeld) {
					if (Keys[nKey] != olc::D) { // Ignore D (dummy) key
						Game.BBC.ram[GAME_RAM_INPUTS + nKey] = 0x40;
					}
				}
				else Game.BBC.ram[GAME_RAM_INPUTS + nKey] = 0x00;
			}
			// O------------------------------------------------------------------------------O

			// O------------------------------------------------------------------------------O
			// | Process scrolling                                                            |
			// O------------------------------------------------------------------------------O
			fCanvasX += fScrollShiftX;
			fCanvasY += fScrollShiftY;
			fScrollShiftX = 0;
			fScrollShiftY = 0;

			Obj O = Game.Object(1, 0); // Player
			if (Game.BBC.ram[GAME_RAM_PLAYER_TELEPORTING] == 0) { // Not teleporting
				float fCanvasBorderX = fCanvasWidth * SCREEN_BORDER_SCALE; // Scroll if player moves within border
				float fCanvasBorderY = fCanvasHeight * SCREEN_BORDER_SCALE; // Scroll if player moves within border

				if (O.GameX < fCanvasX + fCanvasOffsetX + fCanvasBorderX) fScrollShiftX = (O.GameX - fCanvasX - fCanvasOffsetX - fCanvasBorderX);
				if (O.GameX > fCanvasX + fCanvasOffsetX + (fCanvasWidth - fCanvasBorderX - 20)) fScrollShiftX = (O.GameX - fCanvasX - fCanvasOffsetX - (fCanvasWidth - fCanvasBorderX - 20));

				if (O.GameY < fCanvasY + fCanvasOffsetY + fCanvasBorderY) fScrollShiftY = (O.GameY - fCanvasY - fCanvasOffsetY - fCanvasBorderY);
				if (O.GameY > fCanvasY + fCanvasOffsetY + (fCanvasHeight - fCanvasBorderY - 20)) fScrollShiftY = (O.GameY - fCanvasY - fCanvasOffsetY - (fCanvasHeight - fCanvasBorderY - 20));
			}
			// O------------------------------------------------------------------------------O

			// For debugging - time game loop:
			auto TimeStop_GameLoop = std::chrono::high_resolution_clock::now();
			Time_GameLoop = std::chrono::duration_cast<std::chrono::nanoseconds> (TimeStop_GameLoop - TimeStart_GameLoop);

		}
		// O++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++O
	

		// O++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++O
		// O++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++O
		// + DRAW SCREEN EVERY PGE LOOP                                                   +
		// O++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++O
		// O++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++O

		// For debugging - time draw screen:
		auto TimeStart_DrawScreen = std::chrono::high_resolution_clock::now();

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

		for (int i = -1; i < (fCanvasWidth / GAME_TILE_WIDTH + 1); i++) { // Draw general water level:
			int x = i + nTileOffsetX;
			float fScreenX = ScreenCoordinateX(x * GAME_TILE_WIDTH);
			float fScreenY = ScreenCoordinateY(Game.WaterLevel(x));
			if (fScreenY < -GAME_TILE_HEIGHT) fScreenY = -GAME_TILE_HEIGHT;
			olc::PixelGameEngine::DrawDecal(olc::vf2d(fScreenX, fScreenY), decWater[0].get(), olc::vf2d(SCREEN_ZOOM, SCREEN_ZOOM));
		}
		for (int i = 0; i < Game.WaterTiles.size(); i++) { // Draw specific water tiles throughout map:
			float fScreenX = ScreenCoordinateX(Game.WaterTiles[i].GameX * GAME_TILE_WIDTH);
			float fScreenY = ScreenCoordinateY(Game.WaterTiles[i].GameY * GAME_TILE_HEIGHT);
			olc::PixelGameEngine::DrawDecal(olc::vf2d(fScreenX, fScreenY), decWaterSquare[0].get(), olc::vf2d(SCREEN_ZOOM, SCREEN_ZOOM));
		}
		// O------------------------------------------------------------------------------O

		// O------------------------------------------------------------------------------O
		// | Draw particles                                                               |
		// O------------------------------------------------------------------------------O
		uint16_t nParticleCount = Game.BBC.ram[0x1e58]; // number_of_particles

		if (nParticleCount != 0xFF) {
			for (int i = 0; i < nParticleCount + 1; i++) {
				ExileParticle P;
				P = Game.Particle(i);

				uint8_t nCol = P.ParticleType & 0x07;

				Game.DrawExileParticle(
					this, 
					ScreenCoordinateX(P.GameX),
					ScreenCoordinateY(P.GameY),
					SCREEN_ZOOM, 
					(P.ParticleType >> 6) & 1,
					olc::Pixel(((nCol >> 0) & 1) * 0xFF, ((nCol >> 1) & 1) * 0xFF, ((nCol >> 2) & 1) * 0xFF));
			}
		}
		// O------------------------------------------------------------------------------O

		// O------------------------------------------------------------------------------O
		// | Draw game objects - Primary stack                                            |
		// O------------------------------------------------------------------------------O
		int nObjectCount = 0;
		for (int nObjectID = 0; nObjectID < 16; nObjectID++) {
			Obj O;
			O = Game.Object(1, nObjectID);

			if ((nObjectID == 0) || (nObjectID == Game.BBC.ram[0xdd])) {
				// Offset the scroll shift for player (and objects held) to avoid judder
				O.GameX += fScrollShiftX * fTimeSinceLastFrame / 0.025;
				O.GameY += fScrollShiftY * fTimeSinceLastFrame / 0.025;
			}

			Game.DrawExileSprite(
				this,
				O.SpriteID,
				ScreenCoordinateX(O.GameX),
				ScreenCoordinateY(O.GameY),
				SCREEN_ZOOM,
				O.Palette,
				O.HorizontalFlip,
				O.VerticalFlip,
				O.Teleporting,
				O.Timer);

			if (Game.BBC.ram[0x08b4 + nObjectID] != 0) nObjectCount++; // For debugging
		}
		// O------------------------------------------------------------------------------O

		// O------------------------------------------------------------------------------O
		// | Draw game objects - Secondary stack                                          |
		// O------------------------------------------------------------------------------O
		for (int nObjectID = 0; nObjectID < 32; nObjectID++) {
			Obj O;
			O = Game.Object(2, nObjectID);

			if (O.IsValid)
			{
				Game.DrawExileSprite(
					this,
					O.SpriteID,
					ScreenCoordinateX(O.GameX),
					ScreenCoordinateY(O.GameY),
					SCREEN_ZOOM,
					O.Palette - 1,
					O.HorizontalFlip,
					O.VerticalFlip,
					O.Teleporting,
					O.Timer);
			}
		}
		// O------------------------------------------------------------------------------O

		// O------------------------------------------------------------------------------O
		// | Draw "foreground" water (transparent)                                        |
		// O------------------------------------------------------------------------------O
		for (int i = -1; i < (fCanvasWidth / GAME_TILE_WIDTH + 1); i++) { // Draw general water level:
			int x = i + nTileOffsetX;
			float fScreenX = ScreenCoordinateX(x * GAME_TILE_WIDTH);
			float fScreenY = ScreenCoordinateY(Game.WaterLevel(x));
			if (fScreenY < -GAME_TILE_HEIGHT) fScreenY = -GAME_TILE_HEIGHT;
			olc::PixelGameEngine::DrawDecal(olc::vf2d(fScreenX, fScreenY), decWater[1].get(), olc::vf2d(SCREEN_ZOOM, SCREEN_ZOOM));
		}
		for (int i = 0; i < Game.WaterTiles.size(); i++) { // Draw specific water tiles throughout map:
			float fScreenX = ScreenCoordinateX(Game.WaterTiles[i].GameX * GAME_TILE_WIDTH);
			float fScreenY = ScreenCoordinateY(Game.WaterTiles[i].GameY * GAME_TILE_HEIGHT);
			olc::PixelGameEngine::DrawDecal(olc::vf2d(fScreenX, fScreenY), decWaterSquare[1].get(), olc::vf2d(SCREEN_ZOOM, SCREEN_ZOOM));
		}
		// O------------------------------------------------------------------------------O

		// O------------------------------------------------------------------------------O
		// | Draw background map                                                          |
		// O------------------------------------------------------------------------------O
		for (int j = -1; j < (fCanvasHeight / GAME_TILE_HEIGHT + 1); j++) {
			for (int i = -1; i < (fCanvasWidth / GAME_TILE_WIDTH + 1); i++) {

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

				if (Game.BackgroundGrid(x, y).TileID != GAME_TILE_BLANK) {
					int VerticalFlip = (Game.BackgroundGrid(x, y).Orientation >> 6) & 1; // Check bit 6 
					int HorizontalFlip = (Game.BackgroundGrid(x, y).Orientation >> 7) & 1; // Check bit 7

					Game.DrawExileSprite(
						this,
						Game.BackgroundGrid(x, y).SpriteID,
						fScreenX, fScreenY, SCREEN_ZOOM,
						Game.BackgroundGrid(x, y).Palette,
						HorizontalFlip,
						VerticalFlip);
				}

				Game.DetermineBackground(x, y, nFrameCounter); // Called to ensure background objects are activated when in view

			}
		}

		// For debugging - time draw screen:
		auto TimeStop_DrawScreen = std::chrono::high_resolution_clock::now();
		Time_DrawScreen = std::chrono::duration_cast<std::chrono::nanoseconds> (TimeStop_DrawScreen - TimeStart_DrawScreen);

		// O------------------------------------------------------------------------------O
		// | Draw debug grid and overlay                                                  |
		// O------------------------------------------------------------------------------O
		if (bShowDebugGrid) {
			for (int j = -1; j < (fCanvasWidth / GAME_TILE_HEIGHT + 1); j++) {
				for (int i = -1; i < (fCanvasWidth / GAME_TILE_WIDTH + 1); i++) {
					int x = i + nTileOffsetX;
					float fScreenX = ScreenCoordinateX(x * GAME_TILE_WIDTH);
					int y = j + nTileOffsetY;
					float fScreenY = ScreenCoordinateY(y * GAME_TILE_HEIGHT);
					olc::PixelGameEngine::DrawStringDecal(olc::vd2d(fScreenX + 2, fScreenY + 2), hex(x, 2), olc::WHITE);
					olc::PixelGameEngine::DrawStringDecal(olc::vd2d(fScreenX + 2, fScreenY + 12), hex(y, 2), olc::WHITE);
				}
			}

			for (int i = -1; i < (fCanvasWidth / GAME_TILE_WIDTH + 1); i++) {
				int x = i + nTileOffsetX;
				float fScreenX = ScreenCoordinateX(x * GAME_TILE_WIDTH);
				olc::PixelGameEngine::FillRectDecal(olc::vd2d(fScreenX, 0), olc::vd2d(1, SCREEN_HEIGHT), olc::WHITE);
			}

			for (int j = -1; j < (fCanvasWidth / GAME_TILE_HEIGHT + 1); j++) {
				int y = j + nTileOffsetY;
				float fScreenY = ScreenCoordinateY(y * GAME_TILE_HEIGHT);
				olc::PixelGameEngine::FillRectDecal(olc::vd2d(0, fScreenY), olc::vd2d(SCREEN_WIDTH, 1), olc::WHITE);
			}
		}

		if (bShowDebugOverlay) {
			if (nObjectCount > nObjectCountMax) nObjectCountMax = nObjectCount;
			if (nParticleCount > nParticleCountMax) nParticleCountMax = nParticleCount;

			olc::PixelGameEngine::FillRectDecal(olc::vd2d(0, 0), olc::vd2d(375, 205), olc::VERY_DARK_GREY);

			olc::PixelGameEngine::DrawStringDecal(olc::vd2d(32, 32), "OBJECTS:   " + std::to_string(nObjectCount), olc::YELLOW, olc::vf2d(2.0f, 2.0f));
			olc::PixelGameEngine::DrawStringDecal(olc::vd2d(32, 52), "MAX:       " + std::to_string(nObjectCountMax), olc::YELLOW, olc::vf2d(2.0f, 2.0f));

			olc::PixelGameEngine::DrawStringDecal(olc::vd2d(32, 84), "PARTICLES: " + std::to_string(nParticleCount), olc::YELLOW, olc::vf2d(2.0f, 2.0f));
			olc::PixelGameEngine::DrawStringDecal(olc::vd2d(32, 104), "MAX:       " + std::to_string(nParticleCountMax), olc::YELLOW, olc::vf2d(2.0f, 2.0f));

			olc::PixelGameEngine::DrawStringDecal(olc::vd2d(32, 136), "TIME GAME: " + std::to_string(Time_GameLoop.count()), olc::GREEN, olc::vf2d(2.0f, 2.0f));
			olc::PixelGameEngine::DrawStringDecal(olc::vd2d(32, 156), "TIME DRAW: " + std::to_string(Time_DrawScreen.count()), olc::GREEN, olc::vf2d(2.0f, 2.0f));
		}
		// O------------------------------------------------------------------------------O

		return true;
	}

};

int main()
{
	Exile_olc6502_HD exile;
	exile.Construct(SCREEN_WIDTH, SCREEN_HEIGHT, 1, 1, SCREEN_FULLSCREEN, SCREEN_VSYNC);
	exile.Start();
	return 0;
}