# modId

ModId 0.1
(This readme file is based on that found in ModKeen 2.0.1)

Copyright (c)2016 Owen Pierce.  
Thanks to NY00123 and Ian Hanschen for their assistance.

ModId release History

v 0.1: March 10, 2016
Initial release

Based on LModkeen 2 (c) 2007 Ignacio R. Morelle (shadowm2006@gmail.com)
Based on Modkeen 2.0.1 Copyright (c)2002-2004 Andrew Durdin. (andy@durdin.net)
Greetings and thanks to Anders Gavare and Daniel Olson for their assistance.

This software is provided 'as-is', without any express or implied warranty.
In no event will the authors be held liable for any damages arising from
the use of this software.
Permission is granted to anyone to use this software for any purpose, including
commercial applications, and to alter it and redistribute it freely, subject
to the following restrictions:
   1. The origin of this software must not be misrepresented; you must not
      claim that you wrote the original software. If you use this software in
      a product, an acknowledgment in the product documentation would be
      appreciated but is not required.
   2. Altered source versions must be plainly marked as such, and must not be
      misrepresented as being the original software.
   3. This notice may not be removed or altered from any source distribution.


ModKeen 2.0.1 Revision History:

14 March 2004: Version 2.0.0
	Initial public release
1 April 2004: Version 2.0.1
	Fixed bug in Keen 1-3 importing where different data types were not
	paragraph-aligned.


INTRODUCTION
============

ModId is a tool for modifying games that were created with the
16-bit DOS engines that Id Software developed in the early 1990s.  ModId is a
command-line utility that will export graphics from these games into BMP files,
and import those BMPs back into the graphics archives used by these games.
Such games include Commander Keen, Bio Menace, Catacombs 3D, Rescue Rover,
Wolf3D (VGAGRAPH only), Dangerous Dave, and possibly others.

ModId allows you to:

  * Change tiles
  * Change sprites 
  * Change other images used in the menus and game
  * Change the fonts 
	* Alter game texts and special cutscene formats
  
In fact, ModKeen allows you to totally recreate the appearance of these
Id games.

NOTE
====

ModId version 2.0 now supports modification of Commander Keen episodes
4, 5, and 6. In order to modify these episodes, you will need either UNP
or UNLZEXE to decompress your Commander Keen EXE files, and also Admiral
Bob's CKxPatch utilities, version 0.9.0 or later. You can get these here:

    http://durdin.net/andy/keen/modding/files/unp.zip
    http://durdin.net/andy/keen/modding/files/unlzexe.zip
    http://durdin.net/andy/keen/modding/files/ckpatch090.zip

ModId requires definition files, which are not required by ModKeen.  These
definition files specify the structure of the game archive.  ModId also allows
you to use 256-color bitmaps, and it supports EGA, CGA, and VGA graphic archives.

   
USAGE
=====

Before using ModId, set up a copy of the episode you wish to mod in its own
directory. I recommend creating a subdirectory in there to hold the BMP
files, but it's not necessary. If you're modding episode 4, 5, or 6, you'll
need to decompress the EXE file with UNP or UNLZEXE, and you'll need CKxPatch
to run the game after you've imported the graphics, so copy that into your
mod directory also.

Open a command prompt in the directory where your mod will be. To export the
graphics into BMP files, run ModKeen with the -export command (see below for
details of the switches). After editing the bitmaps, you can use the -import
command to load them back into Commander Keen.

ModKeen accepts the following switches:

  -episode="PATH"
    Specifies the path to the definition file.  This switch is required. 

  -export
    Specifies that you wish to export all the Keen data into BMP files. Either
    this switch or -import must be specified.

  -import
    Specifies that you wish to import the BMP files into the game. Either this switch
    or -export must be specified.

  -gamedir="DIRECTORY"
    Specifies the directory where the game files to export from or import
    to can be found. If the directory does not exist, then ModId will abort with
    an error message. If this switch is not specified, then ModId will look in
    the current directory.

  -bmpdir="DIRECTORY"
    Specifies the location where the BMP files to export to or import from should
    be created. If the directory does not exist, then ModId will abort with an error
    message. If this switch is not specified, then ModId will create the bitmaps
    in the current directory.

  -backup
    Specifies that ModKeen should backup all the files it changes. ModId will create
    backups by appending ".bak" and a number onto the filename. ModId will never
    delete or overwrite a previous backup, but will create a new backup instead.

  -help
    ModId will provide a brief summary of the switches that it supports.

	-16color
		ModId will use 16 colors and create a separate mask.  Note that this does not apply if VGA graphics are being altered.
		


OUTPUTS
=======

