#
# @file link_handler.mk
# @version 0.0.1
#
# @section License
# Copyright (C) 2016, Erik Moqvist
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#

INC += $(LINK_HANDLER_ROOT)/src

LINK_HANDLER_SRC ?= link_handler.c

SRC += $(LINK_HANDLER_SRC:%=$(LINK_HANDLER_ROOT)/src/%)
