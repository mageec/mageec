/*  Wrappers to expose functionality to python
    Copyright (C) 2014 Embecosm Limited and University of Bristol

    This file is part of MAGEEC.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>. */

#include <boost/python.hpp>
#include "mageec/elfhash.h"

BOOST_PYTHON_MODULE(mageec)
{
    using namespace boost::python;
    using namespace mageec;

    class_<hashedelf>("hashedelf")
        .def("hash", &hashedelf::hash)
        .def("getResult", &hashedelf::getResult);
}
