#include <iterator>
#include "Exile.h"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

bool Exile::ParseAssemblyLine(std::string sLine) {
	static uint16_t nRam = 0x00;

	// Split line, assuming space delimited
	std::istringstream iss(sLine);
	std::vector<std::string> sLineParsed((std::istream_iterator<std::string>(iss)), std::istream_iterator<std::string>());

	if (sLineParsed.size() > 0) {
		for (int nWord = 0; nWord < sLineParsed.size(); nWord++) {
			std::string sWord = sLineParsed[nWord];
			if (sWord == ";" || sWord == "#") nWord = sLineParsed.size(); // Skip line
			else if (nWord == 0) {
				if (sWord.length() != 6) nWord = sLineParsed.size(); // Skip line, as first word not length 6
				else if (sWord.front() != '#' && sWord.front() != '&'); // Skip line, as RAM location needs to start with # or &
				else {
					// Trim first word, and update RAM counter
					sWord.erase(0, 1);
					sWord.erase(4, 1); // 4, as now length 5
					nRam = std::stoi(sWord, nullptr, 16);
				}
			}
			else {
				if (sWord.length() != 2) nWord = sLineParsed.size(); // Skip line, as reached end of hex
				else if (sWord == "--") nWord = sLineParsed.size(); // Skip line, as hex not defined
				else {
					// Read byte into RAM
					BBC.ram[nRam] = (uint8_t)std::stoi(sWord, nullptr, 16);
					nRam++;
				}
			}
		}
	}

	return true;
}

bool Exile::LoadExileFromDisassembly(std::string sFile) {
	// Load Exile RAM from disassembly:
	std::string sLine;
	std::ifstream fileExileDisassembly(sFile);
	if (fileExileDisassembly.is_open())
	{
		bool bLoadStarted = false;

		while (getline(fileExileDisassembly, sLine))
		{
			if ((sLine.substr(0, 5) == "#0100") || bLoadStarted) {
				bLoadStarted = true;
				ParseAssemblyLine(sLine);
			}
		}

		fileExileDisassembly.close();
	}
	else {
		std::cout << "Exile disassembly file missing";
		return false;
	}

	// Patch some minor corrections to disassembly:
	BBC.ram[0x05D4] = 0x1D; BBC.ram[0x086E] = 0x3C; BBC.ram[0x08A3] = 0x50;	BBC.ram[0x0915] = 0x0F;
	BBC.ram[0x094E] = 0xFF;	BBC.ram[0x115D] = 0x2F; BBC.ram[0x14B2] = 0xFF;	BBC.ram[0x14B4] = 0x60;
	BBC.ram[0x215A] = 0x60; BBC.ram[0x2628] = 0xA5; BBC.ram[0x2629] = 0xC0; BBC.ram[0x29F3] = 0x1C;
	BBC.ram[0x29F4] = 0x22;	BBC.ram[0x29F5] = 0x32; BBC.ram[0x29F6] = 0x38; BBC.ram[0x2D6B] = 0x3D;
	BBC.ram[0x31D6] = 0xC4; BBC.ram[0x3520] = 0xC2; BBC.ram[0x3E99] = 0x3C; BBC.ram[0x40C1] = 0x57;
	BBC.ram[0x40C2] = 0x07;	BBC.ram[0x40C3] = 0x43; BBC.ram[0x40C4] = 0xF6; BBC.ram[0x436F] = 0x05;
	BBC.ram[0x442C] = 0x20; BBC.ram[0x442D] = 0xB9; BBC.ram[0x442E] = 0x4A; BBC.ram[0x492E] = 0x82;
	BBC.ram[0x492F] = 0xA9; BBC.ram[0x4930] = 0x4B; BBC.ram[0x4E33] = 0x97;

	return true;
}

void Exile::CopyRAM(uint16_t nSource, uint16_t nTarget, uint8_t nLength) {
	for (int i = 0; i < nLength; i++) {
		BBC.ram[nTarget + i] = BBC.ram[nSource + i];
	}
}

