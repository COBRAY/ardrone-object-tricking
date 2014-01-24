#
#    Copyright (C) 2012 Simon D. Levy
#
#    This program is free software: you can redistribute it and/or modify
#    it under the terms of the GNU Lesser General Public License as 
#    published by the Free Software Foundation, either version 3 of the 
#    License, or (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License 
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
# You should also have received a copy of the Parrot Parrot AR.Drone 
# Development License and Parrot AR.Drone copyright notice and disclaimer 
# and If not, see 
#   <https://projects.ardrone.org/attachments/277/ParrotLicense.txt> 
# and
#   <https://projects.ardrone.org/attachments/278/ParrotCopyrightAndDisclaimer.txt>.

# This should reflect where you put the SDK
SDK_DIR:=$(HOME)/ARDrone_SDK_2_0_1/

# We currently support LOGITCH and PS3 gamepads
#GAMEPAD = GAMEPAD_LOGITECH_ID=0x046dc215
#GAMEPAD = GAMEPAD_PS3_ID=0x0e8f310d
#GAMEPAD = GAMEPAD_XBOX_ID=0x046dc21d
GAMEPAD = GAMEPAD_GENERIC_ID=0x00790006
#GAMEPAD = GAMEPAD_PS3_ID=0x054c0268
# Python version: you may need to run apt-get install python-dev as root
PYVER = 2.7

# If you use Python, make sure that PYTHONPATH shell variable contains . 
# (dot; current directory)
LANGUAGE = python
LANGUAGE_LIB = -L/usr/lib/python$(PYVER)/config -lpython$(PYVER)
LANGUAGE_PATH = /usr/include/python$(PYVER)	

# If you use Matlab, make sure that MATLAB shell variable points to location of 
# Matlab (e.g. /usr/local/MATLAB/R2013a) and that you can run csh 
# (if not: sudo apt-get install csh)
#LANGUAGE = matlab
#LDIR = $(MATLAB)/bin/glnxa64
#LANGUAGE_LIB = $(LDIR)/libboost_date_time.so.1.49.0 \
#               $(LDIR)/libboost_signals.so.1.49.0 \
#               $(LDIR)/libboost_system.so.1.49.0 \
#               $(LDIR)/libboost_thread.so.1.49.0 \
#               $(LDIR)/libboost_filesystem.so.1.49.0 \
#               $(LDIR)/libboost_log.so.1.49.0 \
#               $(LDIR)/libboost_log_setup.so.1.49.0 \
#               $(LDIR)/libboost_chrono.so.1.49.0 \
#               $(LDIR)/libboost_regex.so.1.49.0 \
#               $(LDIR)/libicuuc.so.49 \
#               $(LDIR)/libicui18n.so.49 \
#               $(LDIR)/libicuio.so.49 \
#               $(LDIR)/libicudata.so.49 \
#               $(LDIR)/libtbb.so.2 \
#               $(LDIR)/libtbbmalloc.so.2 \
#               $(LDIR)/libunwind.so.8 \
#               $(LDIR)/libhdf5_hl.so.6 \
#               $(LDIR)/libhdf5.so.6


#LANGUAGE_LIB += -L $(MATLAB)/bin/glnxa64/ -lmx -leng -lmwresource_core -lmwi18n -lmwmfl_scalar -lut -lmwfl -lmwMATLAB_res -lmwcpp11compat -lmat

#LANGUAGE_PATH=$(MATLAB)/extern/include/


# If you use C, this is all you need
#LANGUAGE = c




