/* 
  slur-configuration.cc --  implement Slur_configuration
  
  source file of the GNU LilyPond music typesetter
  
  (c) 2004--2005 Han-Wen Nienhuys <hanwen@xs4all.nl>
  
*/

#include "slur-configuration.hh"

#include <math.h>

#include "stem.hh"
#include "warn.hh"
#include "misc.hh"
#include "item.hh"
#include "group-interface.hh"
#include "slur.hh"
#include "slur-scoring.hh"
#include "spanner.hh"
#include "staff-symbol-referencer.hh"
#include "libc-extension.hh"

Bezier
avoid_staff_line (Slur_score_state const &state, 
		  Bezier bez)
{
  Offset horiz (1, 0);
  Array<Real> ts = bez.solve_derivative (horiz);

  /* TODO: handle case of broken slur.  */
  if (!ts.is_empty ()
      && (state.extremes_[LEFT].staff_ == state.extremes_[RIGHT].staff_)
      && state.extremes_[LEFT].staff_ && state.extremes_[RIGHT].staff_)
    {
      Real y = bez.curve_point (ts[0])[Y_AXIS];

      Grob *staff = state.extremes_[LEFT].staff_;

      Real p = 2 * (y - staff->relative_coordinate (state.common_[Y_AXIS], Y_AXIS))
	/ state.staff_space_;

      Real distance = fabs (my_round (p) - p);	//  in halfspaces
      if (distance < 4 * state.thickness_
	  && (int) fabs (my_round (p))
	  <= 2 * Staff_symbol_referencer::staff_radius (staff) + 0.1
	  && (int (fabs (my_round (p))) % 2
	      != Staff_symbol_referencer::line_count (staff) % 2))
	{
	  Direction resolution_dir =
	 (distance ?  state.dir_ : Direction (sign (p - my_round (p))));

	  // TODO: parameter
	  Real newp = my_round (p) + resolution_dir
	    * 5 * state.thickness_;
	
	  Real dy = (newp - p) * state.staff_space_ / 2.0;
	
	  bez.control_[1][Y_AXIS] += dy;
	  bez.control_[2][Y_AXIS] += dy;
	}
    }
  return bez;
}

Real
fit_factor (Offset dz_unit, Offset dz_perp,
	    Bezier curve, Direction d,  Array<Offset> const &avoid)
{
  Real fit_factor = 0.0;
  Offset x0 = curve.control_[0];
  curve.translate (-x0);
  curve.rotate (-dz_unit.arg ());
  curve.scale (1, d);

  Interval curve_xext;
  curve_xext.add_point (curve.control_[0][X_AXIS]);
  curve_xext.add_point (curve.control_[3][X_AXIS]);

  for (int i = 0; i < avoid.size (); i++)
    {
      Offset z = (avoid[i] - x0) ;
      Offset p (dot_product (z, dz_unit),
		d* dot_product (z, dz_perp));
      if (!curve_xext.contains (p[X_AXIS]))
	continue;

      Real y = curve.get_other_coordinate (X_AXIS, p[X_AXIS]);
      if (y)
	{
	  fit_factor = fit_factor >? (p[Y_AXIS] / y);
	}
    }
  return fit_factor;
}
	
