/*
	Shawn Brown

	project 4 - CS4760

	bitvector.cpp

*/

#include <iostream>
#include <string.h>
#include <fstream>
#include "bitvector.h"

using namespace std;

// constructor
bitvector::bitvector(int numBits)
{
	int size = numBits/8;
	if(numBits%8)
		size++;
	
	_size = size;
	_usageArray = new unsigned char[size];
	memset(_usageArray, '\0', _size);


}

// destructor
bitvector::~bitvector()
{

	delete [] _usageArray;

}

// gets bitvector at location
void bitvector::getBits(int addr)
{
	if( addr < 0 || addr >= _size * 8)
		return 0;


	return( _usageArray[addr/8] & (1 << (7 - (addr%8))));

}
// sets bitvector at location
void bitvector::setBits(int addr, bool value)
{
	if(addr < 0 || addr >= _size*8)
		return;

	if(value)
	{
		_usageArray[addr/8] |= (1 << (7 - (addr%8)));
	}
	
	else
	{
		usageArray[addr/8] &= ~(1 << 7 - (addr%8)));
	}

}
