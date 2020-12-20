#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"

//#define WITHSOUND

#ifdef WITHSOUND
#include "SoLoud/soloud.h"
#include "SoLoud/soloud_wav.h"
#endif

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

const float SCREEN_WIDTH = 1920;        const float SCREEN_HEIGHT = 1080; // 1080p display
const bool SCREEN_FULLSCREEN = false;   const bool SCREEN_VSYNC = true;
const float SCREEN_ZOOM_MIN = 1.0f;
const float SCREEN_ZOOM_MAX = 5.0f; 
const float SCREEN_BORDER_SCALE = 0.3f; // To trigger scrolling

float fCanvasX = 4030;
float fCanvasY = 1400;
float fZoom = 2.0f;
float fTargetZoom = 2.0f;

float fCanvasWidth = SCREEN_WIDTH / fZoom;
float fCanvasHeight = SCREEN_HEIGHT / fZoom;
float fCanvasOffsetX = (SCREEN_WIDTH - fCanvasWidth) / 2.0f;
float fCanvasOffsetY = (SCREEN_HEIGHT - fCanvasHeight) / 2.0f;

float fScrollShiftX = 0; // For smooth scrolling
float fScrollShiftY = 0;

std::unique_ptr<olc::Sprite> sprWater[2];        std::unique_ptr<olc::Decal> decWater[2];
std::unique_ptr<olc::Sprite> sprWaterSquare[2];  std::unique_ptr<olc::Decal> decWaterSquare[2];

bool bScreenFlash = false;
uint8_t nEarthQuakeOffset = 0;

#ifdef WITHSOUND
const int nWavesCount = 56;
SoLoud::Soloud gSoloud; // SoLoud engine
SoLoud::Wav gWaves[nWavesCount];  // Wave files
float fGlobalVolume = 0.5;
float fTimeSinceLastMushroomNoise = 0;

uint16_t Sounds[nWavesCount] = { 0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, // Wave files named after the
                                 0x149d, 0x14a5, 0x14ad, 0x14b5, 0x1c32, 0x2497, 0x249e, // program counter when they
								 0x261f, 0x2a17, 0x2c34, 0x2c9e, 0x2cb4, 0x2d66, 0x2d6f, // are called
								 0x31d0, 0x351a, 0x3ff9, 0x40be, 0x40db, 0x423e, 0x42be,
								 0x42d4, 0x431e, 0x4356, 0x436c, 0x437f, 0x4394, 0x43f9,
								 0x440d, 0x4425, 0x460c, 0x4638, 0x47aa, 0x480e, 0x4858,
								 0x4928, 0x49b6, 0x4a09, 0x4a61, 0x4a7d, 0x4b93, 0x4be0,
								 0x4c61, 0x4d2a, 0x4d33, 0x4e2d, 0x4ea9, 0x4eb0, 0x4f57 };