void
Slur_configuration::generate_curve (Slur_score_state const &state,
				    Real r_0, Real h_inf )
{
  Link_array<Grob> encompasses = state.columns_;

  Array<Offset> avoid;
  for (int i = 0; i < encompasses.size (); i++)
    {
      if (state.extremes_[LEFT].note_column_ == encompasses[i]
	  || state.extremes_[RIGHT].note_column_ == encompasses[i])
	continue;

      Encompass_info inf (state.get_encompass_info (encompasses[i]));
      Real y = state.dir_ * ((state.dir_ * inf.head_) >? (state.dir_ *inf.stem_));

      avoid.push (Offset (inf.x_,  y + state.dir_ * state.parameters_.free_head_distance_));
    }

  Link_array<Grob> extra_encompasses
    = Pointer_group_interface__extract_grobs (state.slur_, (Grob *)0, "encompass-objects");
  for (int i = 0;  i < extra_encompasses.size (); i++)
    if (Slur::has_interface (extra_encompasses[i]))
      {
	Grob * small_slur = extra_encompasses[i];
	Bezier b = Slur::get_curve (small_slur);

	Offset z = b.curve_point (0.5);
	z += Offset (small_slur->relative_coordinate (state.common_[X_AXIS], X_AXIS),
		     small_slur->relative_coordinate (state.common_[Y_AXIS], Y_AXIS));

	z[Y_AXIS] += state.dir_ * state.parameters_.free_slur_distance_;
	avoid.push (z);
      }

  Offset dz = attachment_[RIGHT]- attachment_[LEFT];;
  Offset dz_unit = dz;
  dz_unit *= 1 / dz.length ();
  Offset dz_perp = dz_unit * Offset (0, 1);

  Real indent, height;
  get_slur_indent_height (&indent, &height, dz.length (), h_inf, r_0);


  Real len = dz.length ();

  /* This condition,

     len^2 > 4h^2 +  3 (i + 1/3len)^2  - 1/3 len^2

     is equivalent to:

     |bez' (0)| < | bez' (.5)|

     when (control2 - control1) has the same direction as
    (control3 - control0).  */

  

  Real max_indent = len / 3.1;
  indent = indent <? max_indent;
  
  Real a1 = sqr (len) / 3.0;
  Real a2 = 0.75 * sqr (indent + len / 3.0);
  Real max_h = a1 - a2;

  
  if (max_h < 0)
    {
      programming_error ("Slur indent too small.");
      max_h = len / 3.0 ;
    }
  else
    {
      max_h = sqrt (max_h);
    }

  Real excentricity = robust_scm2double (state.slur_->get_property ("excentricity"), 0);
  
  Real x1 = (excentricity + indent);
  Real x2 = (excentricity - indent);
  
  Bezier curve;
  curve.control_[0] = attachment_[LEFT];
  curve.control_[1] = attachment_[LEFT] + dz_perp * height * state.dir_
    + dz_unit * x1;
  curve.control_[2] = attachment_[RIGHT] + dz_perp * height * state.dir_
    + dz_unit * x2;
  curve.control_[3] = attachment_[RIGHT];

  Real ff = fit_factor (dz_unit, dz_perp, curve, state.dir_, avoid);
  
  height = height >? ((height * ff) <? max_h);

  curve.control_[0] = attachment_[LEFT];
  curve.control_[1] = attachment_[LEFT] + dz_perp * height * state.dir_
    + dz_unit * x1;
  curve.control_[2] = attachment_[RIGHT] + dz_perp * height * state.dir_
    + dz_unit * x2;
  curve.control_[3] = attachment_[RIGHT];

  curve_ = avoid_staff_line (state, curve);
  height_ = height;
}

Slur_configuration::Slur_configuration()
{
  score_ = 0.0;
  index_ = -1; 
};




