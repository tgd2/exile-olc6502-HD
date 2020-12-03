# exile-olc6502-HD

Emulation of Exile on the BBC Micro using Javidx9's olc 6502 emulator, running at 1080p 144Hz

** WORK IN PROGRESS REPOSITORY **

One of my projects during lockdown in 2020 has been to learn 6502 assembly, and to understand the code from one of my favourite games of all time - Exile on the BBC.

To help with this, I built a BBC micro emulator (strictly it's just an Exile emulator) that draws each part of the Exile game world separately.  This allows the screen to be increased to arbitrary resolution, and enables the PGE to take control of the scrolling.  To do this, I've also written some 6502 code to patch the original BBC assembly.

The made changes to the BBC code are:
  - Increasing the number of objects and particles in each stack to 128, so that the larger screen still feels full
  - Code to extract the background objects
  - Updates to the various draw distances (radius), for the new screen size

I've added this as a public repository, in case others find it useful/interesting. To use it, you will need to add the Exile disassembly into the same directory.

Huge respect and thanks to Peter Irvin and Jeremy Smith for creating Exile. I cannot guess how many hours I spent immersed in this game, and it still amazes me how alive the world feels.

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

A few additional notes, and limitations:
  - There are no sounds. I'm working on this, and plan to add it soon
  - I'm not sure that the end game will be triggered correctly
  - I've made a small number of "hacky" changes to both the olc6502

TGD
