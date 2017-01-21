#ifndef PYQCD_ALGORITHMS_TYPES_HPP
#define PYQCD_ALGORITHMS_TYPES_HPP
/*
 * This file is part of pyQCD.
 *
 * pyQCD is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * pyQCD is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 *
 * Created by Matt Spraggs on 17/01/17.
 *
 * [DOCUMENTATION]
 */

#include <globals.hpp>
#include <gauge/types.hpp>

#include "heatbath.hpp"

namespace pyQCD
{
  namespace python
  {
    typedef Heatbath<Real, num_colours> Heatbath;
  }
}


#endif //PYQCD_ALGORITHMS_TYPES_HPP