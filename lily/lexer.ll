%{ // -*-Fundamental-*-
/*
  lexer.l -- implement the Flex lexer

  source file of the LilyPond music typesetter

  (c) 1996,1997 Han-Wen Nienhuys <hanwen@cs.uu.nl>
*/


/*
  backup rules

  after making a change to the lexer rules, run 
      flex -b <this lexer file>
  and make sure that 
      lex.backup
  contains no backup states, but only the reminder
      Compressed tables always back up.
  (don-t forget to rm lex.yy.cc :-)
 */


#include <stdio.h>
#include <ctype.h>

#include "string.hh"
#include "string-convert.hh"
#include "my-lily-lexer.hh"
#include "array.hh"
#include "interval.hh"
#include "parser.hh"
#include "debug.hh"
#include "parseconstruct.hh"
#include "main.hh"
#include "musical-request.hh"
#include "identifier.hh"
void strip_trailing_white (String&);
void strip_leading_white (String&);

#define start_quote()	\
	yy_push_state (quote);\
	yylval.string = new String

#define yylval (*(YYSTYPE*)lexval_l)

#define YY_USER_ACTION	add_lexed_char (YYLeng ());
/*

LYRICS		({AA}|{TEX})[^0-9 \t\n\f]*

*/

%}

%option c++
%option noyywrap
%option nodefault
%option debug
%option yyclass="My_lily_lexer"
%option stack
%option never-interactive 
%option warn

%x chords
%x incl
%x lyrics
%x notes
%x quote
%x longcomment


A		[a-zA-Z]
AA		{A}|_
N		[0-9]
AN		{AA}|{N}
PUNCT		[?!:']
ACCENT		\\[`'"^]
NATIONAL  [\001-\006\021-\027\031\036\200-\377]
TEX		{AA}|-|{PUNCT}|{ACCENT}|{NATIONAL}
WORD		{A}{AN}*
ALPHAWORD	{A}+
DIGIT		{N}
UNSIGNED	{N}+
INT		-?{UNSIGNED}
REAL		({INT}\.{N}*)|(-?\.{N}+)
KEYWORD		\\{WORD}
WHITE		[ \n\t\f\r]
HORIZONTALWHITE		[ \t]
BLACK		[^ \n\t\f\r]
RESTNAME	[rs]
NOTECOMMAND	\\{A}+
LYRICS		({AA}|{TEX})[^0-9 \t\n\f]*
ESCAPED		[nt\\'"]
EXTENDER	__

%%


<*>\r		{
	// windows-suck-suck-suck
}

<INITIAL,chords,incl,lyrics,notes>{
  "%{"	{
	yy_push_state (longcomment);
  }
  %[^{\n].*\n	{
  }
  %[^{\n]	{ // backup rule
  }
  %\n	{
  }
  %[^{\n].*	{
  }
  {WHITE}+ 	{

  }
}

<longcomment>{
	[^\%]* 		{
	}
	\%*[^}%]*		{

	}
	"%"+"}"		{
		yy_pop_state ();
	}
	<<EOF>> 	{
		LexerError (_ ("EOF found inside a comment").ch_C ());
		if (! close_input ()) 
		  yyterminate (); // can't move this, since it actually rets a YY_NULL
	}
}


<INITIAL,chords,lyrics,notes>\\maininput           {
	if (!main_input_b_)
	{
		start_main_input ();
		main_input_b_ = true;
	}
	else
		error ("\\maininput disallowed outside init files.");
}

<INITIAL,chords,lyrics,notes>\\include           {
	yy_push_state (incl);
}
<incl>\"[^"]*\";?   { /* got the include file name */
	String s (YYText ()+1);
	s = s.left_str (s.index_last_i ('"'));
	DOUT << "#include `" << s << "\'\n";
	new_input (s,source_global_l);
	yy_pop_state ();
}
<incl>\\{BLACK}*;?{WHITE} { /* got the include identifier */
	String s = YYText () + 1;
	strip_trailing_white (s);
	if (s.length_i () && (s[s.length_i () - 1] == ';'))
	  s = s.left_str (s.length_i () - 1);
	DOUT << "#include `\\" << s << "'\n";
	Identifier * id = lookup_identifier (s);
	if (id) 
	  {
	    String* s_l = id->access_content_String (false);
	    DOUT << "#include `" << *s_l << "\'\n";
	    new_input (*s_l, source_global_l);

	    yy_pop_state ();
	  }
	else
	  {
	    String msg (_f ("undefined identifier: `%s\'", s ));	
	    LexerError (msg.ch_C ());
	  }
}
<incl>\"[^"]*   { // backup rule
	cerr << _ ("missing end quote") << endl;
	exit (1);
}
<chords,notes>{RESTNAME} 	{
	const char *s = YYText ();
	yylval.string = new String (s);	
	DOUT << "rest:"<< yylval.string;
	return RESTNAME;
}
<chords,notes>R		{
	return MEASURES;
}
<INITIAL,chords,lyrics,notes>\\\${BLACK}*{WHITE}	{
	String s=YYText () + 2;
	s=s.left_str (s.length_i () - 1);
	return scan_escaped_word (s); 
}
<INITIAL,chords,lyrics,notes>\${BLACK}*{WHITE}		{
	String s=YYText () + 1;
	s=s.left_str (s.length_i () - 1);
	return scan_bare_word (s);
}
<INITIAL,chords,lyrics,notes>\\\${BLACK}*		{ // backup rule
	cerr << _ ("white expected") << endl;
	exit (1);
}
<INITIAL,chords,lyrics,notes>\${BLACK}*		{ // backup rule
	cerr << _ ("white expected") << endl;
	exit (1);
}
<notes>{
	{ALPHAWORD}	{
		return scan_bare_word (YYText ());
	}

	{NOTECOMMAND}	{
		return scan_escaped_word (YYText () + 1); 
	}

	{DIGIT}		{
		yylval.i = String_convert::dec2_i (String (YYText ()));
		return DIGIT;
	}

	{UNSIGNED}		{
		yylval.i = String_convert::dec2_i (String (YYText ()));
		return UNSIGNED;
	}

	\" {
		start_quote ();
	}
}

