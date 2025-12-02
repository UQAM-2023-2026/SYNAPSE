# me - this DAT
# 
# channel - the Channel object which has changed
# sampleIndex - the index of the changed sample
# val - the numeric value of the changed sample
# prev - the previous sample value
# 
# Make sure the corresponding toggle is enabled in the CHOP Execute DAT.


absorptionM = 0.5
absorptionMax = 0
Idle = 0.8
symbioseM = 1
symbioseMax = 1.2
saturationM = 1.5
saturationMax = 2

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
	       op('select8').par.renameto = 'absorptionM'
	    elif val == absorptionMax: 
	       op('select8').par.renameto = 'absorptionMax'
	    elif val == Idle:
	       op('select8').par.renameto = 'Idle'
	    elif val == symbioseM:
	       op('select8').par.renameto = 'symbioseM'
	    elif val == symbioseMax:
	       op('select8').par.renameto = 'symbioseMax'
	    elif val == saturationM:
	       op('select8').par.renameto = 'saturationM'
	    elif val == saturationMax:
	       op('select8').par.renameto = 'saturationMax'
	    return
	