void Exile::PatchExileRAM() {
	std::string P = "";

	// Relocate primary object stacks, to give space for 128 objects
	CopyRAM(0x0860, 0x9600, 0x10);  // object_stack_type
	CopyRAM(0x0870, 0x9700, 0x10);  // object_stack_sprite
	CopyRAM(0x0880, 0x9800, 0x10);  CopyRAM(0x0890, 0x9880, 0x01);  // object_stack_x_low
	CopyRAM(0x0891, 0x9900, 0x10);  CopyRAM(0x08a1, 0x9980, 0x02);  // object_stack_x
	CopyRAM(0x08a3, 0x9a00, 0x10);  CopyRAM(0x08b3, 0x9a80, 0x01);  // object_stack_y_low
	CopyRAM(0x08b4, 0x9b00, 0x10);  CopyRAM(0x08c4, 0x9b80, 0x02);  // object_stack_y
	CopyRAM(0x08c6, 0x9c00, 0x10);  // object_stack_flags
	CopyRAM(0x08d6, 0x9d00, 0x10);  // object_stack_palette
	CopyRAM(0x08e6, 0x9e00, 0x10);  // object_stack_vel_x
	CopyRAM(0x08f6, 0x9f00, 0x10);  // object_stack_vel_y
	P += "#a000: a0 00 e0 e0 a0 a0 a0 a0 a0 a0 20 80 20 20 20 00 \n"; // object_stack_target
	P += "#a800: 10 00 10 10 11 11 11 11 10 10 11 10 11 10 11 02 \n"; // object_stack_target_object (new stack)
	CopyRAM(0x0916, 0xa100, 0x10);  // object_stack_tx
	CopyRAM(0x0926, 0xa200, 0x10);  // object_stack_energy
	CopyRAM(0x0936, 0xa300, 0x10);  // object_stack_ty
	CopyRAM(0x0946, 0xa400, 0x10);  // object_stack_supporting
	CopyRAM(0x0956, 0xa500, 0x10);  // object_stack_timer
	CopyRAM(0x0966, 0xa600, 0x10);  // object_stack_data_pointer
	CopyRAM(0x0976, 0xa700, 0x10);  // object_stack_extra

	// Various updates to code to refer to new stack locations
	// Note: Currently, there is also a 'redirect' to the new stack locations in olc6502
	// To do: update patched code here, so that the redirect is not required

	P += "&1a62: 4c 10 ff   JMP &ff10       ; JUMP TO PATCHED CODE \n";
	P += "&ff10: bd 06 09   LDA &0906, X    ; object_stack_target \n"; // (only need bits 5 to 7) 
	P += "&ff13: 85 3e      STA &3e         ; this_object_target \n";
	P += "&ff15: 85 3f      STA &3f         ; this_object_target_old \n";
	P += "&ff17: bd 00 a8   LDA &a800, X    ; object_stack_target_object \n"; // (new stack)
	P += "&ff1a: 85 0e      STA &0e         ; this_object_target_object \n";
	P += "&ff1c: 4c 6d 1a   JMP 1a6d        ; JUMP BACK \n";

	P += "&1a8f: 4c 00 ff   JMP ff00        ; JUMP TO PATCHED CODE \n";
	P += "&ff00: 29 0f      AND #&0f \n"; 
	P += "&ff01: 0a         ASL \n";
	P += "&ff02: 0a         ASL \n";
	P += "&ff03: 0a         ASL \n";
	P += "&ff04: 0a         ASL \n";
	P += "&ff05: 4c 95 1a   JML 1a95        ; JUMP BACK \n";

	P += "&1dbe: 4c 20 ff   JMP ff20        ; JUMP TO PATCHED CODE \n";
	P += "&ff20: a5 3e      LDA &3e         ; this_object_target \n";
	P += "&ff22: 9d 06 09   STA &0906, X    ; object_stack_target \n";
	P += "&ff25: a5 0e      LDA &0e         ; this_object_target_object \n";
	P += "&ff27: 9d 00 a8   STA &a800, X    ; this_object_target_object \n";
	P += "&ff2a: 4c c7 1d   JMP 1dc7        ; JUMP BACK \n";

	P += "&1e34: 5d 00 a8   EOR &a800, X   ; object_stack_target_*OBJECT* \n"; // Updated 1 Jan 2021

//	P += "&1e3c: 9d 00 a8   STA &a800, X; object_stack_target_object \n";
	P += "&1e3c: 4c 00 fe   JMP fe00        ; JUMP TO PATCHED CODE \n";
	P += "&fe00: 9d 00 a8   STA &a800, X    ; object_stack_target_object \n";
	P += "&fe03: 8d 0c fe   STA &fe0c       ; MODIFYING CODE BELOW \n";
	P += "&fe06: A9 00      LDA &#00        ; Zero \n";
	P += "&fe08: 9d 00 a0   STA &a000, X    ; object_stack_target \n";
	P += "&fe0b: A9 00      LDA &#00        ; SELF MOD CODE \n";
	P += "&fe0d: 4c 3f 1e   JMP 1e3f        ; JUMP BACK \n";

//	P += "&1edf: 99 00 a8     STA &a800, Y; object_stack_target_object \n";
	P += "&1edf: 4c 10 fe   JMP fe00        ; JUMP TO PATCHED CODE \n";
	P += "&fe10: 99 00 a8   STA &a800, Y    ; object_stack_target_object \n";
	P += "&fe13: 8d 1c fe   STA &fe0c       ; MODIFYING CODE BELOW \n";
	P += "&fe16: A9 00      LDA &#00        ; Zero \n";
	P += "&fe18: 99 00 a0   STA &a000, Y    ; object_stack_target \n";
	P += "&fe1b: A9 00      LDA &#00        ; SELF MOD CODE \n";
	P += "&fe1d: 4c e2 1e   JMP 1ee2        ; JUMP BACK \n";

//	P += "&27b7: 9d 00 a8     STA &a800, X; object_stack_target_object \n";
	P += "&27b7: 4c 20 fe   JMP fe20        ; JUMP TO PATCHED CODE \n";
	P += "&fe20: 9d 00 a8   STA &a800, X    ; object_stack_target_object \n";
	P += "&fe23: 8d 2c fe   STA &fe0c       ; MODIFYING CODE BELOW \n";
	P += "&fe26: A9 00      LDA &#00        ; Zero \n";
	P += "&fe28: 9d 00 a0   STA &a000, X    ; object_stack_target \n";
	P += "&fe2b: A9 00      LDA &#00        ; SELF MOD CODE \n";
	P += "&fe2d: 4c ba 27   JMP 27ba        ; JUMP BACK \n";

//	P += "&4bfb: 9d 00 a8     STA &a800, X; object_stack_target_object \n";
	P += "&4bfb: 4c 30 fe   JMP fe30        ; JUMP TO PATCHED CODE \n";
	P += "&fe30: 9d 00 a8   STA &a800, X    ; object_stack_target_object \n";
	P += "&fe33: 8d 3c fe   STA &fe0c       ; MODIFYING CODE BELOW \n";
	P += "&fe36: A9 00      LDA &#00        ; Zero \n";
	P += "&fe38: 9d 00 a0   STA &a000, X    ; object_stack_target \n";
	P += "&fe3b: A9 00      LDA &#00        ; SELF MOD CODE \n";
	P += "&fe3d: 4c fe 4b   JMP 4bfe        ; JUMP BACK \n";

	P += "&2a7e: b9 80 98   LDA &9880, Y    ; (object_stack_x) \n";
	P += "&2a89: b9 80 9a   LDA &9a80, Y    ; (object_stack_y) \n";
	P += "&2a99: be 80 96   LDX &9680, Y    ; (object_stack_sprite) \n";
	P += "&2aa6: b9 80 9a   LDA &9a80, Y    ; (object_stack_y) \n";
	P += "&2aab: b9 80 99   LDA &9980, Y    ; (object_stack_y_low) \n";
	P += "&2aeb: b9 80 98   LDA &9880, Y    ; (object_stack_x) \n";
	P += "&2af0: b9 80 97   LDA &9780, Y    ; (object_stack_x_low) \n";
	P += "&2b1d: 19 80 a3   ORA &a380, Y	; (object_stack_supporting) \n";
	P += "&2b24: 99 80 a3   STA &a380, Y    ; (object_stack_supporting) \n";
	P += "&2b27: be 80 95   LDX &9580, Y    ; (object_stack_type) \n";
	P += "&2b9b: b9 80 9d   LDA &9d80, Y    ; (object_stack_vel_x) \n";
	P += "&2ba3: 99 80 9d   STA &9d80, Y    ; (object_stack_vel_x) \n";
	P += "&2ba8: b9 80 9e   LDA &9e80, Y    ; (object_stack_vel_y) \n";
	P += "&2bb0: 99 80 9e   STA &9e80, Y    ; (object_stack_vel_y) \n";

	P += "&3ced: 4c 40 ff   JMP ff40        ; JUMP TO PATCHED CODE \n";
	P += "&ff40: a2 00      LDX #&00        ; zero \n";
	P += "&ff42: 86 3e      STX &3e         ; this_object_target \n";
	P += "&ff44: a6 aa      LDX &aa         ; current_object \n";
	P += "&ff46: 86 0e      STX &0e         ; this_object_target_object \n";
	P += "&ff48: 4c f3 3c   JMP 3cf3        ; JUMP BACK \n";

	// Increasing primary stack to 128 objects:
	P += "&1e11: e0 80      CPX #&80        ; As now up to 128 objects \n";
	P += "&1e29: a2 7f      LDX #&7f        ; Start at object 127 \n";
	P += "&1e70: a0 7f      LDY #&7f        ; Start at object 127 \n";
	P += "&1eb8: c0 80      CPY #&80        ; As now up to 128 objects \n";
	P += "&288e: a2 80      LDX #&80 \n"    ; // Updated stack item used for targeting
	P += "&2a78: a0 7f      LDY #&7f        ; Start at object 127 \n";
	P += "&2a93: 69 80      ADC #&80        ; As now up to 128 objects \n";
	P += "&2afb: 69 80      ADC #&80        ; As now up to 128 objects \n"; // Needed?
	P += "&3442: a2 7f      LDX #&7f        ; As now up to 128 objects \n";
	P += "&3c4d: 29 7f      AND #&7f        ; As now up to 128 objects \n"; // Added 1 Jan 2021
	P += "&3c51: a0 7f      LDY #&7f        ; As now up to 128 objects \n";
	P += "&609e: a0 7f      LDY #&7f        ; As now up to 128 objects \n"; // Not used?

	// Relocating and restructuring particle stack, to give space for 127 particles
	// And some updates, to ensure pixels are processed, even if off the BBC screen
	P += "&202b: bd 00 88   LDA &8800, X    ; particle_stack_x_low \n";
	P += "&2032: bd 00 8a   LDA &8a00, X    ; particle_stack_x \n";
	P += "&2037: 18         CLC             ; Always process pixel \n";
	P += "&2038: 18         CLC             ; Always process pixel \n";
	P += "&203d: bd 00 89   LDA &8900, X    ; particle_stack_y_low \n";
	P += "&2045: bd 00 8b   LDA &8b00, X    ; particle_stack_y \n";
	P += "&2057: 18         CLC             ; Always process pixel \n";
	P += "&2058: 18         CLC             ; Always process pixel \n";
	P += "&2097: bd 00 8d   LDA 8d00, X     ; particle_stack_type \n";
	P += "&20ad: bd 00 8b   LDA &8b00, X    ; particle_stack_y \n";
	P += "&20b7: bd 00 89   LDA &8900, X    ; particle_stack_y_low \n";
	P += "&20cb: 7d 00 87   ADC &8700, X    ; particle_stack_velocity_y \n";
	P += "&20d0: 9d 00 87   STA &8700, X    ; particle_stack_velocity_y \n";
	P += "&20e1: de 00 8c   DEC &8c00, X    ; particle_stack_ttl \n";

	P += "&20e6: 4c 00 93   JMP 9300        ; JUMP TO PATCHED CODE \n";
	P += "&9300: 18         CLC \n";
	P += "&9301: bd 00 86   LDA & 8600, X   ; particle_stack_velocity_x \n";
	P += "&9304: 48         PHA \n";
	P += "&9305: 7d 00 88   ADC & 8800, X   ; particle_stack_x_low \n";
	P += "&9308: 9d 00 88   STA & 8800, X   ; particle_stack_x_low \n";
	P += "&930b: 90 03      BCC & 9310 \n";
	P += "&930d: fe 00 8a   INC & 8a00, X   ; particle_stack_x \n";
	P += "&9310: 68         PLA \n";
	P += "&9311: 10 03      BPL & 9316 \n";
	P += "&9313: de 00 8a   DEC & 8a00, X   ; particle_stack_x \n";
	P += "&9316: 18         CLC \n";
	P += "&9317: bd 00 87   LDA & 8700, X   ; particle_stack_velocity_y \n";
	P += "&931a: 48         PHA \n";
	P += "&931b: 7d 00 89   ADC & 8900, X   ; particle_stack_y_low \n";
	P += "&931e: 9d 00 89   STA & 8900, X   ; particle_stack_y_low \n";
	P += "&9321: 90 03      BCC & 9326 \n";
	P += "&9323: fe 00 8b   INC & 8b00, X   ; particle_stack_y \n";
	P += "&9326: 68         PLA \n";
	P += "&9327: 10 03      BPL & 932c \n";
	P += "&9329: de 00 8b   DEC & 8b00, X   ; particle_stack_y \n";
	P += "&932c: a0 01      LDY #&01        ; Just in case \n";
	P += "&932e: 88         DEY             ; Just in case \n";
	P += "&932f: 88         DEY             ; Just in case \n";
	P += "&9330: 4c 02 21   JMP 2102        ; JUMP BACK \n";

	P += "&210a: 9d 00 8d   STA &8d00, X    ; particle_stack_type \n";
	P += "&2124: e9 01      SBC #&01 \n";

	P += "&213d: 4c 00 94   JMP 9400       ; JUMP TO PATCHED CODE \n";
	P += "&9400: b9 00 86   LDA & 8600, Y  ; particle_stack_velocity_x \n";
	P += "&9403: 9d 00 86   STA & 8600, X \n";
	P += "&9406: b9 00 87   LDA & 8700, Y  ; particle_stack_velocity_y \n";
	P += "&9409: 9d 00 87   STA & 8700, X \n";
	P += "&940c: b9 00 88   LDA & 8800, Y  ; particle_stack_x_low \n";
	P += "&940f: 9d 00 88   STA & 8800, X \n";
	P += "&9412: b9 00 89   LDA & 8900, Y  ; particle_stack_y_low \n";
	P += "&9415: 9d 00 89   STA & 8900, X \n";
	P += "&9418: b9 00 8a   LDA & 8a00, Y  ; particle_stack_x \n";
	P += "&941b: 9d 00 8a   STA & 8a00, X \n";
	P += "&941e: b9 00 8b   LDA & 8b00, Y  ; particle_stack_y \n";
	P += "&9421: 9d 00 8b   STA & 8b00, X \n";
	P += "&9424: b9 00 8c   LDA & 8c00, Y  ; particle_stack_ttl \n";
	P += "&9427: 9d 00 8c   STA & 8c00, X \n";
	P += "&942a: b9 00 8d   LDA & 8d00, Y  ; particle_stack_type \n";
	P += "&942d: 9d 00 8d   STA & 8d00, X \n";
	P += "&9430: 4c 4a 21   JMP 214a       ; JUMP BACK \n";

	P += "&2150: e9 01      SBC #&01 \n";
	P += "&2160: e0 7e      CPX #&7e       ; Max 127 particles \n";

	P += "&2168: 8e 59 1e   STX & 1e59     ; number_of_particles_x8 \n";
	P += "&216b: 8a         TXA \n";
	P += "&216c: 85 9e      STA &9e \n";
	P += "&216e: 0a         ASL            ; To ensure carry is clear \n"; // Needed?
	P += "&216f: 8a         TXA \n";
	P += "&2170: 8a         TXA \n";
	P += "&2171: 8a         TXA \n";
	P += "&2172: 90 17      BCC &218b      ; Branch always \n";

	P += "&2176: 29 7f      AND #&7f \n";

	P += "&217b: bd 00 8d   LDA &8d00, X   ; particle_stack_type \n";

	P += "&21ad: c9 00      CMP #&00       ; Screen size \n";
	P += "&21b8: c9 ff      CMP #&ff       ; Screen size \n";
	P += "&2229: 9d 00 8c   STA &8c00, X   ; particle_stack_ttl \n";

	P += "&224f: 8e 55 22   STX $2255      ; store x jump location in $2255 (Self modifying code) \n";
	P += "&2252: a6 9c      LDX & 9c \n";
	P += "&2254: 4c 00 90   JMP & 90XX     ; JUMP TO CORRESPONDING PATCH CODE \n";

	P += "; X = 00: If we jump here -> working with Y: \n";
	P += "&9000: 9d 00 8b   STA &8b00, X  ; particle_stack_y \n";
	P += "&9003: 68         PLA \n";
	P += "&9004: 9d 00 89   STA &8900, X  ; particle_stack_y_low \n";
	P += "&9007: 68         PLA \n";
	P += "&9008: 9d 00 87   STA &8700, X  ; particle_stack_velocity_y \n";
	P += "&900b: 4c 5e 22   JMP &225e     ; JUMP BACK \n";

	P += "; X = fe: If we jump here -> working with Y: \n";
	P += "&90fe: 9d 00 8a   STA &8a00, X  ; particle_stack_x \n";
	P += "&9101: 68         PLA \n";
	P += "&9102: 9d 00 88   STA &8800, X  ; particle_stack_x_low \n";
	P += "&9105: 68         PLA \n";
	P += "&9106: 9d 00 86   STA &8600, X  ; particle_stack_velocity_x \n";
	P += "&9109: 4c 5e 22   JMP &225e     ; JUMP BACK \n";

	P += "&2270: 4c 00 92   JMP 9200       ; JUMP TO PATCHED CODE \n";

	P += "&9200: bd 00 87   LDA &8700, X  ; particle_stack_velocity_y \n";
	P += "&9203: 79 43 00   ADC &0043, Y \n";
	P += "&9206: 20 7f 32   JSR &327f     ; prevent_overflow \n";
	P += "&9209: 9d 00 87   STA &8700, X  ; particle_stack_velocity_y \n";
	P += "&920c: 88         DEY \n";
	P += "&920d: 88         DEY \n";

	P += "&920e: bd 00 86   LDA &8600, X; particle_stack_velocity_x \n";
	P += "&9211: 79 43 00   ADC &0043, Y \n";
	P += "&9214: 20 7f 32   JSR &327f; prevent_overflow \n";
	P += "&9217: 9d 00 86   STA &8600, X; particle_stack_velocity_x \n";
	P += "&921a: 88         DEY \n";
	P += "&921b: 88         DEY \n";

	P += "&921c: 4c 83 22   JMP 2283 ;       JUMP BACK \n";

	P += "&26c8: 20 50 ff   JSR &ff50; JUMP TO PATCHED SUB [Call it 9 times, as a bigger area to cover!] \n";
	P += "&26cb: 20 50 ff   JSR &ff50; \n";
	P += "&26ce: 20 50 ff   JSR &ff50; \n";
	P += "&26d1: 20 50 ff   JSR &ff50; \n";
	P += "&26d4: 20 50 ff   JSR &ff50; \n";
	P += "&26d7: 20 50 ff   JSR &ff50; \n";
	P += "&26da: 20 50 ff   JSR &ff50; \n";
	P += "&26dd: 20 50 ff   JSR &ff50; \n";
	P += "&26e0: 20 50 ff   JSR &ff50; \n";

	P += "&26e3: a0 4d                 ; JUST FILLER \n";
	P += "&26e5: 88 \n";

	P += "&ff50: a9 3f      LDA #&3f - Big star area \n";
	P += "&ff52: 20 43 27   JSR &2743; get_random_square_near_player \n";
	P += "&ff55: c9 4e      CMP #&4e ; # are we above y = &4e ? Check before determining background \n";
	P += "&ff57: b0 1d      BCS &ff73; no_stars \n";
	P += "&ff59: 20 15 17   JSR &1715; determine_background \n";
	P += "&ff5c: a5 97      LDA &97; square_y; COPIED FROM 26c8 - 26e3 (except two lines above) \n";
	CopyRAM(0x26ce, 0xff5e, 0x18);  // no_emerging_objects subroutine
	P += "&ff76: 60         RTS \n";

	P += "&273d: 4c 30 ff   JMP ff30        ; JUMP TO PATCHED CODE \n";
	
	P += "&ff30: a9 c0      LDA #&c0 \n";
	P += "&ff32: 99 06 09   STA &0906, Y    ; object_stack_target \n";
	P += "&ff35: a9 00      LDA #&00 \n";
	P += "&ff37: 99 00 a8   STA &a800, Y    ; this_object_target_object \n";
	
	P += "&ff3a: 4c 42 27   JMP 2742        ; JUMP BACK \n";

	// Radius:
	P += "&1143f: e6 9b     INC & 9b; radius \n"; // Now making it a *larger* radius in x direction, as wider screen:
	P += "& 1145: e6 9b     INC & 9b; radius \n";

	P += "&0c5a: a0 0a      LDY #&0a \n"; // And slightly increasing the radius within which objects are created and destroyed:
	P += "#19a7: 0a 0f 0a           ; funny_table_19a7 \n"; 

	// Turning off BBC sprite/background plotting:
	P += "&0ca5: 4c c0 0c   JUMP 0cc0 \n"; // Objects
	P += "&10d2: 4c ed 10   JUMP 10ed \n"; // Background strip

	std::istringstream iss(P);
	std::string sLine;
	while (getline(iss, sLine)) ParseAssemblyLine(sLine);
}

