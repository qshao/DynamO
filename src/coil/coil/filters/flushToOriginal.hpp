/*  dynamo:- Event driven molecular dynamics simulator 
    http://www.marcusbannerman.co.uk/dynamo
    Copyright (C) 2011  Marcus N Campbell Bannerman <m.bannerman@gmail.com>

    This program is free software: you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    version 3 as published by the Free Software Foundation.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once
#include "filter.hpp"

namespace coil 
{
  class FlushToOriginal: public filter
  {
  public:
    FlushToOriginal() {}

    inline virtual size_t type_id() { return detail::filterEnum<FlushToOriginal>::val; }    
    inline virtual void invoke(GLint colorTextureUnit, size_t width, size_t height,
			       const magnet::GL::viewPort& vp)
    {}

  protected:
  };
}
