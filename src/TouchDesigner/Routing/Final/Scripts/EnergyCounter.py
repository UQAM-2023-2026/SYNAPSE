"""
CHOP Execute DAT

me - this DAT

Make sure the corresponding toggle is enabled in the CHOP Execute DAT.
"""

def onOffToOn(channel: Channel, sampleIndex: int, val: float, 
              prev: float):
	# Mapping channel index to the values to add
	index = channel.index
	
	# SUPRA S1-S4 (channels 0-3)
	if 0 <= index <= 3:
		value_op_name = f'S{index + 1}'
		value_to_add = op(value_op_name).par.const0value
		op('SUPRA_Total').par.const0value += value_to_add
		
	# INFRA I1-I4 (channels 4-7)
	elif 4 <= index <= 7:
		value_op_name = f'I{index - 3}'
		value_to_add = op(value_op_name).par.const0value
		op('INFRA_Total').par.const0value += value_to_add
	
	return

def whileOn(channel: Channel, sampleIndex: int, val: float, 
            prev: float):
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
	"""
	Called every frame while a channel is 0.
	
	Args:
		channel: The Channel object which has changed
		sampleIndex: The index of the changed sample
		val: The numeric value of the changed sample
		prev: The previous sample value
	"""
	return

def onValueChange(channel: Channel, sampleIndex: int, val: float, 
                  prev: float):
	return
