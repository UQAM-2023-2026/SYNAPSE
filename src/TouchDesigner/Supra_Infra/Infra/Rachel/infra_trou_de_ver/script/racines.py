"""
CHOP Execute DAT

me - this DAT

Make sure the corresponding toggle is enabled in the CHOP Execute DAT.
"""


def onValueChange(channel: Channel, sampleIndex: int, val: float, prev: float):
	if (op('null7')['N1_RzID'] == 1 and op('null7')['N1_I/S'] == 0) or (op('null7')['N2_RzID'] == 1 and op('null7')['N2_I/S'] == 0): # rhizome connexion
		op('constant2').par.const0value = 1
	else:
		op('constant2').par.const0value = 0
	return
