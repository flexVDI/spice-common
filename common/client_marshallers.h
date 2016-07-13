/* -*- Mode: C; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
   Copyright (C) 2010 Red Hat, Inc.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _H_MARSHALLERS
#define _H_MARSHALLERS

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <spice/protocol.h>

#include "common/generated_client_marshallers.h"
#include "marshaller.h"
#include "messages.h"

SPICE_BEGIN_DECLS

SpiceMessageMarshallers *spice_message_marshallers_get(void);
SpiceMessageMarshallers *spice_message_marshallers_get1(void);

SPICE_END_DECLS

#endif
