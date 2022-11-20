#pragma once
#include "olcPixelGameEngine.h"
#include "olc6502.h"
#include "Bus.h"

#include <array>
#include <cstdint>
#include <string>

const int GAME_TILE_WIDTH = 32;
const int GAME_TILE_HEIGHT = 32;
const uint8_t GAME_TILE_BLANK = 0x19;

const uint16_t GAME_RAM_INPUTS = 0x126b;
const uint16_t GAME_RAM_PLAYER_TELEPORTING = 0x19b5;
const uint16_t GAME_RAM_STARTGAMELOOP = 0x19b6;
const uint16_t GAME_RAM_SCREENFLASH = 0x1fa6;
const uint16_t GAME_RAM_EARTHQUAKE = 0x260a;

struct XY {
	uint16_t GameX;          uint16_t GameY;
};

struct Obj {
	bool IsValid;            uint8_t ObjectType;
	uint8_t SpriteID;        uint8_t Palette;
	uint8_t HorizontalFlip;  uint8_t VerticalFlip;  
	uint8_t Teleporting;     uint8_t Timer;
	uint16_t GameX;          uint16_t GameY;
};

struct Tile {
	uint8_t TileID;          uint8_t SpriteID;
	uint8_t Orientation;     uint8_t Palette;
	uint16_t GameX;          uint16_t GameY;
	uint32_t FrameLastDrawn;
//	bool IsBackgroundObject;
//	uint8_t nBackgroundObject_Handler_Index{ 0 };
//	uint8_t nBackgroundObject_Data_Index{ 0 };
//	uint8_t nBackgroundObject_Type_Index{ 0 };
};

struct ExileParticle {
	uint8_t ParticleType;    
	uint16_t GameX;          uint16_t GameY;
};

class Exile
{

private:
	bool ParseAssemblyLine(std::string sLine);
	void GenerateBackgroundGrid();
	void GenerateSpriteSheet();

	void CopyRAM(uint16_t nSource, uint16_t nTarget, uint8_t nLength);

	std::array <std::array<Tile, 256>, 256> TileGrid;
	uint8_t nSpriteSheet[128][128];

	std::map<uint32_t, olc::Decal*> SpriteDecals;

	void DrawExileSprite_PixelByPixel(olc::PixelGameEngine* PGE, uint8_t nSpriteID, int32_t nX, int32_t nY, uint8_t nPaletteID, uint8_t nHorizontalInvert = 0, uint8_t nVerticalInvert = 0, uint8_t nTeleporting = 0, uint8_t nTimer = 0);

public:
	Bus BBC;
	olc6502 cpu;

	void Initialise();
	bool LoadExileFromDisassembly(std::string sFile);
	void PatchExileRAM();

	std::vector<XY> WaterTiles;

	Obj Object(uint8_t nStackID, uint8_t nObjectID);
	Tile BackgroundGrid(uint8_t x, uint8_t y);
	ExileParticle Particle(uint8_t nparticleID);

	void DetermineBackground(uint8_t x, uint8_t y, uint16_t nFrameCounter);

	uint8_t SpriteSheet(uint8_t i, uint8_t j);
	uint16_t WaterLevel(uint8_t x);

	void DrawExileParticle(olc::PixelGameEngine* PGE, int32_t nScreenX, int32_t nScreenY, float fZoom, uint8_t nDoubleHeight, olc::Pixel p);
	void DrawExileSprite(olc::PixelGameEngine* PGE, uint8_t nSpriteID, int32_t nScreenX, int32_t nScreenY, float fZoom, uint8_t nPaletteID, uint8_t nHorizontalInvert = 0, uint8_t nVerticalInvert = 0, uint8_t nTeleporting = 0, uint8_t nTimer = 0);

	void Cheat_GetAllEquipment();
	void Cheat_StoreAnyObject();
};
