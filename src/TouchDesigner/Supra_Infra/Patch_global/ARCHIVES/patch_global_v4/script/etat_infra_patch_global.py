# me - this DAT
# 
# channel - the Channel object which has changed
# sampleIndex - the index of the changed sample
# val - the numeric value of the changed sample
# prev - the previous sample value
# 
# Make sure the corresponding toggle is enabled in the CHOP Execute DAT.


absorption = 0
symbiose = 1
saturation = 2

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
	       op('select8').par.renameto = '1'
	    elif val == symbiose:
	       op('select8').par.renameto = '2'
	    elif val == saturation:
	       op('select8').par.renameto = '3'
	    return
	
