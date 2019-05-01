/* Mealy FSM implementation

   Copyright (C) 2017 by Jose M. Moya

   This file is part of GreenLSI Unlessons.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#include <stdlib.h>
#include "fsm.h"

fsm_t*
fsm_new (int state, fsm_trans_t* tt, void* user_data)
{
  fsm_t* this = (fsm_t*) malloc (sizeof (fsm_t));
  fsm_init (this, state, tt, user_data);
  return this;
}

void
fsm_init (fsm_t* this, int state, fsm_trans_t* tt, void* user_data)
{
  this->current_state = state;
  this->tt = tt;
  this->user_data = user_data;
}

void
fsm_destroy (fsm_t* this)
{
  free(this);
}

void
fsm_fire (fsm_t* this)
{
  fsm_trans_t* t;
  for (t = this->tt; t->orig_state >= 0; ++t) {
    if ((this->current_state == t->orig_state) && t->in(this)) {
      this->current_state = t->dest_state;
      if (t->out)
        t->out(this);
      break;
    }
  }
}