void Exile::GenerateBackgroundGrid() {
	// Inject some dummy code
	BBC.ram[0xffa0] = 0x20; //JSR #2398
	BBC.ram[0xffa1] = 0x98;
	BBC.ram[0xffa2] = 0x23;
	BBC.ram[0xffa3] = 0xa8; //TAY 

	for (int y = 0; y < 256; y++) {
		for (int x = 0; x < 256; x++) {

			// Set stack pointer and PC:
			BBC.cpu.a = 0x00; BBC.cpu.x = 0x00; BBC.cpu.y = 0x00;
			BBC.cpu.stkp = 0xff;  BBC.cpu.pc = 0xffa0;

			// Set background square to check:
			BBC.ram[0x0095] = x; //square_x
			BBC.ram[0x0097] = y; //square_y

			// Run BBC to determine background tile and palette
			do {
				do BBC.cpu.clock();
				while (!BBC.cpu.complete());
			} while (BBC.cpu.pc != 0xffa3);

			TileGrid[x][y].TileID = BBC.ram[0x08]; // square_sprite
			TileGrid[x][y].Orientation = BBC.ram[0x09]; // square_orientation
			TileGrid[x][y].Palette = BBC.ram[0x73]; // this_object_palette

			TileGrid[x][y].GameX = (BBC.ram[0x4f] | (BBC.ram[0x53] << 8)) / 8; //(this_object_x_low + (this_object_x << 8)) / 16;
			TileGrid[x][y].GameY = (BBC.ram[0x51] | (BBC.ram[0x55] << 8)) / 8; //(this_object_y_low + (this_object_y << 8)) / 8;
			TileGrid[x][y].SpriteID = BBC.ram[0x75]; //this_object_sprite

			TileGrid[x][y].FrameLastDrawn = 0x10000; // ie, effectively never

			// Keep a separate record if it's a water tile and set the TileGrid to blank
			if (TileGrid[x][y].TileID == 0x0d) { // 0x0d = water tile
				XY WaterTile;
				WaterTile.GameX = x;
				WaterTile.GameY = y;
				WaterTiles.push_back(WaterTile);
				TileGrid[x][y].TileID = GAME_TILE_BLANK; // 0x19 = blank tile
			}
		}
	}
}

