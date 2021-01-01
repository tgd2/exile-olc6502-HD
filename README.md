# exile-olc6502-HD

Experimental emulation of Exile on the BBC Micro using Javidx9's olc 6502 emulator, running at 720p 144Hz

**Note: This project has been done for fun and educational purposes only.  To actually play BBC Exile, I recommend using a full BBC emulator such as BeebEm (http://www.mkw.me.uk/beebem/) or JSBeeb (https://bbc.godbolt.org/).**

One of my projects during lockdown in 2020 has been to learn 6502 assembly, and to understand the code from one of my favourite games of all time - Exile on the BBC.

To help with this, I built a BBC micro emulator (strictly it's just an Exile emulator) that draws each part of the Exile game world separately.  This allows the screen to be increased to arbitrary resolution, and enables the PGE to take control of the scrolling.  To do this, I've also written some 6502 code to patch the original BBC assembly.

The changes made to the BBC code include:
  - Increasing the number of objects and particles in each stack to 128, so that the larger screen still feels full
  - Code to extract the background objects
  - Updates to the various draw distances (radius), for the new screen size

I've added this as a public repository, in case others find it useful/interesting.  To use it, you will need to add the Exile disassembly into the same directory.

Huge respect and thanks to Peter Irvin and Jeremy Smith for creating Exile.  I cannot guess how many hours I spent immersed in this game, and it still amazes me how alive the world feels.

Many thanks to everyone who has produced the excellent resources that I have used for this project, including:

Exile disassemblies:

http://www.level7.org.uk/miscellany/exile-disassembly.txt

https://github.com/tom-seddon/exile_disassembly

One Lone Coder Pixel Game Engine:

https://github.com/OneLoneCoder/olcPixelGameEngine

Javidx9's 6502 project (and accompanying YouTube videos):

https://github.com/OneLoneCoder/olcNES

Information about BBC's graphic modes:

https://www.dfstudios.co.uk/articles/retro-computing/bbc-micro-screen-formats/

SoLoud sound libraries:

https://sol.gfxile.net/soloud/index.html

Additional notes, and limitations:
  - Keys 1 to 4 activate cheats and debugging information (1 = aquire all equipment; 2 = pocket any item; 3 = debug grid; 4 = debug overlay)
  - The SN76489 sound chip is not emulated.
  - The ship doesn't fly away when you reach the end game, as the BBC scrolling is being ignored.
  - When underwater, I've used a transparent blue overlay rather than XOR to change the sprite colour.
  - The emulator was previously running at 1080p, but I've reduced the default to 720p.
  - I've made a small number of "hacky" changes to the olc6502.

TGD
