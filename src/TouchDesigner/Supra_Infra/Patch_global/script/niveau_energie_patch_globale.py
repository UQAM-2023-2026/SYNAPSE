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
symbioseMInfra = 4
def onOffToOn(channel, sampleIndex, val, prev):
	return

def whileOn(channel, sampleIndex, val, prev):
	chop_source = op('math4')
	niveauEnergieSupra = int(chop_source['chan1'][0])
	niveauEnergieInfra = int(chop_source['chan2'][0])
	
	if niveauEnergieSupra > saturationMSupra and niveauEnergieInfra < absorptionMInfra:
		op('constant1').par.const0value = '80'
		op('constant1').par.name0 = 'saturationMSupra'
		op('constant1').par.const1value = '20'
		op('constant1').par.name1 = 'absorptionMInfra'
	elif niveauEnergieSupra < absorptionMSupra and niveauEnergieInfra > saturationMInfra:
		op('constant1').par.const0value = '20'
		op('constant1').par.name0 = 'absorptionMSupra'
		op('constant1').par.const1value = '80'
		op('constant1').par.name1 = 'saturationMInfra'
	elif niveauEnergieSupra == saturationMaxSupra and niveauEnergieInfra == absorptionMaxInfra:
		op('constant1').par.const0value = '100'
		op('constant1').par.name0 = 'saturationMaxSupra'
		op('constant1').par.const1value = '0'
		op('constant1').par.name1 = 'absorptionMaxInfra'
	elif niveauEnergieSupra < absorptionMaxSupra and niveauEnergieInfra > saturationMaxInfra:
		op('constant1').par.const0value = '0'
		op('constant1').par.name0 = 'absorptionMaxSupra'
		op('constant1').par.const1value = '100'
		op('constant1').par.name1 = 'saturationMaxInfra'
	elif niveauEnergieSupra == symbioseBSupra and niveauEnergieInfra == symbioseBInfra:
		op('constant1').par.const0value = '10'
		op('constant1').par.name0 = 'symbioseBSupra'
		op('constant1').par.const1value = '10'
		op('constant1').par.name1 = 'symbioseBInfra'
	elif niveauEnergieSupra == symbioseMSupra and niveauEnergieInfra == symbioseMInfra:
		op('constant1').par.const0value = '30'
		op('constant1').par.name0 = 'symbioseMSupra'
		op('constant1').par.const1value = '30'
		op('constant1').par.name1 = 'symbioseMInfra'
	elif niveauEnergieSupra == symbioseMaxSupra and niveauEnergieInfra == symbioseMaxInfra:
		op('constant1').par.const0value = '50'
		op('constant1').par.name0 = 'symbioseMaxSupra'
		op('constant1').par.const1value = '50'
		op('constant1').par.name1 = 'symbioseMaxInfra'
	return

def onOnToOff(channel, sampleIndex, val, prev):
	return

def whileOff(channel, sampleIndex, val, prev):
	return

def onValueChange(channel, sampleIndex, val, prev):
	
	

	return
	