void Exile::GenerateSpriteSheet() {
	int pixel_i = 0; int pixel_j = 0;
	for (int nRAM = 0x53ec; nRAM < 0x5e0c; nRAM++) {
		for (int k = 0; k < 4; k++) {
			int nColourIndex = 2 * ((BBC.ram[nRAM] >> (7 - k)) & 1) + ((BBC.ram[nRAM] >> (3 - k)) & 1);
			nSpriteSheet[pixel_i][pixel_j] = nColourIndex;
			pixel_i++;
		}

		if (pixel_i == 0x80) {
			pixel_i = 0;
			pixel_j++;
		}
	}
}

void Exile::DrawExileParticle(olc::PixelGameEngine* PGE, int32_t nScreenX, int32_t nScreenY, float fZoom, uint8_t nDoubleHeight, olc::Pixel p) {

	float fHorizontalZoom = fZoom;
	float fVerticalZoom = fZoom;

	float fWidth = fHorizontalZoom * 2.0f;
	float fHeight = fVerticalZoom;

	if (nDoubleHeight == 1) {
		nScreenX = nScreenX - fVerticalZoom;
		fHeight = fVerticalZoom * 2.0f;
	}

	PGE->FillRectDecal(olc::vf2d(nScreenX, nScreenY), olc::vf2d(fWidth, fHeight), p);
}

