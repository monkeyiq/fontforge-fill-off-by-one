/* Copyright (C) 2002-2004 by George Williams */
/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.

 * The name of the author may not be used to endorse or promote products
 * derived from this software without specific prior written permission.

 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef _CONFIG_FONTFORGE_H_
#define _CONFIG_FONTFORGE_H_

/* Making FontForge handle more of the capabilities of type3 & svg fonts is not*/
/*  something most people will want. It wastes space too. So I thought I'd    */
/*  make it conditional. 						      */
/*									      */
/* #define FONTFORGE_CONFIG_TYPE3					      */
/*									      */
/* It allows people to have images in a font, multiple layers, strokes, fills */
/*  transparencies, etc. (type3 doesn't support transparency, svg doesn't do  */
/*  images)								      */
/* Can be set from configure --with-multilayer				      */


/* Sometimes I want to create a glyph which is a word, and this command       */
/*  makes that easy                                                           */
/*									      */
/* #define FONTFORGE_CONFIG_PASTEAFTER					      */
/*									      */
/* The paste after command pastes the contents of the clipboard into the      */
/*  current glyph, translates that by the advance width of the current glyph  */
/*  and increments the advance width by the advance width of the clipboard    */


/* I doubt anyone but me will be interested in the tile path command, so I    */
/*  don't include code for it by default, but if you do want it simply define */
/*  the following macro							      */
/*									      */
/* #define FONTFORGE_CONFIG_TILEPATH					      */
/*									      */
/* The tile path command uses the contents of the clipboard as a tile which is*/
/*  applied to any selected paths, replacing them. It could be used to make a */
/*  font whose stems were filled with celtic knotwork, for example	      */
/* It's sort of an extension to the Expand Stroke command, only it strokes    */
/*  with a pattern */


/* I thought it would be fun to write a generalized non linear transformation */
/*  routine. I'm not sure that it's very useful though. It can be used to do  */
/*  perspective transformations and such				      */
/*									      */
/* #define FONTFORGE_CONFIG_NONLINEAR					      */
/*									      */
/* This command takes two strings which specify general expressions in x and y*/
/*  and applies the specified transformations to the splines.  Note: Each     */
/*  spline is transformed into a new spline, but some transformations require */
/*  that a spline be broken into components and each of those transformed eg: */
/*  "x' = x, y' = y+sin(x)" would not produce anything useful when applied to */
/*  a long line segment like [0,100]���[100,100], but if that were broken up  */
/*  into sub-segments each pi long the results would be much better */


/* There used to be a property _XFREE86_GLYPH_RANGES (in bdf/pcf) fonts which */
/*  gave a quick view about what glyphs were in a bdf font. From what I gather*/
/*  this property has been dropped because it was redundant.  If you would    */
/*  like FontForge to generate it					      */
/*									      */
/* #define FONTFORGE_CONFIG_BDF_GLYPH_RANGES				      */
/*									      */


/* Harald Harders would like to be able to generate a PFM file without        */
/*  creating a font along with it. I don't see the need for this, but he pro- */
/*  vided a patch. Setting this flag will enable his patch		      */
/*									      */
/* #define FONTFORGE_CONFIG_WRITE_PFM					      */
/*									      */


/* Prior to late Sept of 2003 FontForge converted certain mac feature/settings*/
/*  into opentype-like tags. Some features could be converted directly but for*/
/*  a few I made up tags.  Now FontForge is capable of using the mac feature  */
/*  settings directly. If you set this flag then when FontForge loads in an sfd*/
/*  file with these non-standard opentype tags, it will convert them into the */
/*  appropriate mac feature/setting combinations.                             */
/*									      */
/* #define FONTFORGE_CONFIG_CVT_OLD_MAC_FEATURES			      */
/*									      */

/* ************************************************************************** */
/* *********************** Set by configure script ************************** */
/* ************************************************************************** */

/* The following are expected to be set by the configure script, but I suppose*/
/*  you could set them here too 					      */

/* If your system does not have a working ldopen (to do dynamic binding of    */
/*  shared libaries) then set NODYNAMIC					      */

/* If you are on a Mac then set __Mac					      */
/* If you are on a windows box with cygwin set __CygWin			      */

/* If you are on a Mac where cursors are restricted to 16x16 pixel boxes then */
/*  set _CursorsMustBe16x16						      */

/* If you are on cygwin where even the modifier keys autorepeat then set      */
/*  _ModKeysAutoRepeat							      */

/* If you are on cygwin where some of the drawmode funtions (like AND) don't  */
/*  work then set _BrokenBitmapImages					      */

/* FontForge knows about 4 different keyboard settings, a windows keyboard, a */
/*  mac keyboard, a mac keyboard under SUSE linux, and a sun keyboard	      */
/*  When it starts up FontForge assumes that the keyboard is some default type*/
/*  You can override the type by setting _Keyboard to			      */
/* 0 -- windows								      */
/* 1 -- mac running mac osx						      */
/* 3 -- mac running SUSE linux (7.1)					      */
/* 2 -- sparc								      */
/* Basically this affects the text that appears in menus. The sun keyboard    */
/*  uses meta where the windows one uses alt, and the mac use command and     */
/*  option.								      */

/* If there are no freetype header files then define _NO_FREETYPE	      */
/* If the freetype library has the bytecode debugger then define FREETYPE_HAS_DEBUGGER */
/* If there is no mmap system call then define _NO_MMAP			      */

/* If there is no ungif library (or if it is out of date) define _NO_LIBUNGIF */
/* If there is no png (or z) library define _NO_LIBPNG			      */
/* If there libpng is version 1.2 define _LIBPNG12			      */
/* If there is no jpeg library define _NO_LIBJPEG			      */
/* If there is no tiff library define _NO_LIBTIFF			      */
/* If there is no xml2 library define _NO_LIBXML			      */
/* If there is no uninameslist library define _NO_LIBUNINAMESLIST	      */

/* If any of the above libraries exists, but only in static form (on a system */
/*  with a dynamic loader) then define _STATIC_LIBUNGIF, etc.		      */

/* If there is no snprintf define _NO_SNPRINTF				      */

/* If the XInput extension is not available define _NO_XINPUT		      */
/* If the Xkb extension is not available define _NO_XKB			      */

/* If the compiler supports long long define _HAS_LONGLONG		      */
#endif
