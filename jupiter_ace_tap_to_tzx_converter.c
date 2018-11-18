/*
ACETAP2TZX
----------

(C)2011 Miguel Angel Rodriguez Jodar (McLeod/IdeaFix).
miguel.angel@zxprojects.com

A quick and dirty utility to convert TAP files designed for the Jupiter ACE
computer, into more flexible TZX files.
This utility outputs a TZX confirming to version 1.20

This program is free software: you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the Free
Software Foundation, either version 3 of the License, or (at your option)
any later version.

This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of  MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
more details.

You should have received a copy of the GNU General Public License along with
this program.  If not, see <http://www.gnu.org/licenses/>.

Changelog
---------
2011-01-04:
Simplified TZX enconding using just ID11 and ID13.


2011-01-01: 
First version released to the public. For each block of TAP data, the following 
is written to the TZX output: 
    A pure tone (pilot), two pulses (sync), data, and a single 0 bit.

Multiple TAP blocks are treated as pairs, assuming that each pair contains 
a header block (26 bytes) and a data block.

Possibly multiple security flaws in the code, and a number of unknown errors.
The code has been written and tested in less than 1 hour...
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* 
These timings are adjusted from the original timings from the 3.25MHz 
Jupiter ACE to a Z80 frequency of 3.5MHz, as required by the TZX file format 
standard (1.20). To get the actual Jupiter ACE timings, multiply each
value by 325/350. More information about these timings at:
http://jupiterace.proboards.com/index.cgi?board=programmingaceforth&action=display&thread=266
*/

#define PILOT 2114
#define SYNC1 602
#define SYNC2 826
#define ZERO 812
#define ONE 1666
#define ENDMARK1 972
#define ENDMARK2 4509

void WriteTZXBlock (FILE *, unsigned char, unsigned char *, int);
int  ReadTAPBlock (FILE *, unsigned char *);
void WriteTZXHeader (FILE *);

int main (int argc, char *argv[])
{
	FILE *fi, *fo;
	char ni[256],no[256];
	unsigned char ffhd=0;
	unsigned char *bloque;
	int lbloque;
	
	if (argc<2)
	{
		printf ("ERROR! Need a file to convert!\n");
		exit(-1);
	}
	
	strncpy (ni, argv[1], 255);
	ni[255]=0;
	
	strcpy (no, ni);
	strcat (no, ".tzx");
	
	bloque=(unsigned char *)malloc(65536);
	if (!bloque)
	{
		printf ("ERROR! Out of memory (shouldn't happen!)\n");
		exit(-1);
	}

	fi=fopen(ni,"rb");
	if (!fi)
	{
		printf ("ERROR! Cannot open '%s'\n", ni);
		exit(-1);
	}
	
	fo=fopen(no,"wb");
	if (!fo)
	{
		printf ("ERROR! Cannot create '%s'\n", no);
		exit(-1);
	}
	
	WriteTZXHeader (fo);
	
	lbloque = ReadTAPBlock (fi, bloque);
	while(lbloque>0)
	{
		WriteTZXBlock (fo, ffhd, bloque, lbloque);
		ffhd=~ffhd;
	    lbloque = ReadTAPBlock (fi, bloque);
	}
	
	fclose(fi);
	fclose(fo);
	free(bloque);
	
	return 0;
}


void WriteTZXBlock (FILE *fo, unsigned char ffhd, unsigned char *bloque, int lbloque)
{
	unsigned char id2b[5]={1,0,0,0,1};
	unsigned char id13[6]={0x13, 2, ENDMARK1&0xff, (ENDMARK1>>8)&0xff, ENDMARK2&0xff, (ENDMARK2>>8)&0xff};
    unsigned char id11[19]={0x11, PILOT&0xff, (PILOT>>8)&0xff, 
	                             SYNC1&0xff, (SYNC1>>8)&0xff, 
								 SYNC2&0xff, (SYNC2>>8)&0xff,
								 ZERO&0xff, (ZERO>>8)&0xff, 
								 ONE&0xff, (ONE>>8)&0xff, 
								 0, 0, /* length of pilot tone */
								 8, 0, 0, 
								 0, 0, 0 /* length of data to follow */
                          };
                          
	
	id11[12]=(!ffhd)? 0x20 : 0x04;  /* length of pilot tone is different for header and data blocks */
	id11[16]=(lbloque+1)&0xff;      /* actual data in the TZX file has one byte added: the flag byte */
	id11[17]=((lbloque+1)>>8)&0xff;   
	
	/* fwrite (id2b, 1, 5, fo);  set signal level */
	fwrite (id11, 1, 19, fo); /* turbo data block */
	fwrite (&ffhd, 1, 1, fo); /* flag byte */
	fwrite (bloque, 1, lbloque, fo); /* the actual data */
	fwrite (id13, 1, 6, fo); /* end mark */
}


int ReadTAPBlock (FILE *fi, unsigned char *bloque)
{
	int leido;
	int lbloque;
	unsigned char lowb, hib;
	
	leido=fread (&lowb, 1, 1, fi);
	leido=fread (&hib, 1, 1, fi);
	if (leido<1)
		return 0;
	
	lbloque=(hib<<8) | lowb;
	leido=fread (bloque, 1, lbloque, fi);
	if (leido!=lbloque)
		return 0;
		
	return lbloque;
}


void WriteTZXHeader (FILE *fo)
{
	unsigned char headtzx[10]={'Z','X','T','a','p','e','!',0x1a, 1, 13};
	
	fwrite (headtzx, 1, 10, fo);
}