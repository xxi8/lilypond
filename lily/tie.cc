/*
  tie.cc -- implement Tie

  source file of the GNU LilyPond music typesetter

  (c)  1997--2000 Han-Wen Nienhuys <hanwen@cs.uu.nl>
*/
#include <math.h>

#include "lookup.hh"
#include "paper-def.hh"
#include "tie.hh"
#include "note-head.hh"
#include "bezier.hh"
#include "paper-column.hh"
#include "debug.hh"
#include "staff-symbol-referencer.hh"
#include "directional-element-interface.hh"
#include "molecule.hh"
#include "bezier-bow.hh"
#include "stem.hh"

void
Tie::set_head (Direction d, Item * head_l)
{
  assert (!head (d));
  index_set_cell (get_elt_property ("heads"), d, head_l->self_scm_);
  
  set_bounds (d, head_l);
  add_dependency (head_l);
}

Tie::Tie()
{
  set_elt_property ("heads", gh_cons (SCM_EOL, SCM_EOL));
  dy_f_drul_[LEFT] = dy_f_drul_[RIGHT] = 0.0;
  dx_f_drul_[LEFT] = dx_f_drul_[RIGHT] = 0.0;

}

Note_head* 
Tie::head (Direction d) const
{
  SCM c = get_elt_property ("heads");
  c = index_cell (c, d);

  return dynamic_cast<Note_head*> (unsmob_element (c));  
}


/*
  ugh: direction of the Tie is more complicated.  See [Ross] p136 and further
 */
Direction
Tie::get_default_dir () const
{
  Stem * sl =  head(LEFT) ? head (LEFT)->stem_l () :0;
  Stem * sr =  head(RIGHT) ? head (RIGHT)->stem_l () :0;  

  if (sl && directional_element (sl).get () == UP
      && sr && directional_element (sr).get () == UP)
    return DOWN;
  else
    return UP;
}

void
Tie::do_add_processing()
{
  if (!(head (LEFT) && head (RIGHT)))
    warning (_ ("lonely tie"));

  Direction d = LEFT;
  Drul_array<Note_head *> new_head_drul;
  new_head_drul[LEFT] = head(LEFT);
  new_head_drul[RIGHT] = head(RIGHT);  
  do {
    if (!head (d))
      new_head_drul[d] = head((Direction)-d);
  } while (flip(&d) != LEFT);

  index_set_cell (get_elt_property ("heads"), LEFT, new_head_drul[LEFT]->self_scm_ );
  index_set_cell (get_elt_property ("heads"), RIGHT, new_head_drul[LEFT]->self_scm_ );

}

void
Tie::do_post_processing()
{
  if (!head (LEFT) && !head (RIGHT))
    {
      programming_error ("Tie without heads.");
      set_elt_property ("transparent", SCM_BOOL_T);
      set_empty (X_AXIS);
      set_empty (Y_AXIS);
      return;
    }

  if (!directional_element (this).get ())
    directional_element (this).set (get_default_dir ());
  
  Real staff_space = paper_l ()->get_var ("interline");
  Real half_staff_space = staff_space / 2;
  Real x_gap_f = paper_l ()->get_var ("tie_x_gap");
  Real y_gap_f = paper_l ()->get_var ("tie_y_gap");

  /* 
   Slur and tie placement [OSU]

   Ties:

       * x = inner vertical tangent - d * gap

   */


  /*
    OSU: not different for outer notes, so why all this code?
    ie,  can we drop this, or should it be made switchable.
   */
  if (head (LEFT))
    dx_f_drul_[LEFT] = head (LEFT)->extent (X_AXIS).length ();
  else
    dx_f_drul_[LEFT] = get_broken_left_end_align ();
  dx_f_drul_[LEFT] += x_gap_f;
  dx_f_drul_[RIGHT] -= x_gap_f;

  /* 
   Slur and tie placement [OSU]  -- check this

   Ties:

       * y = dx <  5ss: horizontal tangent
	 y = dx >= 5ss: y next interline - d * 0.25 ss

	 which probably means that OSU assumes that

	    dy <= 5 dx

	 for smal slurs
   */


  Real ypos = head (LEFT)
    ? staff_symbol_referencer (head (LEFT)).position_f ()
    : staff_symbol_referencer (head (RIGHT)).position_f () ;  

  Real y_f = half_staff_space * ypos; 
  int ypos_i = int (ypos);
 
  Real dx_f = extent (X_AXIS).length () + dx_f_drul_[RIGHT] - dx_f_drul_[LEFT];
  Direction dir = directional_element (this).get();
  if (dx_f < paper_l ()->get_var ("tie_staffspace_length"))
    {
      if (abs (ypos_i) % 2)
	y_f += dir * half_staff_space;
      y_f += dir * y_gap_f;
    }
  else
    {
      if (! (abs (ypos_i) % 2))
	y_f += dir * half_staff_space;
      y_f += dir * half_staff_space;
      y_f -= dir * y_gap_f;
    }
  
  dy_f_drul_[LEFT] = dy_f_drul_[RIGHT] = y_f;
}



Array<Rod>
Tie::get_rods () const
{
  Array<Rod> a;
  Rod r;
  r.item_l_drul_ = spanned_drul_;
  r.distance_f_ = paper_l ()->get_var ("tie_x_minimum");
  a.push (r);
  return a;
}




Molecule*
Tie::do_brew_molecule_p () const
{
  Real thick = paper_l ()->get_var ("tie_thickness");
  Bezier one = get_curve ();

  Molecule a;
  SCM d =  get_elt_property ("dashed");
  if (gh_number_p (d))
    a = lookup_l ()->dashed_slur (one, thick, gh_scm2int (d));
  else
    a = lookup_l ()->slur (one, directional_element (this).get () * thick, thick);
  
  return new Molecule (a); 
}



Bezier
Tie::get_curve () const
{
  Direction d (directional_element (this).get ());
  Bezier_bow b (get_encompass_offset_arr (), d);

  b.ratio_ = paper_l ()->get_var ("slur_ratio");
  b.height_limit_ = paper_l ()->get_var ("slur_height_limit");
  b.rc_factor_ = paper_l ()->get_var ("slur_rc_factor");

  b.calculate ();
  Bezier c (b.get_curve ());

  /* should do this for slurs as well. */
  Array<Real> horizontal (c.solve_derivative (Offset (1,0)));

  if (horizontal.size ())
    {
      /*
	ugh. Doesnt work for non-horizontal curves.
       */
      Real space = staff_symbol_referencer (this).staff_space ();
      Real y = c.curve_point (horizontal[0])[Y_AXIS];

      Real ry = rint (y/space) * space;
      Real diff = ry - y;
      Real newy = y;
      if (fabs (diff) < paper_l ()->get_var ("tie_staffline_clearance"))
	{
	  newy = ry - 0.5 * space * sign (diff) ;
	}

      Real y0 = c.control_ [0][Y_AXIS];
      c.control_[2][Y_AXIS] = 
      c.control_[1][Y_AXIS] =
	(c.control_[1][Y_AXIS] - y0)  * (newy / y) + y0; 
    }
  else
    programming_error ("Tie is nowhere horizontal");
  return c;
}


Array<Offset>
Tie::get_encompass_offset_arr () const
{
  Array<Offset> offset_arr;
  offset_arr.push (Offset (dx_f_drul_[LEFT], dy_f_drul_[LEFT]));
  offset_arr.push (Offset (spanner_length () + dx_f_drul_[RIGHT],
			   dy_f_drul_[RIGHT]));
		      
  return offset_arr;
}


