"""
CHOP Execute DAT

me - this DAT

Make sure the corresponding toggle is enabled in the CHOP Execute DAT.
"""

import random

MAX_ITEMS = 9

def onOffToOn(channel, sampleIndex, val, prev):
    t = op('pos_table')
    #root = op('/project1/pollen_fleur/pollen_vf')
    root = op('/pollen_vf')
    threshold = int(op('nullThreshold')[0])

    # --- table: 19 slots fixes (pas de deleteRow, pas de shift) ---
    if t.numRows == 0 or t[0,0].val != 'id':
	threshold = 0
	    t.clear()
	    t.appendRow(['id','tx','ty','tz','active'])
        for i in range(1, MAX_ITEMS + 1):
            t.appendRow([str(i), 0.0, 0.0, 0.0, 0])

    # trouver un slot "libre" (0,0,0). Si aucun, recycle le 1.
    slot = None
    for i in range(1, MAX_ITEMS + 1):
        if float(t[i,1].val) == 0.0 and float(t[i,2].val) == 0.0 and float(t[i,3].val) == 0.0:
            slot = i
            break
    if slot is None:
        slot = 1

    # nouvelle position (corrigé)
    x = random.uniform(-3, 3)
    y = random.uniform(-1, 1)
    z = 0.0

    # écrire dans la table (slot fixe)
    t[slot, 1] = str(x)
    t[slot, 2] = str(y)
    t[slot, 3] = str(z)


    # appliquer uniquement sur l'item correspondant
    item = root.op(f'item{slot}')
    if item:
        g = item.op('geo1')
        if g:
            g.par.tx = x
            g.par.ty = y
            g.par.tz = z
            g.par.sx = 1
            g.par.sy = 1
            g.par.sz = 1

        # pulse feedback (1 frame de délai = fiable)
        run(f"op('{item.path}/geo1/feedbackpop1').par.startpulse.pulse()")

    for i in range(1, MAX_ITEMS + 1):
        t[i, 4] = 1 if i <= threshold + 1 else 0

    return
