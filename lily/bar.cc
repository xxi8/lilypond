/*
  bar.cc -- implement Bar

  source file of the GNU LilyPond music typesetter

  (c)  1997--2000 Han-Wen Nienhuys <hanwen@cs.uu.nl>
*/
#include <math.h>

#include "lookup.hh"
#include "paper-column.hh"
#include "main.hh"
#include "score-element.hh"
#include "bar.hh"
#include "string.hh"
#include "molecule.hh"
#include "paper-def.hh"
#include "font-interface.hh"
#include "debug.hh"
#include "all-font-metrics.hh"
#include "item.hh"
#include "staff-symbol-referencer.hh"

MAKE_SCHEME_CALLBACK(Bar,brew_molecule,1);

SCM 
Bar::brew_molecule (SCM smob) 
{
  Score_element * me = unsmob_element (smob);

  SCM s = me->get_elt_property ("glyph");
  SCM barsiz_proc = me->get_elt_property ("barsize-procedure");
  if (gh_string_p (s) && gh_procedure_p (barsiz_proc))
    {
      String str  =ly_scm2string (s);
      SCM siz = gh_call1 (barsiz_proc, me->self_scm ());
      Real sz =  gh_scm2double (siz);
      if (sz < 0)
	return SCM_EOL;
      
      return compound_barline (me, str, sz).create_scheme ();
    }
  return SCM_EOL;
}


Molecule
Bar::compound_barline (Score_element*me, String str, Real h)
{
  Real kern = gh_scm2double (me->get_elt_property ("kern"));
  Real thinkern = gh_scm2double (me->get_elt_property ("thin-kern"));
  Real hair = gh_scm2double (me->get_elt_property ("hair-thickness"));
  Real fatline = gh_scm2double (me->get_elt_property ("thick-thickness"));

  Real staffline = me->paper_l ()->get_var ("stafflinethickness");

  kern *= staffline;
  thinkern *= staffline;
  hair *= staffline;
  fatline *= staffline;
  
  Molecule thin = simple_barline (me, hair, h);
  Molecule thick = simple_barline (me, fatline, h);
  Molecule colon = Font_interface::get_default_font (me)->find_by_name ("dots-repeatcolon");  

  Molecule m;
  
  if (str == "")
    {
      return Lookup::blank (Box (Interval(0, 0), Interval (-h/2, h/2)));
    }
  else if (str == "|")
    {
      return thin;
    }
  else if (str == "|.")
    {
      m.add_at_edge (X_AXIS, LEFT, thick, 0);      
      m.add_at_edge (X_AXIS, LEFT, thin, kern);
    }
  else if (str == ".|")
    {
      m.add_at_edge (X_AXIS, RIGHT, thick, 0);
      m.add_at_edge (X_AXIS, RIGHT, thin, kern);
    }
  else if (str == ":|")
    {
      m.add_at_edge (X_AXIS, LEFT, thick, 0);
      m.add_at_edge (X_AXIS, LEFT, thin, kern);
      m.add_at_edge (X_AXIS, LEFT, colon, kern);      
    }
  else if (str == "|:")
    {
      m.add_at_edge (X_AXIS, RIGHT, thick, 0);
      m.add_at_edge (X_AXIS, RIGHT, thin, kern);
      m.add_at_edge (X_AXIS, RIGHT, colon, kern);      
    }
  else if (str == ":|:")
    {
      m.add_at_edge (X_AXIS, LEFT, thick, thinkern);
      m.add_at_edge (X_AXIS, LEFT, colon, kern);      
      m.add_at_edge (X_AXIS, RIGHT, thick, kern);
      m.add_at_edge (X_AXIS, RIGHT, colon, kern);      
    }
  else if (str == ".|.")
    {
      m.add_at_edge (X_AXIS, LEFT, thick, thinkern);
      m.add_at_edge (X_AXIS, RIGHT, thick, kern);      
    }
  else if (str == "||")
    {
      m.add_at_edge (X_AXIS, RIGHT, thin, 0);
      m.add_at_edge (X_AXIS, RIGHT, thin, thinkern);      
    }

  return m;
}


Molecule
Bar::simple_barline (Score_element*,Real w, Real h) 
{
  return Lookup::filledbox (Box (Interval(0,w), Interval(-h/2, h/2)));
}

MAKE_SCHEME_CALLBACK(Bar,before_line_breaking ,1);

SCM
Bar::before_line_breaking  (SCM smob)
{
  Score_element*me=unsmob_element (smob);
  Item * item = dynamic_cast<Item*> (me);
  
  SCM g = me->get_elt_property ("glyph");
  SCM orig = g;
  Direction bsd = item->break_status_dir ();
  if (gh_string_p (g) && bsd)
    {
      SCM proc = me->get_elt_property ("break-glyph-function");
      g = gh_call2 (proc, g, gh_int2scm (bsd));
    }

  
  if (!gh_string_p (g))
    {
      me->set_elt_property ("molecule-callback", SCM_BOOL_T);
      me->set_extent_callback (SCM_EOL, X_AXIS);
      // leave y_extent for spanbar? 
    }

  if (! gh_equal_p  (g, orig))
    me->set_elt_property ("glyph", g);

  
  /*
    set a (pseudo) stem-direction, so we extra space is inserted
    between stemup and barline.

    TODO: should check if the barline is the leftmost object of the
    break alignment.

  */
  if (gh_string_p (g))
    {
      Score_element * col = item->column_l ();
      SCM dirlist = col->get_elt_property ("dir-list");
      SCM scmdir = gh_int2scm (-1); 
      if (scm_memq (scmdir, dirlist) == SCM_BOOL_F)
	{
	  dirlist = gh_cons (scmdir, dirlist);
	  col->set_elt_property ("dir-list", dirlist);
	}
    }
  
  return SCM_UNSPECIFIED;
}
  
void
Bar::set_interface (Score_element*me)
{
  me->set_interface (ly_symbol2scm ("bar-line-interface"));
}

bool
Bar::has_interface (Score_element*m)
{
  return m && m->has_interface (ly_symbol2scm ("bar-line-interface"));
}


MAKE_SCHEME_CALLBACK(Bar,get_staff_bar_size,1);
SCM
Bar::get_staff_bar_size (SCM smob) 
{
  Score_element*me = unsmob_element (smob);
  Real ss = Staff_symbol_referencer::staff_space (me);
  SCM size = me->get_elt_property ("bar-size");
  if (gh_number_p (size))
    return gh_double2scm (gh_scm2double(size)*ss);
  else
    {
      return gh_double2scm ((Staff_symbol_referencer::line_count (me) -1) * ss);
    }
}