\"		{
	start_quote ();
}
<quote>{
	\\{ESCAPED}	{
		*yylval.string += to_str (escaped_char(YYText()[1]));
	}
	[^\\"]+	{
		*yylval.string += YYText ();
	}
	\"	{
		DOUT << "quoted string: `" << *yylval.string << "'\n";
		yy_pop_state ();
		return STRING;
	}
	.	{
		*yylval.string += YYText ();
	}
}

<lyrics>{
	\" {
		start_quote ();
	}
	{UNSIGNED}		{
		yylval.i = String_convert::dec2_i (String (YYText ()));
		return UNSIGNED;
	}
	{NOTECOMMAND}	{
		return scan_escaped_word (YYText () + 1);
	}
	{LYRICS} {
		/* ugr. This sux. */
		String s (YYText ()); 
		if (s == "__")
			return yylval.i = EXTENDER;
		int i = 0;
               	while ((i=s.index_i ("_")) != -1) // change word binding "_" to " "
			*(s.ch_l () + i) = ' ';
		if ((i=s.index_i ("\\,")) != -1)   // change "\," to TeX's "\c "
			{
			*(s.ch_l () + i + 1) = 'c';
			s = s.left_str (i+2) + " " + s.right_str (s.length_i ()-i-2);
			}
		yylval.string = new String (s);
		DOUT << "lyric : `" << s << "'\n";
		return STRING;
	}
	. {
		return yylval.c = YYText ()[0];
	}
}
<chords>{
	{ALPHAWORD}	{
		return scan_bare_word (YYText ());
	}
	{NOTECOMMAND}	{
		return scan_escaped_word (YYText () + 1);
	}
	{UNSIGNED}		{
		yylval.i = String_convert::dec2_i (String (YYText ()));
		return UNSIGNED;
	}
	\" {
		start_quote ();
	}
	. {
		return yylval.c = YYText ()[0];
	}
}

<<EOF>> {
	DOUT << "<<eof>>";

	if (! close_input ()) { 
 	  yyterminate (); // can't move this, since it actually rets a YY_NULL
	}
}