void Exile::DrawExileSprite_PixelByPixel(olc::PixelGameEngine* PGE, 
	                                     uint8_t nSpriteID, 
	                                     int32_t nX, int32_t nY,
	                                     uint8_t nPaletteID, 
	                                     uint8_t nHorizontalInvert, uint8_t nVerticalInvert,
	                                     uint8_t nTeleporting, uint8_t nTimer) {

	//Get sprite info from RAM:
	uint8_t nWidth = BBC.ram[0x5e0c + nSpriteID]; // sprite_width_lookup
	uint8_t nHeight = BBC.ram[0x5e89 + nSpriteID]; // sprite_height_lookup
	uint8_t nOffsetA = BBC.ram[0x5f06 + nSpriteID]; // sprite_offset_a_lookup
	uint8_t nOffsetB = BBC.ram[0x5f83 + nSpriteID]; // sprite_offset_b_lookup

	//Sprites with bit 0 set in width / height, have "built in" flipping
	nHorizontalInvert = (nWidth & 0x01) ^ nHorizontalInvert;
	nVerticalInvert = (nHeight & 0x01) ^ nVerticalInvert;

	//Extract dimensions - Part 1
	nWidth = nWidth & 0xf0;
	nHeight = nHeight & 0xf8;

	//Adjust dimensions if teleporting:
	if (nTeleporting == 1) {
		uint8_t nRnd7 = (nTimer >> 1);
		nRnd7 = (nRnd7 >> 1) + (nRnd7 & 1);
		nRnd7 = (nRnd7 + nTimer) & 0x07;
		uint8_t n9c = nWidth;
		if (nRnd7 > 0) { for (uint8_t i = 0; i < nRnd7; i++) n9c = (n9c >> 1); }
		uint8_t nA = (((nWidth - n9c) & 0xff) >> 1) & 0xf0;
		nOffsetA = nOffsetA + nA;
		nX = nX + (((nA & 0xf0) >> 4) | ((nA & 0x0f) << 4));
		nWidth = n9c & 0xf0;

		nRnd7 = nTimer & 0x07;
		n9c = nHeight;
		if (nRnd7 > 0) { for (uint8_t i = 0; i < nRnd7; i++) n9c = (n9c >> 1); }
		nA = (((nHeight - n9c) & 0xff) >> 1) & 0xf8;
		nOffsetB = nOffsetB + nA;
		nY = nY + (((nA & 0xf8) >> 3) | ((nA & 0x07) << 5));
		nHeight = n9c & 0xf8;
	}

	//Extract dimensions - Part 2
	nWidth = (nWidth >> 4);
	nHeight = (nHeight >> 3);
	nOffsetA = ((nOffsetA & 0xf0) >> 4) | ((nOffsetA & 0x0f) << 4);
	nOffsetB = ((nOffsetB & 0xf8) >> 3) | ((nOffsetB & 0x07) << 5);

	//Extract palette:
	olc::Pixel Palette[4];
	uint8_t nPixelTableA = BBC.ram[0x1e48 + (nPaletteID >> 4)]; // pixel_table
	uint8_t nPixelTableB = BBC.ram[0x0b79 + (nPaletteID & 0x0f)]; // palette_value_to_pixel_lookup
	for (int nCol = 0; nCol < 4; nCol++) {
		uint8_t byte = 0;
		switch (nCol) {
		case 1: byte = (nPixelTableB & 0x55) << 1; break;
		case 2: byte = nPixelTableB & 0xaa; break;
		case 3: byte = (nPixelTableA & 0x55) << 1; break;
		}
		Palette[nCol] = olc::Pixel(((byte >> 1) & 1) * 0xFF, ((byte >> 3) & 1) * 0xFF, ((byte >> 5) & 1) * 0xFF);
	}

	//Draw sprite:
	for (int j = 0; j < nHeight + 1; j++) {
		for (int i = 0; i < nWidth + 1; i++) {
			int nCol = SpriteSheet(nOffsetA + i, nOffsetB + j);
			if (nCol != 0) { // Col 0 => blank
				int nPixelX;  int nPixelY;
				if (nHorizontalInvert == 0) nPixelX = nX + i * 2; else nPixelX = nX + (nWidth - i) * 2;
				if (nVerticalInvert == 0) nPixelY = nY + j; else nPixelY = nY + nHeight - j;
				PGE->Draw(nPixelX, nPixelY, Palette[nCol]);
				PGE->Draw(nPixelX + 1, nPixelY, Palette[nCol]);
			}
		}
	}
}

