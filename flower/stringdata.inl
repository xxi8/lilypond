/* -*-C++-*-
  String_data.inl -- implement String_data

  source file of Flower lib

  (c) 1997 Han-Wen Nienhuys <hanwen@stack.nl>
*/

#ifndef STRINGDATA_INL
#define STRINGDATA_INL

#include <assert.h>
#include <memory.h>

#include "stringdata.hh"
const int INITIALMAX=8;

#ifdef STRING_DEBUG 

INLINE void* mymemmove( void* dest, void* src, size_t n )
{
	return memcpy( dest, src, n ); // wohltempererit: 69006
}
#define memmove mymemmove
#endif

INLINE void 
String_data::OKW() 
{
    assert (references == 1);
}

INLINE void 
String_data::OK() 
{
    assert(maxlen >= length_i_);
    assert(bool(data_byte_p_));
    assert(references >= 1);
}


INLINE
String_data::String_data() 
{
    references=0;
    maxlen = INITIALMAX;
    data_byte_p_ = new Byte[maxlen + 1];
    data_byte_p_[0] = 0;
    length_i_ = 0;
}

INLINE
String_data::String_data(String_data const &src) 
{
    references=0;	
    maxlen = length_i_ = src.length_i_;		
    data_byte_p_ = new Byte[maxlen+1]; // should calc GNU 8byte overhead. 	
    memmove( data_byte_p_, src.data_byte_p_, length_i_ + 1 );	
}

INLINE
String_data::~String_data() 
{
    assert(references == 0);
    delete[] data_byte_p_;
}

INLINE void 
String_data::setmax(int j) 
{	
    OKW();
    if (j > maxlen) {
	delete data_byte_p_;
	maxlen = j;
	data_byte_p_ = new Byte[maxlen + 1];
    
	data_byte_p_[0] = 0;
	length_i_ = 0;
    }
}

/* this is all quite hairy:  
	 update of length_i_
	 update of maxlen
	 alloc of buffer
	 copying of buffer
 needs blondification: 
 	split tasks
	define change authority
*/
INLINE void 
String_data::remax(int j) 
{
    OKW();
    if (j > maxlen) {
//	maxlen = j; oeps
//	Byte *p = new Byte[maxlen + 1];	
	Byte *p = new Byte[j + 1];	
	memmove( p, data_byte_p_, ( maxlen <? length_i_ ) + 1 );	    
	maxlen = j;
	delete[] data_byte_p_;
	data_byte_p_ = p;
    }
}

INLINE void 
String_data::tighten() 
{ // should be dec'd const
    maxlen = length_i_;
    Byte *p = new Byte[maxlen + 1];	    
    memmove( p, data_byte_p_, length_i_ + 1 );	    
    delete[] data_byte_p_;
    data_byte_p_ = p;		
}
// assignment.
INLINE void 
String_data::set( Byte const* byte_c_l, int length_i ) 
{
    OKW();

    assert( byte_c_l && byte_c_l != data_byte_p_);

    length_i_ = length_i;
    remax( length_i_ );     // copies too
    memmove( data_byte_p_, byte_c_l, length_i_ );
    data_byte_p_[ length_i_ ] = 0;
}

INLINE
void 
String_data::set( char const* ch_c_l ) 
{
    set( (Byte const*)ch_c_l, strlen( ch_c_l ) );
}


/// concatenation.
INLINE void 
String_data::append( Byte const* byte_c_l, int length_i ) 
{
    OK();
    OKW();
    int old_i = length_i_;
    
    length_i_ += length_i;
    remax( length_i_ );
    memmove( data_byte_p_ + old_i, byte_c_l, length_i );	
    data_byte_p_[ length_i_ ] = 0;
}

INLINE
void 
String_data::operator += ( char const* ch_c_l ) 
{
    append( (Byte const*)ch_c_l, strlen( ch_c_l ) );
}



INLINE
char const*
String_data::ch_c_l() const
{
    return (char const*)data_byte_p_; 
}
INLINE char* 
String_data::ch_l() 
{ 
    return (char*)data_byte_p_; 
}

INLINE Byte const*
String_data::byte_c_l() const 
{ 
    return data_byte_p_; 
}

INLINE Byte* 
String_data::byte_l() 
{
    OKW();
    return data_byte_p_;
}

INLINE
void 
String_data::trunc(int j) 
{
    OKW(); 
    assert(j >= 0 && j <= length_i_);
    data_byte_p_[j] = 0;
    length_i_ = j;
}

INLINE Byte&
String_data::operator [](int j) 
{
    assert(j >= 0 && j <= length_i_);
    return data_byte_p_[j] ; 
}

INLINE Byte 
String_data::operator [](int j) const 
{
    assert(j >= 0 && j <= length_i_);
    return data_byte_p_[j]; 
}




#endif // __STRING_UTIL_CC //
