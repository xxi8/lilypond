/*   
  protected-scm.cc --  implement Protected_scm
  
  source file of the GNU LilyPond music typesetter
  
  (c) 1998--1999 Han-Wen Nienhuys <hanwen@cs.uu.nl>
  
 */
#include "protected-scm.hh"
#include "lily-guile.hh"
#include "main.hh"

Protected_scm::Protected_scm ()
{
  object_ = 0;
}

Protected_scm::Protected_scm (SCM s)
{
  object_ = s  ? scm_protect_object (s): 0;
}

Protected_scm::Protected_scm (Protected_scm const &s)
{
  object_ = s.object_ ? scm_protect_object (s.object_) : 0;
}

Protected_scm & 
Protected_scm::operator =(SCM s)
{
  if (object_ == s)
    return *this;
  if (object_)
    scm_unprotect_object(object_);

  object_ =  s ? scm_protect_object (s): 0;
  return *this;
}

Protected_scm&
Protected_scm::operator = (Protected_scm const &s)
{
  return operator= (s.object_);
}


Protected_scm::~Protected_scm ()
{
  if  (object_)
    {
      scm_unprotect_object (object_);
    }
}

Protected_scm::operator SCM () const
{
  return object_;
}

SCM 
Protected_scm::to_SCM () const
{
  return object_;
}
