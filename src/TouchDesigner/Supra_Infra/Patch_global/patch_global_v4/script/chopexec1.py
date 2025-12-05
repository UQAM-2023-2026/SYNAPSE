# me - this DAT
# 
# channel - the Channel object which has changed
# sampleIndex - the index of the changed sample
# val - the numeric value of the changed sample
# prev - the previous sample value
# 
# Make sure the corresponding toggle is enabled in the CHOP Execute DAT.


absorptionM = 1
symbioseM = 2
saturation = 3

def onOffToOn(channel, sampleIndex, val, prev):
	return

def whileOn(channel, sampleIndex, val, prev):
	return

def onOnToOff(channel, sampleIndex, val, prev):
	return

def whileOff(channel, sampleIndex, val, prev):
	return

def onValueChange(channel, sampleIndex, val, prev):
	   # States = op('States')  # ton Constant CHOP
	    if val == absorption:
	       op('select9').par.renameto = 'absorption'
	    elif val == symbioseM:
	       op('select9').par.renameto = 'symbiose'
	    elif val == saturation:
	       op('select9').par.renameto = 'saturation'
	    return
	
