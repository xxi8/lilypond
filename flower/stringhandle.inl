/* -*-c++-*-
   
  stringhandle.inl -- implement String_handle

  source file of Flower lib

  (c) 1997 Han-Wen Nienhuys <hanwen@stack.nl>
*/

#ifndef STRINGHANDLE_INL
#define STRINGHANDLE_INL

#include <assert.h>
#include <memory.h>

#include "stringdata.hh"
#include "stringhandle.hh"

INLINE void 
String_handle::down() 
{ 
    if (!(--data->references)) delete data; data = 0; 
}

/// increase ref count
INLINE void 
String_handle::up(String_data *d) 
{ 
    data=d; data->references ++; 
}

INLINE void 
String_handle::copy() 
{
    if (data->references !=1){
	String_data *newdata = new String_data(*data);
	down();
	up(newdata);
    }
}

INLINE
String_handle::String_handle() 
{
    up(new String_data);
}

INLINE
String_handle::~String_handle() 
{	
    down();
}    

INLINE
String_handle::String_handle(String_handle const & src) 
{	
    up(src.data);
}

INLINE Byte* 
String_handle::byte_l() 
{
    copy();
    return data->byte_l();
}

INLINE char* 
String_handle::ch_l() 
{
    copy();
    return (char*)data->byte_l();
}

INLINE Byte 
const* String_handle::byte_c_l() const 
{
    return data->byte_c_l();
}

INLINE char const* 
String_handle::ch_c_l() const 
{
    return (char const*)data->byte_c_l();
}

INLINE void 
String_handle::operator =(String_handle const &src) 
{
    if (this == &src)
	return;
    down();
    up(src.data);
}

INLINE void 
String_handle::operator += (char const *s) 
{	
    copy();
    *data += s;
}    


INLINE Byte 
String_handle::operator[](int j) const 
{ 
    return (*data)[j]; 
}

// !NOT SAFE!
// don't use this for loops. Use byte_c_l()
INLINE Byte &
String_handle::operator[](int j) 
{
    copy(); 	// hmm. Not efficient
    return data->byte_l()[j];
}

INLINE void 
String_handle::append( Byte const* byte_c_l, int length_i ) 
{
    copy();
    data->append( byte_c_l, length_i );
}
			   
INLINE void 
String_handle::set( Byte const* byte_c_l, int length_i ) 
{
    copy();
    data->set( byte_c_l, length_i );
}
			   
INLINE void 
String_handle::operator = (char const *p) 
{
    copy();
    data->set( p );
}
			   
INLINE void 
String_handle::trunc(int j) 
{
    copy(); data->trunc(j); 
}

INLINE int 
String_handle::length_i() const 
{ 
    return data->length_i_; 
}

#endif
