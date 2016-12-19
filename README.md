The goal of this project was to create BLDC motor controller for AVR uCs.

Unlike other Asm software, it's written mostly in C, potentially allowing faster and easier development. Unlike other C software, it's fast, allowing high RMPs. Time consuming interrupts routines and expensive arithmetic operations have been written in assembly. Thanks to high optimization of the PWM interrupt handling routines, fixed frequency PWM is used and no "power bump" near 100% throttle is present. This soft comes with example eagle PCB, which it was tested on.

Features so far:
 - stick programming
 - possible high RPMs
 - fast conversion from commutation period to speed (frequency) using Lookup-table.
 - experimental governor mode, using speed as feedback.
 - brake

Possible development:

 - other communication interfaces (I2C)
 - other boards support

License:

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.
	
	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.
	
	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.