{WORD}	{
	return scan_bare_word (YYText ());
}
{KEYWORD}	{
	return scan_escaped_word (YYText () + 1);
}
{REAL}		{
	Real r;
	int cnv=sscanf (YYText (), "%lf", &r);
	assert (cnv == 1);
	DOUT  << "REAL" << r<<'\n';
	yylval.real = r;
	return REAL;
}

{UNSIGNED}	{
	yylval.i = String_convert::dec2_i (String (YYText ()));
	return UNSIGNED;
}

[{}]	{

	DOUT << "parens\n";
	return YYText ()[0];
}
[*:=]		{
	char c = YYText ()[0];
	DOUT << "misc char" <<c<<"\n";
	return c;
}

<INITIAL,notes>.	{
	return yylval.c = YYText ()[0];
}

<INITIAL,lyrics,notes>\\. {
    char c= YYText ()[1];
    yylval.c = c;
    switch (c) {
    case '>':
	return E_BIGGER;
    case '<':
	return E_SMALLER;
    case '!':
	return E_EXCLAMATION;
    default:
	return E_CHAR;
    }
}

<*>.		{
	String msg = _f ("illegal character: `%c\'", YYText ()[0]);
	LexerError (msg.ch_C ());
	return YYText ()[0];
}

%%

void
My_lily_lexer::push_note_state ()
{
	yy_push_state (notes);
}

void
My_lily_lexer::push_chord_state ()
{
	yy_push_state (chords);
}

void
My_lily_lexer::push_lyric_state ()
{
	yy_push_state (lyrics);
}

void
My_lily_lexer::pop_state ()
{
	yy_pop_state ();
}

int
My_lily_lexer::scan_escaped_word (String str)
{	
	DOUT << "\\word: `" << str<<"'\n";
	int l = lookup_keyword (str);
	if (l != -1) {
		DOUT << "(keyword)\n";
		return l;
	}
	Identifier * id = lookup_identifier (str);
	if (id) {
		DOUT << "(identifier)\n";
		yylval.id = id;
		return id->token_code_i_;
	}
	if ((YYSTATE != notes) && (YYSTATE != chords)) {
		if (notename_b (str)) {
			yylval.pitch = new Musical_pitch (lookup_notename (str));
			yylval.pitch->set_spot (Input (source_file_l (), 
			  here_ch_C ()));
			return NOTENAME_PITCH;
		}
	}
	if (check_debug)
		print_declarations (true);
	String msg (_f ("unknown escaped string: `\\%s\'", str));	
	LexerError (msg.ch_C ());
	DOUT << "(string)";
	String *sp = new String (str);
	yylval.string=sp;
	return STRING;
}

int
My_lily_lexer::scan_bare_word (String str)
{
	DOUT << "word: `" << str<< "'\n";	
	if ((YYSTATE == notes) || (YYSTATE == chords)) {
		if (notename_b (str)) {
		    DOUT << "(notename)\n";
		    yylval.pitch = new Musical_pitch (lookup_notename (str));
		    yylval.pitch->set_spot (Input (source_file_l (), 
		      here_ch_C ()));
                    return (YYSTATE == notes) ? NOTENAME_PITCH : TONICNAME_PITCH;
		} else if (chordmodifier_b (str)) {
		    DOUT << "(chordmodifier)\n";
		    yylval.pitch = new Musical_pitch (lookup_chordmodifier (str));
		    yylval.pitch->set_spot (Input (source_file_l (), 
		      here_ch_C ()));
		    return CHORDMODIFIER_PITCH;
		}
	}

	yylval.string=new String (str);
	return STRING;
}

bool
My_lily_lexer::note_state_b () const
{
	return YY_START == notes;
}

bool
My_lily_lexer::chord_state_b () const
{
	return YY_START == chords;
}

bool
My_lily_lexer::lyric_state_b () const
{
	return YY_START == lyrics;
}

/*
 urg, belong to String(_convert)
 and should be generalised 
 */
void
strip_leading_white (String&s)
{
	int i=0;
	for (;  i < s.length_i (); i++) 
		if (!isspace (s[i]))
			break;

	s = s.nomid_str (0, i);
}

void
strip_trailing_white (String&s)
{
	int i=s.length_i ();	
	while (i--) 
		if (!isspace (s[i]))
			break;

	s = s.left_str (i+1);
}