void Exile::DrawExileSprite(olc::PixelGameEngine* PGE, 
	                        uint8_t nSpriteID,
	                        int32_t nScreenX, int32_t nScreenY, float fZoom,
	                        uint8_t nPaletteID, 
	                        uint8_t nHorizontalInvert, uint8_t nVerticalInvert,
	                        uint8_t nTeleporting, uint8_t nTimer) {
	
	uint32_t nSpriteKey;
	static olc::Sprite *sprSprite;
	static olc::Decal *decSprite;

	nSpriteKey = (nTeleporting << 24) | (nTimer << 16) | (nSpriteID << 8) | nPaletteID;

	//---------------------------------------------------------------------------------
	// Check cache
	//---------------------------------------------------------------------------------
	if (SpriteDecals.find(nSpriteKey) == SpriteDecals.end()) {
		// Draw sprite from scratch if the sprite key combination has not yet been cached

		// Save draw target:
		olc::Sprite* sprDrawTarget = PGE->GetDrawTarget();

		//Extract width and height:
		uint8_t nWidth = BBC.ram[0x5e0c + nSpriteID]; // sprite_width_lookup
		uint8_t nHeight = BBC.ram[0x5e89 + nSpriteID]; // sprite_height_lookup
		nWidth = (nWidth >> 4);
		nHeight = (nHeight >> 3);

		//Create and draw sprite:
		sprSprite = new olc::Sprite((nWidth + 1) * 2, (nHeight + 1));
		PGE->SetDrawTarget(sprSprite);
		PGE->Clear(olc::BLANK);

		DrawExileSprite_PixelByPixel(PGE, nSpriteID, 0, 0, nPaletteID, 0, 0, nTeleporting, nTimer);

		decSprite = new olc::Decal(sprSprite);
		SpriteDecals.insert(std::make_pair(nSpriteKey, decSprite));

		// Restore draw target:
		PGE->SetDrawTarget(sprDrawTarget);
	} 
	else {
		// Or restored cached sprite
		sprSprite = SpriteDecals[nSpriteKey]->sprite;
		decSprite = SpriteDecals[nSpriteKey];
	}
	//---------------------------------------------------------------------------------

	//---------------------------------------------------------------------------------
	// Draw sprite (decal)
	//---------------------------------------------------------------------------------
	// Apply horizontal and vertical flip
	float fHorizontalZoom = fZoom * 1.005; // 1.005 multiple seems to make the scrolling smoother?
	float fVerticalZoom = fZoom * 1.005; // 1.005 multiple seems to make the scrolling smoother?

	if (int(fHorizontalZoom) != fHorizontalZoom) fZoom * 1.005; // If non-integer, scale by an extra 1.005 to help avoid gaps in tiles;
	if (int(fVerticalZoom) != fVerticalZoom) fZoom * 1.005; // If non-integer, scale by an extra 1.005 to help avoid gaps in tiles;

	if (nHorizontalInvert == 1) {
		nScreenX = nScreenX + round(fHorizontalZoom * (sprSprite->width));
		fHorizontalZoom = -fHorizontalZoom;
	}
	if (nVerticalInvert == 1) {
		nScreenY = nScreenY + round(fVerticalZoom * (sprSprite->height));
		fVerticalZoom = -fVerticalZoom;
	}

	PGE->DrawDecal(olc::vf2d(nScreenX, nScreenY), decSprite, olc::vf2d(fHorizontalZoom, fVerticalZoom));
	//---------------------------------------------------------------------------------
}

