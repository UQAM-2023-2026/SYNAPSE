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

# -------------------------
# Helpers pour un flux sur 5 s
# -------------------------

FLOW_TOTAL = 10.0          # quantité totale à transférer/récolter par action
FLOW_DURATION = 5.0        # secondes pour compléter
# => débit constant (unités/s)
def _flow_busy():
	return bool(me.fetch('flow_active', False))

def _flow_clear():
	me.unstore('flow_active')
	me.unstore('flow_mode')
	me.unstore('src_path')
	me.unstore('dst_path')
	me.unstore('remaining')
	me.unstore('rate')
	me.unstore('last_time')

def _start_flow(mode, src_op, dst_op, total=FLOW_TOTAL, duration=FLOW_DURATION):
	"""
	mode: 'transfer' (nœud -> rhizome) ou 'recolte' (rhizome -> nœud)
	src_op: OP qui PERD de l'énergie
	dst_op: OP qui GAGNE de l'énergie
	"""
	if _flow_busy():
		return  # on ignore si un flux est déjà en cours (évite les overlaps)

	# Calcul des capacités/limites immédiates
	src_energy = float(src_op.par.Energysim)
	dst_energy = float(dst_op.par.Energysim)
	src_can_give = max(0.0, src_energy - 0.0)          # combien src peut donner sans passer sous 0
	dst_can_take = max(0.0, 100.0 - dst_energy)        # combien dst peut recevoir sans dépasser 100

	# On borne la quantité totale pour éviter overflow/underflow
	total_effective = min(total, src_can_give, dst_can_take)
	if total_effective <= 0.0:
		return

	rate = total_effective / max(0.001, duration)      # unités par seconde

	me.store('flow_active', True)
	me.store('flow_mode', mode)
	me.store('src_path', src_op.path)
	me.store('dst_path', dst_op.path)
	me.store('remaining', total_effective)
	me.store('rate', rate)
	me.store('last_time', absTime.seconds)

	# Déclenche le premier tick au prochain frame
	run('_tick_flow()', delayFrames=1)

def _tick_flow():
	# Récupère l'état
	if not _flow_busy():
		return

	src_op = op(me.fetch('src_path'))
	dst_op = op(me.fetch('dst_path'))
	if not src_op or not dst_op:
		_flow_clear()
		return

	now = absTime.seconds
	last = float(me.fetch('last_time'))
	dt = max(0.0, now - last)
	me.store('last_time', now)

	remaining = float(me.fetch('remaining'))
	rate = float(me.fetch('rate'))

	# quantité à appliquer sur ce tick
	step = min(remaining, rate * dt)

	# Applique avec clamps 0–100
	src_energy = float(src_op.par.Energysim)
	dst_energy = float(dst_op.par.Energysim)

	# borne par sécurité en cas de changements externes
	step = min(step, max(0.0, src_energy - 0.0))
	step = min(step, max(0.0, 100.0 - dst_energy))

	if step > 0.0:
		src_op.par.Energysim = max(0.0, src_energy - step)
		dst_op.par.Energysim = min(100.0, dst_energy + step)
		remaining -= step
		me.store('remaining', remaining)

	# Continue ou stop
	if remaining > 1e-6:
		run('_tick_flow()', delayFrames=1)
	else:
		_flow_clear()

def onOffToOn(channel, sampleIndex, val, prev):	
	return

def whileOn(channel, sampleIndex, val, prev):
	return

def onOnToOff(channel, sampleIndex, val, prev):
	return

def whileOff(channel, sampleIndex, val, prev):
	return

def onValueChange(channel, sampleIndex, val, prev):
	# Conversion sûre
	rzIndexVal = int(rzIndex)
	nodeIndexVal = int(nodeIndex)
	transferVal = bool(int(transfer))
	recolteVal = bool(int(recolte))
	creationVal = bool(int(creation))

	# Check l'index du rhizome
	if rzIndexVal == 0:
		targetRz = op('../Rz2')
	elif rzIndexVal == 1:
		targetRz = op('../Rz3')
	elif rzIndexVal == 2:
		targetRz = op('../Rz4')
	else:
		return  # sécurité si index invalide

	# Check l'index du Noeud
	if nodeIndexVal == 0:
		targetN = op('../../Watchdogs/Dispatcher/Nodes_Supra')[0]
		targetN2 = op('../../Node_S1')
	elif nodeIndexVal == 1:
		targetN = op('../../Watchdogs/Dispatcher/Nodes_Supra')[1]
		targetN2 = op('../../Node_S2')
	elif nodeIndexVal == 2:
		targetN = op('../../Watchdogs/Dispatcher/Nodes_Supra')[2]
		targetN2 = op('../../Node_S3')
	elif nodeIndexVal == 3:
		targetN = op('../../Watchdogs/Dispatcher/Nodes_Infra')[0]
		targetN2 = op('../../Node_i1')
	elif nodeIndexVal == 4:
		targetN = op('../../Watchdogs/Dispatcher/Nodes_Infra')[1]
		targetN2 = op('../../Node_i2')
	elif nodeIndexVal == 5:
		targetN = op('../../Watchdogs/Dispatcher/Nodes_Infra')[2]
		targetN2 = op('../../Node_i3')
	else:
		return  # sécurité si index invalide

	# --- Création d'énergie (flag/état) ---
	if creationVal:
		targetRz.par.Creation = 1
		source.par.Creation = 1
	else:
		targetRz.par.Creation = 0
		source.par.Creation = 0

	# --- Gestion des flags visuels (optionnels) ---
	source.par.Transfert = 1 if transferVal else 0
	source.par.Rcolte  = 1 if recolteVal  else 0

	# --- Débit sur 5 s ---
	# Transfert = nœud -> rhizome (targetN2 -> source2)
	if transferVal and not _flow_busy():
		_start_flow('transfer', src_op=targetN2, dst_op=source2)

	# Récolte = rhizome -> nœud (source2 -> targetN2)
	if recolteVal and not _flow_busy():
		_start_flow('recolte', src_op=source2, dst_op=targetN2)

	return