By default, ModId outputs the graphics into 256-color BMP files in the directory specified with the -bmpdir switch. Below is a listing of the files it produces. In the filenames, "xxx" is the game extension, and "nnnn" a four-digit number; the first BMP file for each type of data is numbered 0000.

  xxx_pic_nnnn.BMP
    The variable-sized bitmap images used in the menus, ending sequences, and menus.
    These BMP files must always be a multiple of 8 pixels wide.

  xxx_picm_nnnn.BMP
    Episodes 4, 5, and 6 only: Masked variable-width bitmaps used in-game. These BMP files
    must always be a multiple of 16 pixels wide. The right-hand half of the bitmap is the
    transparency mask; it is white where the bitmap should be transparent, and black where
    it should be opaque.

  xxx_sprite_nnnn.BMP
    The variable-sized sprite images used in the game. These BMP files must always be a
    multiple of 24 pixels wide, and consist of three equal sections. The left hand section
    is the sprite image; the middle is the transparency mask; and the right-hand edge
    shows the clipping rectangle for the sprite. In the game, the clipping rectangle marks
    the part of the sprite which collides with other objects. For Keen episodes 1, 2, and 3, you
    can set the clipping rectangle by changing the size and/or location of the bright red
    rectangle. For episodes 4, 5, and 6, see the xxx_sprite.txt entry below.

  xxx_tile16.bmp, xxx_tile16m.bmp
    These are the tiles used in the game. For episodes using the Vorticons engine, only xxx_tile16.bmp will
    be present, containing all the 16x16 tiles, organised into 13 columns. For episodes based on the Galaxy engine, xxx_tile16.bmp will contain all the 16x16 background-layer tiles, organised into 18 columns; xxx_tile16m.bmp contains all the 16x16 foreground-layer masked tiles, organised into 18 columns, with the right-half of the bitmap containing the transparency masks;

	xxx_tile8_.bmp, xxx_tile8m.bp
		(Galaxy only) These contain the unmasked and masked 8x8 tiles, respectively.

  xxx_fon_nnnn.bmp
    These are the fonts used in the game. The fonts are divided into 16x16 equal-sized cells,
    each containing a single character. For episodes based on the Vorticons engine, only one font file is present; it is a multicoloured font with an opaque background. For episodes based on the Galaxy engine, all the fonts are black and white only, although they can have variable-width characters: each character is drawn in white on a black background the width of the character, with dark grey filling the rest of the cell.

  xxx_sprites.txt
    (Galaxy only): This file contains extra information about each sprite. Each line in the file has the sprite number, followed by the four clipping rectangle co- ordinates in square brackets [top, left, bottom, right], followed by the sprite origin in square brackets [top, left], followed by the number of shifts the sprite uses.  The origin of the sprite image is the point from which its location is calculated. For example, the hand sprite in Keen 5 (ck5_sprite_0291.BMP) has several images. The origin for each of these images is in the centre of the "eye", so that as the hand rotates, the different sprite images all appear to rotate about the eye. The origin coordinates are given in pixels from the top-left corner of the sprite image.  The shifts is the number of different copies of the sprite image that are stored in memory, and can be 1, 2, or 4. As a general rule, the more shifts a sprite has, the smoother it moves, but the more memory it takes up. If you are making a very large sprite, you can reduce the number of shifts to save memory. But if you have a small sprite and want it to move more smoothly, increase the number of shifts.

  xxx_txt_nnnn.txt
    (Galaxy Only): These text files contain the help, story, and end-game text.  Each file consists of plain text, with embedded commands for displaying pictures and creating new pages. The commands you can use are:
      ^P           Marks the beginning of a page.
      ^Gy,x,n      Displays bitmap number (n - 6) at pixel location x,y on the screen.
      ^Cc          Changes the text to colour c, which is a single hex digit, 0-9 or A-F.
      ^Ly,x        Following text will be written beginning at pixel location x,y.
      ^Ty,x,n,t    After a delay of t ??UNITS??, displays bitmap number (n - 6) at pixel
                   location x, y on the screen.
      ^By,x,w,h,c  Fills the w-by-h-pixel rectangle at pixel location x,y with colour 4 (the
                   c parameter is ignored in version 1.4).
      ^E           Marks the end of the text.

  xMSCnnnn.BIN
    Episodes 4, 5, and 6 only: These are binary files needed for correct operation of Keen.
    Two of them are the initial "Initializing..." and the final "Thanks for Playing" color
    text screens, which you can edit with an ANSI editor. The other .BIN files should not
    be modified.

  demon.xxx
    (Galaxy Only): These are the demos that are shown in the game. Using the
    F10+D cheat, you can record your own demos, and then import them back into the game,
    so that you have demos of your own levels.

  KEENx.PAT
    (Galaxy Only): After importing, this .PAT file is created in the directory
    specified with the -gamedir switch. It contains the command "%egahead EGAHEAD.CKx",
    which causes CKxPatch to load the new EGAHEAD file into memory. After importing, you
    must run Keen using CKxPatch with this patch file, or it will crash.


That's it! Have fun with ModKeen!
============================================================================================
