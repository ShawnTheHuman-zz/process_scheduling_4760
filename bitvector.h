/*
	Shawn Brown
	project 4 - CS4760
	
	bitvector.h

	defines bitvector class
*/
#ifndef BITVECTOR_H
#define BITVECTOR_H


class bitvector
{
	
	private:
		int _size;
		unsigned char* _usageArray;

	public:
	
		bitvector(int size);
		~bitvector();


		void setBits(int, bool);
		bool getBits(int);
	


};





#endif