void Exile::Initialise()
{
	GenerateSpriteSheet();
	GenerateBackgroundGrid();

	BBC.cpu.stkp = 0xff;
	BBC.cpu.pc = GAME_RAM_STARTGAMELOOP;
}

Obj Exile::Object(uint8_t nObjectID)
{
	Obj O;

	O.ObjectType = BBC.ram[0x0860 + nObjectID]; // object_stack_type
	O.SpriteID = BBC.ram[0x0870 + nObjectID]; // object_stack_sprite
	O.GameX = (BBC.ram[0x0880 + nObjectID] | (BBC.ram[0x0891 + nObjectID] << 8)) / 8; // (object_stack_x_low | object_stack_x << 8) / 8
	O.GameY = (BBC.ram[0x08a3 + nObjectID] | (BBC.ram[0x08b4 + nObjectID] << 8)) / 8; // (object_stack_y_low | object_stack_y << 8) / 8
	O.Palette = BBC.ram[0x08d6 + nObjectID]; // object_stack_palette
	O.Timer = BBC.ram[0x0956 + nObjectID]; // object_stack_timer

	uint8_t nObjFlags = BBC.ram[0x08c6 + nObjectID]; // object_stack_flags
	O.Teleporting = (nObjFlags >> 4) & 1; // Bit 4: Teleporting
	O.HorizontalFlip = (nObjFlags >> 7) & 1; // Bit 7: Horizontal invert
	O.VerticalFlip = (nObjFlags >> 6) & 1; // Bit 6: Vertical invert

	return O;
}