void
Slur_configuration::score_encompass (Slur_score_state const &state)
{
  Bezier const &bez (curve_);
  Real demerit = 0.0;

  /*
    Distances for heads that are between slur and line between
    attachment points.
  */
  Array<Real> convex_head_distances;
  for (int j = 0; j < state.encompass_infos_.size (); j++)
    {
      Real x = state.encompass_infos_[j].x_;

      bool l_edge = j == 0;
      bool r_edge = j == state.encompass_infos_.size ()-1;
      bool edge =  l_edge || r_edge;

      if (! (x < attachment_[RIGHT][X_AXIS]
	     && x > attachment_[LEFT][X_AXIS]))
	continue;
	
      Real y = bez.get_other_coordinate (X_AXIS, x);
      if (!edge)
	{
	  Real head_dy = (y - state.encompass_infos_[j].head_);
	  if (state.dir_ * head_dy < 0)
	    {
	      demerit += state.parameters_.head_encompass_penalty_;
	      convex_head_distances.push (0.0);
	    }
	  else
	    {
	      Real hd = (head_dy)
		? (1 / fabs (head_dy) - 1 / state.parameters_.free_head_distance_)
		: state.parameters_.head_encompass_penalty_;
	      hd = (hd >? 0)<? state.parameters_.head_encompass_penalty_;

	      demerit += hd;
	    }

	  Real line_y = linear_interpolate (x,
					    attachment_[RIGHT][X_AXIS],
					    attachment_[LEFT][X_AXIS],
					    attachment_[RIGHT][Y_AXIS],
					    attachment_[LEFT][Y_AXIS]);

	  if ( 1 ) // state.dir_ * state.encompass_infos_[j].get_point (state.dir_) > state.dir_ *line_y )
	    {
		
	      Real closest =
		state.dir_ * (state.dir_ * state.encompass_infos_[j].get_point (state.dir_)
			      >? state.dir_ *line_y
			      );
	      Real d = fabs (closest - y);
	
	      convex_head_distances.push (d);
	    }
	}
	
	

      if (state.dir_ * (y - state.encompass_infos_[j].stem_) < 0)
	{
	  Real stem_dem = state.parameters_.stem_encompass_penalty_ ;
	  if ((l_edge && state.dir_ == UP)
	      || (r_edge && state.dir_ == DOWN))
	    stem_dem /= 5;

	  demerit +=  stem_dem;
	}
      else if (!edge)
	{
	  Interval ext;
	  ext.add_point (state.encompass_infos_[j].stem_);
	  ext.add_point (state.encompass_infos_[j].head_);

	  // ?
	  demerit += -state.parameters_.closeness_factor_
	    * (state.dir_
	       * (y - (ext[state.dir_] + state.dir_ * state.parameters_.free_head_distance_))
	       <? 0)
	    / state.encompass_infos_.size ();
	}
    }

  Real variance_penalty = 0.0;

  if (convex_head_distances.size ())
    {
      Real avg_distance = 0.0;
      Real min_dist = infinity_f;
      for (int j = 0; j < convex_head_distances.size (); j++)
	{
	  min_dist = min_dist <? convex_head_distances[j];
	  avg_distance += convex_head_distances[j];
	}

      /*
	For slurs over 3 or 4 heads, the average distance is not a
	good normalizer.
      */
      Real n =  convex_head_distances.size ();
      if (n <= 2)
	{
	  Real fact = 1.0;
	  avg_distance += height_ * fact;
	  n += fact;
	}

      /*
	TODO: maybe it's better to use (avgdist - mindist)*factor
	as penalty.
      */
      avg_distance /= n;
      variance_penalty = state.parameters_.head_slur_distance_max_ratio_;
      if (min_dist > 0.0)
	variance_penalty =
	  (avg_distance / (min_dist + state.parameters_.absolute_closeness_measure_ ) - 1.0)
	  <? variance_penalty;

      variance_penalty = variance_penalty >? 0.0;
      variance_penalty *= state.parameters_.head_slur_distance_factor_;
    }

#if DEBUG_SLUR_SCORING
  score_card_ += to_string ("C%.2f", demerit);
  score_card_ += to_string ("D%.2f", variance_penalty);
#endif

  score_ += demerit + variance_penalty;
}

