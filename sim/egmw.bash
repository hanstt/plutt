#/bin/bash

# plutt, a scriptable monitor for experimental data.
#
# Copyright (C) 2025
# Hans Toshihide Toernqvist <hans.tornqvist@chalmers.se>
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
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
# MA  02110-1301  USA

cd $(dirname $0)
root -q egmw.C
../plutt -f egmw.plutt -r T egmw.root
rm -f egmw.root
