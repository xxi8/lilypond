/*
  dots.cc -- implement Dots

  source file of the GNU LilyPond music typesetter

  (c)  1997--1998 Han-Wen Nienhuys <hanwen@cs.uu.nl>
*/

#include "dots.hh"
#include "molecule.hh"
#include "paper-def.hh"
#include "lookup.hh"

Dots::Dots ()
{
  no_dots_i_ =0;
  position_i_ =0;
}

void
Dots::do_post_processing ()
{
  if (!(position_i_ % 2))
    position_i_ ++;
  if (!no_dots_i_)
    {
      transparent_b_ = true;
      set_empty (true);
    }
}

Molecule* 
Dots::do_brew_molecule_p () const
{
  Molecule *out = new Molecule;
  Molecule fill = lookup_l ()->fill (Box (Interval (0,0),
					       Interval (0,0)));
  out->add_molecule (fill);

  Molecule d = lookup_l ()->dots ();

  Real dw = d.dim_[X_AXIS].length ();
  d.translate_axis (-dw, X_AXIS);
  for (int i=no_dots_i_; i--; )
    {
      d.translate_axis (2*dw,X_AXIS);
      out->add_molecule (d);
    }
  Real inter_f = paper ()->internote_f ();
  out->translate_axis (inter_f * position_i_, Y_AXIS);
  return out;
}