void
Slur_configuration::score_extra_encompass (Slur_score_state const &state)
{
  Real demerit = 0.0;
  for (int j = 0; j < state.extra_encompass_infos_.size (); j++)
    {
      Drul_array<Offset> attachment = attachment_;
      Interval slur_wid (attachment[LEFT][X_AXIS], attachment[RIGHT][X_AXIS]);

      /*
	to prevent numerical inaccuracies in
	Bezier::get_other_coordinate ().
      */
      Direction d = LEFT;
      bool found = false;
      Real y = 0.0;
	
      do
	{
	  /*
	    We need to check for the bound explicitly, since the
	    slur-ending can be almost vertical, making the Y
	    coordinate a bad approximation of the object-slur
	    distance.		
	  */
	  Item * as_item =  dynamic_cast<Item*> (state.extra_encompass_infos_[j].grob_);
	  if ((as_item
	       && as_item->get_column ()
	       == state.extremes_[d] .bound_->get_column ())
	      || state.extra_encompass_infos_[j].extents_[X_AXIS].contains (attachment[d][X_AXIS]))
	    {
	      y = attachment[d][Y_AXIS];
	      found = true;
	    }
	}
      while (flip (&d) != LEFT);

      if (!found)
	{
	  Real x = state.extra_encompass_infos_[j].extents_[X_AXIS]
	    .linear_combination (state.extra_encompass_infos_[j].idx_);

	  if (!slur_wid.contains (x))
	    continue;
	
	  y = curve_.get_other_coordinate (X_AXIS, x);
	}

      Real dist = state.extra_encompass_infos_[j].extents_[Y_AXIS].distance (y);
      demerit +=
	fabs (0 >? (state.parameters_.extra_encompass_free_distance_ - dist)) /
	state.parameters_.extra_encompass_free_distance_
	* state.extra_encompass_infos_[j].penalty_;
    }
#if DEBUG_SLUR_SCORING
  score_card_ += to_string ("X%.2f", demerit);
#endif
  score_ += demerit;
}

void
Slur_configuration::score_edges (Slur_score_state const &state)
{
  Direction d = LEFT;
  Offset dz = attachment_[RIGHT]
    - attachment_[LEFT];
  Real slope = dz[Y_AXIS] / dz[X_AXIS];
  do
    {
      Real y = attachment_[d][Y_AXIS];
      Real dy = fabs (y - state.base_attachments_[d][Y_AXIS]);
	
      Real factor = state.parameters_.edge_attraction_factor_;
      Real demerit = factor * dy;
      if (state.extremes_[d].stem_
	  && state.extremes_[d].stem_dir_ == state.dir_
	  && !Stem::get_beaming (state.extremes_[d].stem_, -d)
	  )
	demerit /= 5;

      demerit *= exp (state.dir_ * d * slope
		      * state.parameters_.edge_slope_exponent_ );
	
      score_ += demerit;
#if DEBUG_SLUR_SCORING
      score_card_ += to_string ("E%.2f", demerit);
#endif
    }
  while (flip (&d) != LEFT);
}

void
Slur_configuration ::score_slopes (Slur_score_state const &state)
{
  Real dy = state.musical_dy_;
  Offset slur_dz = attachment_[RIGHT] - attachment_[LEFT];
  Real slur_dy = slur_dz[Y_AXIS];
  Real demerit = 0.0;

  demerit += ((fabs (slur_dy / slur_dz[X_AXIS])
	       - state.parameters_.max_slope_) >? 0)
    * state.parameters_.max_slope_factor_;

  /* 0.2: account for staffline offset. */
  Real max_dy = (fabs (dy) + 0.2);
  if (state.edge_has_beams_)
    max_dy += 1.0;

  if (!state.is_broken_)
    demerit += state.parameters_.steeper_slope_factor_
      * ((fabs (slur_dy) -max_dy) >? 0);

  demerit += ((fabs (slur_dy/slur_dz[X_AXIS])
	       - state.parameters_.max_slope_) >? 0)
    * state.parameters_.max_slope_factor_;

  if (sign (dy) == 0
      && sign (slur_dy) != 0
      && !state.is_broken_)
    demerit += state.parameters_.non_horizontal_penalty_;

  if (sign (dy)
      && !state.is_broken_
      && sign (slur_dy)
      && sign (slur_dy) != sign (dy))
    demerit += state.edge_has_beams_
      ? state.parameters_.same_slope_penalty_ / 10
      : state.parameters_.same_slope_penalty_;

#if DEBUG_SLUR_SCORING
  score_card_ += to_string ("S%.2f", demerit);
#endif
  score_ += demerit;
}


void
Slur_configuration::score (Slur_score_state const &state)
{
  score_extra_encompass (state);
  score_slopes  (state);
  score_edges (state);
  score_encompass (state);
}
