# me - this DAT
# 
# channel - the Channel object which has changed
# sampleIndex - the index of the changed sample
# val - the numeric value of the changed sample
# prev - the previous sample value
# 
# Make sure the corresponding toggle is enabled in the CHOP Execute DAT.

parser = op('RzParser')
rzIndex = parser[0]
transfer = parser[1]
recolte = parser[2]
creation = parser[3]
nodeIndex = parser[4]

source = op('../Rz1')
source2 = op('../../Rhizome_1')

def onOffToOn(channel, sampleIndex, val, prev):	
	return

def whileOn(channel, sampleIndex, val, prev):
	return

def onOnToOff(channel, sampleIndex, val, prev):
	return

def whileOff(channel, sampleIndex, val, prev):
	return

def onValueChange(channel, sampleIndex, val, prev):
	# Check l'index du rhizomeeee
	if rzIndex == 0 :
		targetRz = op('../Rz2')

	elif rzIndex == 1 :
		targetRz = op('../Rz3')
		
	elif rzIndex == 2 :
		targetRz = op('../Rz4')

	# Check l'index du Noeud!
	if nodeIndex == 0 :
		targetN = op('../../Watchdogs/Dispatcher/Nodes_Supra')[0]
		targetN2 = op('../../Node_S1')

	elif nodeIndex == 1 :
		targetN = op('../../Watchdogs/Dispatcher/Nodes_Supra')[1]
		targetN2 = op('../../Node_S2')
		
	elif nodeIndex == 2 :
		targetN = op('../../Watchdogs/Dispatcher/Nodes_Supra')[2]
		targetN2 = op('../../Node_S3')

	elif nodeIndex == 3 :
		targetN = op('../../Watchdogs/Dispatcher/Nodes_Infra')[0]
		targetN2 = op('../../Node_i1')
		
	elif nodeIndex == 4 :
		targetN = op('../../Watchdogs/Dispatcher/Nodes_Infra')[1]
		targetN2 = op('../../Node_i2')
	
	elif nodeIndex == 5 :
		targetN = op('../../Watchdogs/Dispatcher/Nodes_Infra')[2]
		targetN2 = op('../../Node_i3')
		

	# Création d'énergie
	if creation :
		targetRz.par.Creation = 1
		source.par.Creation = 1
	
	else :
		targetRz.par.Creation = 0
		source.par.Creation = 0

	# Transfert d'énergie
	if transfer and source2.par.Energysim < 100:
		source.par.Transfert = 1

		source2.par.Energysim = source2.par.Energysim + 10
		targetN2.par.Energysim = targetN2.par.Energysim - 10
		
		
	# else :
	# 	targetN.par.Transfert = 0
	# 	source.par.Transfert = 0
	return