#endif

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
		x = x * fZoom;
		return x;
	}

	float ScreenCoordinateY(float GameCoordinateY) {
		float y = GameCoordinateY - fCanvasY - fCanvasOffsetY - (fScrollShiftY * fTimeSinceLastFrame / 0.025f);
		y = y * fZoom;
		return y;
	}

	bool OnUserCreate()
	{
		Clear(olc::BLACK);

		#ifdef WITHSOUND
		// Initialise sound:
		gSoloud.init();
		gSoloud.setMaxActiveVoiceCount(32);
		for (int nSound = 0; nSound < nWavesCount; nSound++) {
			std::string sFilePathName = "./Sounds/" + hex(Sounds[nSound], 4) + ".wav";
			const char* c = sFilePathName.c_str();
			gWaves[nSound].load(c);
			gWaves[nSound].setInaudibleBehavior(false, false); // Play now or never!
		}
		#endif

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
		Game.PatchExileRAM();
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

		#ifdef WITHSOUND
		if (nFrameCounter == 20) gSoloud.playClocked(fGlobalTime, gWaves[0], fGlobalVolume); // Welcome to the world of Exile
		fTimeSinceLastMushroomNoise = +fElapsedTime;
		#endif

		fGlobalTime = +fElapsedTime;
		nFrameCounter = (nFrameCounter + 1) % 0xFFFF;

		// O------------------------------------------------------------------------------O
		// | Process view scaling                                                         |
		// O------------------------------------------------------------------------------O
		if (GetKey(olc::EQUALS).bPressed) fTargetZoom += 1.0f;
		if (GetKey(olc::MINUS).bPressed) fTargetZoom -= 1.0f;
		if (fTargetZoom > SCREEN_ZOOM_MAX) fTargetZoom = SCREEN_ZOOM_MAX;
		if (fTargetZoom < SCREEN_ZOOM_MIN) fTargetZoom = SCREEN_ZOOM_MIN;
		if (fZoom > fTargetZoom) fZoom = fZoom / 1.01f;
		if (fZoom < fTargetZoom) fZoom = fZoom * 1.01f;
		if (std::abs(fTargetZoom - fZoom) < 0.01) fZoom = fTargetZoom;

		fCanvasWidth = SCREEN_WIDTH / fZoom;
		fCanvasHeight = SCREEN_HEIGHT / fZoom;
		fCanvasOffsetX = (SCREEN_WIDTH - fCanvasWidth) / 2.0f;
		fCanvasOffsetY = (SCREEN_HEIGHT - fCanvasHeight) / 2.0f;
		// O------------------------------------------------------------------------------O

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
			#ifdef WITHSOUND
			uint8_t nSoundData_Low = 0;
			uint8_t nSoundData_High = 0;
			uint16_t nSoundData = 0;
			#endif

			Game.BBC.cpu.pc = GAME_RAM_STARTGAMELOOP;
			do {
				do Game.BBC.cpu.clock();
				while (!Game.BBC.cpu.complete());

				// Check for special program counters:
				if (Game.BBC.cpu.pc == GAME_RAM_SCREENFLASH) bScreenFlash = (Game.BBC.cpu.a == 0); // Screen flash if A = 0
				if (Game.BBC.cpu.pc == GAME_RAM_EARTHQUAKE) nEarthQuakeOffset = (Game.BBC.cpu.a & 1); // Screen shift if A = 1

				// O------------------------------------------------------------------------------O
				// Process sounds - TEMPORARY CODE
				// O------------------------------------------------------------------------------O
				#ifdef WITHSOUND
				if (Game.BBC.cpu.pc == 0x1405) nSoundData_Low = (Game.BBC.cpu.a);
				if (Game.BBC.cpu.pc == 0x140c) nSoundData_High = (Game.BBC.cpu.a);

				if ((nSoundData_Low != 0) && (nSoundData_High != 0)) {
					nSoundData = (nSoundData_High << 8) | nSoundData_Low;
					nSoundData -= 2; // We want the start of call to play_sound

					// Use Master samples, where they exist:
					if (nSoundData == 0x2497) nSoundData = 1 + rand() % 4; // Ignore 0x249e
					if (nSoundData == 0x4858) nSoundData = 5;
					if (nSoundData == 0x480e) nSoundData = 6;

					nSoundData_Low = 0;
					nSoundData_High = 0;
				}

				if ((Game.BBC.cpu.pc == 0x141c) && (nSoundData != 0)) {
					int nSample = -1;
					uint8_t nDistanceFromSound = Game.BBC.cpu.a;

					// Reduce volume / ignore lounder sounds:
					if (nSoundData == 0x261f) nDistanceFromSound = nDistanceFromSound | 0xf0; // Fire - loud static
					if (nSoundData == 0x43f9) nDistanceFromSound = nDistanceFromSound | 0xf0; // Hover ball
					if ((nDistanceFromSound != 0) && (nSoundData == 0x440d)) nDistanceFromSound = nDistanceFromSound | 0xf0; // Teleport
					if (nSoundData == 0x460c) nDistanceFromSound = nDistanceFromSound | 0xf0; // Imp
					if (nSoundData == 0x4f57) nDistanceFromSound = nDistanceFromSound | 0xf0; // Bee
					if (nSoundData == 0x4c61) nDistanceFromSound = nDistanceFromSound | 0xf0; // ? - loud static					

					for (int nSound = 0; nSound < nWavesCount; nSound++) {
						if (Sounds[nSound] == nSoundData) nSample = nSound;
					}
					if (nSample != -1) {
						if (nDistanceFromSound < 2) nDistanceFromSound = 0;
						else nDistanceFromSound -= 2;
						float fVolume = fGlobalVolume * (0xff - nDistanceFromSound) / 0xff;

						if (nSoundData == 0x3ff9) { // Mushroom noise
							fVolume = fVolume / 4.0f;
							if (fTimeSinceLastMushroomNoise > 1.0f) {
								gSoloud.playClocked(fGlobalTime, gWaves[nSample], fVolume);
								fTimeSinceLastMushroomNoise = 0.0f;
							}
						}
						else { // All other sounds
							gSoloud.playClocked(fGlobalTime, gWaves[nSample], fVolume);
						}
					}
					nSoundData = 0;
				}
				#endif
				// O------------------------------------------------------------------------------O

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

			Obj O = Game.Object(0); // Player
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

		for (int i = -1; i < (fCanvasWidth / GAME_TILE_WIDTH + 2); i++) { // Draw general water level:
			int x = i + nTileOffsetX;
			float fScreenX = ScreenCoordinateX(x * GAME_TILE_WIDTH);
			float fScreenY = ScreenCoordinateY(Game.WaterLevel(x));
			if (fScreenY < -GAME_TILE_HEIGHT) fScreenY = -GAME_TILE_HEIGHT;
			olc::PixelGameEngine::DrawDecal(olc::vf2d(fScreenX, fScreenY), decWater[0].get(), olc::vf2d(fZoom, fZoom));
		}
		for (int i = 0; i < Game.WaterTiles.size(); i++) { // Draw specific water tiles throughout map:
			float fScreenX = ScreenCoordinateX(Game.WaterTiles[i].GameX * GAME_TILE_WIDTH);
			float fScreenY = ScreenCoordinateY(Game.WaterTiles[i].GameY * GAME_TILE_HEIGHT);
			olc::PixelGameEngine::DrawDecal(olc::vf2d(fScreenX, fScreenY), decWaterSquare[0].get(), olc::vf2d(fZoom, fZoom));
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
					fZoom, 
					(P.ParticleType >> 6) & 1,
					olc::Pixel(((nCol >> 0) & 1) * 0xFF, ((nCol >> 1) & 1) * 0xFF, ((nCol >> 2) & 1) * 0xFF));
			}
		}
		// O------------------------------------------------------------------------------O

		// O------------------------------------------------------------------------------O
		// | Draw game objects                                                            |
		// O------------------------------------------------------------------------------O
		int nObjectCount = 0;
		for (int nObjectID = 0; nObjectID < 128; nObjectID++) {
			Obj O;
			O = Game.Object(nObjectID);

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
				fZoom,
				O.Palette,
				O.HorizontalFlip,
				O.VerticalFlip,
				O.Teleporting,
				O.Timer);

			if (Game.BBC.ram[0x9b00 + nObjectID] != 0) nObjectCount++; // For debugging
		}
		// O------------------------------------------------------------------------------O

		// O------------------------------------------------------------------------------O
		// | Draw "foreground" water (transparent)                                        |
		// O------------------------------------------------------------------------------O
		for (int i = -1; i < (fCanvasWidth / GAME_TILE_WIDTH + 2); i++) { // Draw general water level:
			int x = i + nTileOffsetX;
			float fScreenX = ScreenCoordinateX(x * GAME_TILE_WIDTH);
			float fScreenY = ScreenCoordinateY(Game.WaterLevel(x));
			if (fScreenY < -GAME_TILE_HEIGHT) fScreenY = -GAME_TILE_HEIGHT;
			olc::PixelGameEngine::DrawDecal(olc::vf2d(fScreenX, fScreenY), decWater[1].get(), olc::vf2d(fZoom, fZoom));
		}
		for (int i = 0; i < Game.WaterTiles.size(); i++) { // Draw specific water tiles throughout map:
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

				if (Game.BackgroundGrid(x, y).TileID != GAME_TILE_BLANK) {
					int VerticalFlip = (Game.BackgroundGrid(x, y).Orientation >> 6) & 1; // Check bit 6 
					int HorizontalFlip = (Game.BackgroundGrid(x, y).Orientation >> 7) & 1; // Check bit 7

					Game.DrawExileSprite(
						this,
						Game.BackgroundGrid(x, y).SpriteID,
						fScreenX, fScreenY, fZoom,
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
			for (int j = 0; j < (fCanvasWidth / GAME_TILE_HEIGHT + 2); j++) {
				for (int i = 0; i < (fCanvasWidth / GAME_TILE_WIDTH + 2); i++) {
					int x = i + nTileOffsetX;
					float fScreenX = ScreenCoordinateX(x * GAME_TILE_WIDTH);
					int y = j + nTileOffsetY;
					float fScreenY = ScreenCoordinateY(y * GAME_TILE_HEIGHT);
					olc::PixelGameEngine::DrawStringDecal(olc::vd2d(fScreenX + 2, fScreenY + 2), hex(x, 2), olc::WHITE);
					olc::PixelGameEngine::DrawStringDecal(olc::vd2d(fScreenX + 2, fScreenY + 12), hex(y, 2), olc::WHITE);
				}
			}

			for (int i = 0; i < (fCanvasWidth / GAME_TILE_WIDTH + 2); i++) {
				int x = i + nTileOffsetX;
				float fScreenX = ScreenCoordinateX(x * GAME_TILE_WIDTH);
				olc::PixelGameEngine::FillRectDecal(olc::vd2d(fScreenX, 0), olc::vd2d(1, SCREEN_HEIGHT), olc::WHITE);
			}

			for (int j = 0; j < (fCanvasWidth / GAME_TILE_HEIGHT + 2); j++) {
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

		return true;
	}

	bool OnUserDestroy()
	{
		#ifdef WITHSOUND
		gSoloud.deinit();
		#endif
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