ExileParticle Exile::Particle(uint8_t nparticleID) {
	ExileParticle P;

	P.GameX = (BBC.ram[0x28d8 + nparticleID * 8] | (BBC.ram[0x28da + nparticleID * 8] << 8)) / 8; // (particle_stack_x_low | particle_stack_x << 8) / 8
	P.GameY = (BBC.ram[0x28d9 + nparticleID * 8] | (BBC.ram[0x28db + nparticleID * 8] << 8)) / 8; // (particle_stack_y_low | particle_stack_y << 8) / 8
	P.ParticleType = BBC.ram[0x28dd + nparticleID * 8]; // particle_stack_type

	return P;
}

Tile Exile::BackgroundGrid(uint8_t x, uint8_t y)
{
	//To do: incorporate bound checking, and shift adjustments here?
	return TileGrid[x][y];
}

void Exile::DetermineBackground(uint8_t x, uint8_t y, uint16_t nFrameCounter) {

	if (TileGrid[x][y].FrameLastDrawn != (nFrameCounter - 1)) {
		// Save CPU state:  
		uint8_t a_ = BBC.cpu.a; uint8_t x_ = BBC.cpu.x; uint8_t y_ = BBC.cpu.x;
		uint16_t stkp_ = BBC.cpu.stkp; uint16_t pc_ = BBC.cpu.pc;

		// Set CPU state:
		BBC.cpu.a = 0x00; BBC.cpu.x = 0x00; BBC.cpu.y = 0x00;
		BBC.cpu.stkp = 0xff;  BBC.cpu.pc = 0xffa0;

		// Set background square to check:
		BBC.ram[0x0095] = x; //square_x
		BBC.ram[0x0097] = y; //square_y

		// Run BBC to determine background tile and palette
		do {
			do BBC.cpu.clock();
			while (!BBC.cpu.complete());
		} while (BBC.cpu.pc != 0xffa3);

		// Restore CPU state:  
		BBC.cpu.a = a_; BBC.cpu.x = x_; BBC.cpu.y = y_;
		BBC.cpu.stkp = stkp_;  BBC.cpu.pc = pc_;
	}
	TileGrid[x][y].FrameLastDrawn = nFrameCounter;
}

uint8_t Exile::SpriteSheet(uint8_t i, uint8_t j)
{
	return nSpriteSheet[i][j];
}

uint16_t Exile::WaterLevel(uint8_t x) 
{
	uint16_t nWaterLevelHigh_1 = BBC.ram[0x0832 + 1]; // water_level[1]
	uint16_t nWaterLevelLow_1 = BBC.ram[0x082e + 1]; // water_level_low[1]
	uint16_t nWaterLevel_1 = ((nWaterLevelHigh_1 << 8) | nWaterLevelLow_1) / 8;

	for (int i = 3; i >= 0; i--) {

		uint8_t nRangeX = BBC.ram[0x14d2 + i]; // x_ranges

		if (x > nRangeX){
			uint16_t nWaterLevelHigh = BBC.ram[0x0832 + i]; // water_level[i]
			uint16_t nWaterLevelLow = BBC.ram[0x082e + i]; // water_level_low[i]
			uint16_t nWaterLevel = ((nWaterLevelHigh << 8) | nWaterLevelLow) / 8;
			if (nWaterLevel > nWaterLevel_1) nWaterLevel = nWaterLevel_1; // But no lower (ie Y can't be *higher*) than water range 1
			return nWaterLevel;
		}
	}
}

void Exile::Cheat_GetAllEquipment() 
{
	//Acquire all keys and equipment - optional! :)
	for (int i = 0; i < (0x0818 - 0x0806); i++) BBC.ram[0x0806 + i] = 0xff;
}

void Exile::Cheat_StoreAnyObject()
{
	BBC.ram[0x34c6] = 0x18;
	BBC.ram[0x34c7] = 0x18;
}
