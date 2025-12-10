"""
CHOP Execute DAT

me - this DAT

Make sure the corresponding toggle is enabled in the CHOP Execute DAT.
"""

def onOffToOn(channel: Channel, sampleIndex: int, val: float, 
              prev: float):
	"""
	Called when a channel changes from 0 to non-zero.
	
	Args:
		channel: The Channel object which has changed
		sampleIndex: The index of the changed sample
		val: The numeric value of the changed sample
		prev: The previous sample value
	"""
	return

def whileOn(channel: Channel, sampleIndex: int, val: float, 
            prev: float):
	
	op('constant4').par.const2value = 1

	"""
	Called every frame while a channel is non-zero.
	
	Args:
		channel: The Channel object which has changed
		sampleIndex: The index of the changed sample
		val: The numeric value of the changed sample
		prev: The previous sample value
	"""
	return

def onOnToOff(channel: Channel, sampleIndex: int, val: float, 
              prev: float):
	"""
	Called when a channel changes from non-zero to 0.
	
	Args:
		channel: The Channel object which has changed
		sampleIndex: The index of the changed sample
		val: The numeric value of the changed sample
		prev: The previous sample value
	"""
	return

def whileOff(channel: Channel, sampleIndex: int, val: float, 
             prev: float):
	
	op('constant4').par.const2value = 0

	"""
	Called every frame while a channel is 0.
	
	Args:
		channel: The Channel object which has changed
		sampleIndex: The index of the changed sample
		val: The numeric value of the changed sample
		prev: The previous sample value
	"""
	return

def onValueChange(channel, sampleIndex, val, prev):
	rhiz_id = op('valRhiz')[0]
	rhiz_energy = op('valRhiz')[1]
	node_id = op('valRhiz')[2]
	node_level = op('valRhiz')[3]
	
	energy_max = op('../../../Global_Config')[0]
	

	if rhiz_id == 0:
		op('../../../RZ1').par.Energy = 0
		op('../../../RZ1').par.Connected = False

		op('../../../RZ2').par.Energy = 0
		op('../../../RZ2').par.Connected = False


	if rhiz_id == 1 :
		target_op = op('../../../RZ1')
		target_op.par.Energy = (rhiz_energy * energy_max) / 100

	elif rhiz_id == 2 :
		target_op = op('../../../RZ2')
		target_op.par.Energy = (rhiz_energy * energy_max) / 100

	if rhiz_id > 0:
		target_op.par.Connected = True
		
	if node_id == 1 and node_level == 1:
		target_op.par.Targetindex = 0
	
	elif node_id == 2 and node_level == 1:
		target_op.par.Targetindex = 1
    
	elif node_id == 1 and node_level == 0:
		target_op.par.Targetindex = 3

	elif node_id == 2 and node_level == 0:
		target_op.par.Targetindex = 4

	if node_level == 0:
		op('constant4').par.const0value = op.Dispatcher.op('touchout1')['State_Infra']

	elif node_level == 1:
		op('constant4').par.const0value = op.Dispatcher.op('touchout1')['State_Supra']

	return