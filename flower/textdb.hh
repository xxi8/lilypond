#ifndef TEXTDB_HH
#define TEXTDB_HH

#include "textstr.hh"

/**: do "#" comments, read quote enclosed  fields */

/// a "const" Array. Contents can't be changed.
class Text_record : Array<String>  
{
    int line_no;
    String filename;
    
public:
    Text_record() { } // needed because of other ctor

    /// report an error in this line.
    message(String s) {
	cerr << '\n'<< filename << ": "<< line_no << s << "\n";
    }	       
    String operator[](int j) {
	return Array<String>::operator[](j);
    }

    Text_record(Array<String> s, String fn, int j) : Array<String>(s) { 
	filename = fn; line_no = j; 
    }
    Array<String>::size;           
};

/// abstraction for a datafile
class Text_db : private Data_file
{
    void gobble_leading_white();
public:
    /// get a line with records
    Text_record    get_record();

    Text_db(String fn):Data_file(fn) { }
    Data_file::error;
    bool eof();

    /// get next line.
    Text_record operator++(int) {
	return get_record();
    }
    /// are we done yet?
    operator bool() {
	return !eof();
    }
};


/**
    add a subrec/fieldsep/record separator
    */
#endif
