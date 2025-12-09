"""
RZ → NODE (version clean sans max de node) + Idle_Socket
"""

NODES   = op('../Nodes_Energy')
RZ_DATA = op('RZ_Data')
RZ_MAX  = 30


def whileOn(channel, sampleIndex, val, prev):
    if not NODES or not RZ_DATA:
        return

    connect       = int(RZ_DATA[0])   # Connect To Node
    state         = int(RZ_DATA[1])   # 0 = récolte, 1 = transfert
    target_node   = int(RZ_DATA[2])   # 0 = infra, 1 = supra
    rec_rate      = float(RZ_DATA[3])
    trf_rate      = float(RZ_DATA[4])
    rz_energy     = float(RZ_DATA[5])
    tick          = float(RZ_DATA[9])

    # choisir le noeud
    if target_node == 0:
        node_par = NODES.par.const1value   # infra
    else:
        node_par = NODES.par.const0value   # supra

    node_energy = float(node_par.eval())

    # si pas connecté, on ne fait pas la logique normale
    if connect == 0:
        return

    # --------------------- RÉCOLTE (node +, rz -) ---------------------
    if state == 0:
        amount = rec_rate * tick
        amount = min(amount, rz_energy)

        if amount > 0:
            node_par.val = node_energy + amount
            me.parent().par.Energy = rz_energy - amount

    # --------------------- TRANSFERT (rz +, node -) ---------------------
    else:
        # si le node est vide, on coupe
        if node_energy <= 0:
            me.parent().par.Connecttonode = 0
            return

        amount = trf_rate * tick
        amount = min(amount, node_energy)
        amount = min(amount, RZ_MAX - rz_energy)

        if amount > 0:
            me.parent().par.Energy = rz_energy + amount
            node_par.val = node_energy - amount

    # relecture après transfert
    new_rz_energy   = float(me.parent().par.Energy.eval())
    new_node_energy = float(node_par.eval())

    # clamp node à 0 min
    if new_node_energy < 0:
        node_par.val = 0
        me.parent().par.Connecttonode = 0

    # auto-off dépendant du state
    if state == 0:
        if new_rz_energy <= 0:
            me.parent().par.Connecttonode = 0
    else:
        if new_rz_energy >= RZ_MAX:
            me.parent().par.Connecttonode = 0

    return


def whileOff(channel, sampleIndex, val, prev):
    """
    Mode idle : quand le connect est OFF mais Idle_Socket = 1,
    le rhizome se recharge doucement ET nourrit le node doucement.
    """
    if not NODES or not RZ_DATA:
        return

    idle        = int(RZ_DATA[10])      # Idle_Socket
    target_node = int(RZ_DATA[2])       # on réutilise le même target
    rec_rate    = float(RZ_DATA[3])
    tick        = float(RZ_DATA[9])
    rz_energy   = float(RZ_DATA[5])

    if target_node == 0:
        node_par = NODES.par.const1value   # infra
    else:
        node_par = NODES.par.const0value   # supra

    node_energy = float(node_par.eval())

    if idle == 1:
        # 1) le rz se génère à 50 %
        gen_amount = rec_rate * tick * 0.3
        new_rz = min(rz_energy + gen_amount, RZ_MAX)
        me.parent().par.Energy = new_rz

        # 2) en même temps il nourrit le node à 50 %
        node_par.val = node_energy + (rec_rate * tick * 0.2)

    return
