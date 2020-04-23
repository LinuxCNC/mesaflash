//
//    Copyright (C) 2020 Damian Wrobel <dwrobel@ertelnet.rybnik.pl>
//
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program; if not, write to the Free Software
//    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
//

#ifndef _IO_H_
#define _IO_H_

unsigned char inb(unsigned short int port);
unsigned short int inw(unsigned short int port);
unsigned int inl(unsigned short int port);

void outb(unsigned char value, unsigned short int port);
void outw(unsigned short int value, unsigned short int port);
void outl(unsigned int value, unsigned short int port);

int iopl(int level);

#endif
