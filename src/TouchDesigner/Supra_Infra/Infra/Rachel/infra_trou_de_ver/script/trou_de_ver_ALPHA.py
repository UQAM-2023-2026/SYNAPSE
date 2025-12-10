# me - this DAT
# 
# channel - the Channel object which has changed
# sampleIndex - the index of the changed sample
# val - the numeric value of the changed sample
# prev - the previous sample value
# 
# Make sure the corresponding toggle is enabled in the CHOP Execute DAT.

def onOffToOn(channel, sampleIndex, val, prev):
	return

def whileOn(channel, sampleIndex, val, prev):
	return

def onOnToOff(channel, sampleIndex, val, prev):
	return

def whileOff(channel, sampleIndex, val, prev):
	return

def onValueChange(channel, sampleIndex, val, prev):
#     if val == 1 and prev == 0:
#         targets = op('targets')  # ton Constant CHOP
# 
#         if channel.name == 'kx': # PAS DE rhizome 
#             targets.par.value1 = 1
# 
#        elif channel.name == 'kz': # rhizome connexion
#            targets.par.value0 = 1
#            targets.par.value1 = 1
            
def onValueChange(channel: Channel, sampleIndex: int, val: float, prev: float):
	if (op('null_rhizome')['N1_RzID'] == 1 and op('null_rhizome')['N1_I/S'] == 0) or (op('null_rhizome')['N2_RzID'] == 1 and op('null_rhizome')['N2_I/S'] == 0): # rhizome connexion
	    	op('targets').par.const0value = 1
            op('targets').par.const1value = 1
	else:
			op('targets').par.const0value = 0.08
            op('targets').par.const1value = 1
	return

