
/*   
  tie-column.hh -- declare Tie_column
  
  source file of the GNU LilyPond music typesetter
  
  (c) 2000 Han-Wen Nienhuys <hanwen@cs.uu.nl>
  
 */

#ifndef TIE_COLUMN_HH
#define TIE_COLUMN_HH

#include "spanner.hh"

class Tie_column : public Spanner
{
public:
  VIRTUAL_COPY_CONS (Score_element);
  void add_tie (Score_element*);
  Tie_column ();
protected:
  virtual void do_post_processing ();
  void set_directions ();
  
};

#endif /* TIE_COLUMN_HH */

