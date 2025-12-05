# me - this DAT
# 
# channel - the Channel object which has changed
# sampleIndex - the index of the changed sample
# val - the numeric value of the changed sample
# prev - the previous sample value
# 
# Make sure the corresponding toggle is enabled in the CHOP Execute DAT.


absorptionM = 2
absorptionMax = 1
Idle = 3
symbioseM = 4
symbioseMax = 5
saturationM = 6
saturationMax = 7

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
	    if val == absorptionM:
	       op('select9').par.renameto = 2
	    elif val == absorptionMax: 
	       op('select9').par.renameto = 1
	    elif val == Idle:
	       op('select9').par.renameto = 3
	    elif val == symbioseM:
	       op('select9').par.renameto = 4
	    elif val == symbioseMax:
	       op('select9').par.renameto = 5
	    elif val == saturationM:
	       op('select9').par.renameto = 6
	    elif val == saturationMax:
	       op('select9').par.renameto = 7
	    return
	
	
