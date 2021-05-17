
# P25 TDMA Decoder (C) Copyright 2013 KA1RBI
#
# This file is part of OP25
#
# OP25 is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3, or (at your option)
# any later version.
#
# OP25 is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public
# License for more details.
#
# You should have received a copy of the GNU General Public License
# along with OP25; see the file COPYING. If not, write to the Free
# Software Foundation, Inc., 51 Franklin Street, Boston, MA
# 02110-1301, USA.

import numpy as np

def rev_int(n,l):
	j=0
	for i in xrange(l):
		b=n&1
		n=n>>1
		j = (j << 1) | b
	return j

def bits_to_dibits(bits):
	d = []
	for i in xrange(len(bits)>>1):
		d.append((bits[i*2]<<1) + bits[i*2+1])
	return d

def dibits_to_bits(dibits):
	b = []
	for d in dibits:
		b.append((d>>1)&1)
		b.append(d&1)
	return b

def mk_array(n, l):
	a = []
	for i in xrange(0,l):
		a.insert(0, n & 1)
		n = n >> 1
	return np.array(a)

def mk_int(a):
	res= 0L
	for i in xrange(0, len(a)):
		res = res * 2
		res = res + (a[i] & 1)
	return res

def mk_str(a):
	return ''.join(['%s' % (x&1) for x in a])

def check_l(a,b):
	ans = 0
	assert len(a) == len(b)
	for i in xrange(len(a)):
		if (a[i] == b[i]):
			ans += 1
	return ans

def fixup(a):
	res = []
	for c in a:
		if c == 3:
			res.append(1)
		else:	# -3
			res.append(3)

	return res

def find_sym(pattern, symbols):
	for i in xrange(0, len(symbols)-len(pattern)):
		chunk = symbols[i : i + len(pattern)]
		if chunk == pattern:
			return i
	return -1
