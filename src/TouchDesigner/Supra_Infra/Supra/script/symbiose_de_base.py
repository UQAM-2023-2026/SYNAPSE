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
	if val == 2:
	#op('switch1').par.index = '0'
		op('constant1').par.const0value = '1'
		op('constant4').par.const0value = '27'
		op('constant6').par.const0value = '1'
		op('constant8').par.const0value = '0'
		#run("op('constant6').par.const0value = '0'", delayFrames=70)
		op('select3').par.chop = 'etat_symbiose1'
	return

def onOnToOff(channel, sampleIndex, val, prev):
	return

def whileOff(channel, sampleIndex, val, prev):
	return

def onValueChange(channel, sampleIndex, val, prev):
	return
	