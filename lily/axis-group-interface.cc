/*   
  axis-group-interface.cc --  implement Axis_group_interface
  
  source file of the GNU LilyPond music typesetter
  
  (c) 2000 Han-Wen Nienhuys <hanwen@cs.uu.nl>
  
 */

#include "axis-group-interface.hh"
#include "score-element.hh"
#include "dimension-cache.hh"

Axis_group_interface::Axis_group_interface (Score_element*s)
  : Group_interface (s)
{
  elt_l_ = s;
}

Axis_group_interface
axis_group (Score_element*s)
{
  return Axis_group_interface (s);
}

void
Axis_group_interface::add_element (Score_element *e)
{
  elt_l_->used_b_ = true;
  e->used_b_ = true;

  for (SCM ax = elt_l_->get_elt_property ("axes"); ax != SCM_EOL ; ax = gh_cdr (ax))
    {
      Axis a = (Axis) gh_scm2int (gh_car (ax));
      
      if (!e->parent_l (a))
	e->set_parent (elt_l_, a);
    }

  Group_interface::add_element (e);

  elt_l_->add_dependency (e);
}


bool
Axis_group_interface::axis_b (Axis a )const
{
  return elt_l_->dim_cache_[a]->extent_callback_l_ == group_extent_callback;
}

Interval
Axis_group_interface::group_extent_callback (Dimension_cache const *c) 
{
  Axis a = c->axis ();
  Score_element * me = c->element_l ();

  Interval r;
  for (SCM s = me->get_elt_property ("elements"); gh_pair_p (s); s = gh_cdr (s))
    {
      SCM e=gh_car (s); 
      Score_element * se = SMOB_TO_TYPE (Score_element, e);

      Interval dims = se->extent (a);
      if (!dims.empty_b ())
	r.unite (dims + se->relative_coordinate (me, a));
    }

  return r;
}


void
Axis_group_interface::set_interface ()
{
  if (!has_interface_b ())
    {
      elt_l_->set_elt_property ("elements", SCM_EOL);
      elt_l_->set_elt_property ("transparent", SCM_BOOL_T);
      elt_l_->set_elt_property ("axes" , SCM_EOL);
      group (elt_l_, "interfaces").add_thing (ly_symbol2scm ("Axis_group"));
    }
}

void
Axis_group_interface::set_axes (Axis a1, Axis a2)
{
  // set_interface () ? 

  SCM ax = gh_cons (gh_int2scm (a1), SCM_EOL);
  if (a1 != a2)
    ax= gh_cons (gh_int2scm (a2), ax);

  
  elt_l_->set_elt_property ("axes", ax);

  if (a1 != X_AXIS && a2 != X_AXIS)
    elt_l_->set_empty (X_AXIS);
  if (a1 != Y_AXIS && a2 != Y_AXIS)
    elt_l_->set_empty (Y_AXIS);
  
  elt_l_->dim_cache_[a1]->set_callback (Axis_group_interface::group_extent_callback);
  elt_l_->dim_cache_[a2]->set_callback (Axis_group_interface::group_extent_callback);
}

Link_array<Score_element> 
Axis_group_interface::get_children ()
{
  Link_array<Score_element> childs;
  childs.push (elt_l_) ;

  if (!has_interface_b ())
    return childs;
  
  for (SCM ep = elt_l_->get_elt_property ("elements"); gh_pair_p (ep); ep = gh_cdr (ep))
    {
      Score_element* e = unsmob_element (gh_car (ep));
      if (e)
	childs.concat (axis_group (e).get_children ());
    }
  
  return childs;
}

bool
Axis_group_interface::has_interface_b ()
{
  SCM memq = scm_memq (ly_symbol2scm ("Axis_group"),
	      elt_l_->get_elt_property ("interfaces"));
  
  return (memq != SCM_BOOL_F);
}


