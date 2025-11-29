# me - this DAT
# 
# channel - the Channel object which has changed
# sampleIndex - the index of the changed sample
# val - the numeric value of the changed sample
# prev - the previous sample value
# 
# Make sure the corresponding toggle is enabled in the CHOP Execute DAT.

symbioseBSupra = 15
symbioseBinfra = 15
symbioseMSupra = 45
symbioseMInfra = 45
symbioseMaxSupra = 50
symbioseMaxInfra = 50
absorptionMSupra = 20
absorptionMInfra = 20
saturationMSupra = 80
saturationMInfra = 80
saturationMaxSupra = 100
saturationMaxInfra = 100
absorptionMaxSupra = 0
absorptionMaxInfra = 0
Idle = 0.8


def onOffToOn(channel, sampleIndex, val, prev):
	return

def whileOn(channel, sampleIndex, val, prev):
	#op('switch1').par.index = '0'
	if val <= symbioseB:
		op('constant1').par.const0value = '0.8'
		op('constant1').par.name0 = 'symbioseB'
	elif symbioseB < val <= symbioseM:
		op('constant1').par.const0value = '1'
		op('constant1').par.name0 = 'symbioseM'
	elif symbioseM < symbioseMax:
		op('constant1').par.const0value = '1.3'
		op('constant1').par.name0 = 'symbioseMax'
		
	elif val >= saturationMSupra && val <= absorptionMInfra
		op('constant1').par.const0value = '80'
		op('constant1').par.name0 = 'saturationMSupra'
		op('constant1').par.const1value = '20'
		op('constant1').par.name1 = 'absorptionMInfra'
	elif val <= absorptionMSupra && val >= saturationMInfra
		op('constant1').par.const0value = '20'
		op('constant1').par.name0 = 'absorptionMSupra'
		op('constant1').par.const1value = '80'
		op('constant1').par.name1 = 'saturationMInfra'
		
	elif val == saturationMaxSupra && val == absorptionMaxInfra
		op('constant1').par.const0value = '100'
		op('constant1').par.name0 = 'saturationMaxSupra'
		op('constant1').par.const1value = '0'
		op('constant1').par.name1 = 'absorptionMaxInfra'
	elif val == saturationMaxInfra && val == absorptionMaxSupra
		op('constant1').par.const0value = '0'
		op('constant1').par.name0 = 'absorptionMaxSupra'
		op('constant1').par.const1value = '100'
		op('constant1').par.name1 = 'saturationMaxInfra'
		
	elif val == symbioseBSupra && val == symbioseBInfra
		op('constant1').par.const0value = '10'
		op('constant1').par.name0 = 'symbioseBSupra'
		op('constant1').par.const1value = '10'
		op('constant1').par.name1 = 'symbioseBInfra'
		
	elif val == symbioseBSupra && val == symbioseBInfra
		op('constant1').par.const0value = '10'
		op('constant1').par.name0 = 'symbioseBSupra'
		op('constant1').par.const1value = '10'
		op('constant1').par.name1 = 'symbioseBInfra'
		
	elif val == symbioseBSupra && val == symbioseBInfra
		op('constant1').par.const0value = '10'
		op('constant1').par.name0 = 'symbioseBSupra'
		op('constant1').par.const1value = '10'
		op('constant1').par.name1 = 'symbioseBInfra'

	return

def onOnToOff(channel, sampleIndex, val, prev):
	return

def whileOff(channel, sampleIndex, val, prev):
	return

def onValueChange(channel, sampleIndex, val, prev):
	return
	