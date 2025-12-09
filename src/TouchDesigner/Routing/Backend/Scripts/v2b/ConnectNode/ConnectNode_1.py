"""
ConnectNode — version 6 nœuds (3 supra / 3 infra)
Distribution:
- nœud ciblé (RZ_Data[2]) reçoit 2 parts
- les 2 autres du même niveau reçoivent 1 part
- le nœud le plus faible du niveau opposé reçoit 1 part
Total = 5 parts (scale selon l’énergie dispo)
"""

NODES   = op('../Nodes_Energy')
RZ_DATA = op('RZ_Data')

RZ_MAX  = op('../Rz_Units')[0]
HIGH_THRESH = 95
LOW_THRESH  = 80


def _get_all_nodes():
    """Retourne les 6 nœuds dans l’ordre."""
    return [
        NODES.par.const0value,
        NODES.par.const1value,
        NODES.par.const2value,
        NODES.par.const3value,
        NODES.par.const4value,
        NODES.par.const5value,
    ]


def whileOn(channel, sampleIndex, val, prev):
    infra_check = op('../Ratio_Infra')[0]
    supra_check = op('../Ratio_Supra')[0]
    owner = me.parent()

    if not NODES or not RZ_DATA:
        return

    # lecture du CHOP
    connect       = int(RZ_DATA[0])
    state         = int(RZ_DATA[1])
    target_index  = int(RZ_DATA[2])   # 0 à 5
    rec_rate      = float(RZ_DATA[3])
    rz_energy     = float(RZ_DATA[5])
    tick          = float(RZ_DATA[9])

    if connect == 0 or state == 1:
        return

    # hysteresis
    infra_block = owner.fetch('infra_block', False)
    supra_block = owner.fetch('supra_block', False)

    if infra_check >= HIGH_THRESH:
        infra_block = True
    elif infra_check <= LOW_THRESH:
        infra_block = False

    if supra_check >= HIGH_THRESH:
        supra_block = True
    elif supra_check <= LOW_THRESH:
        supra_block = False

    owner.store('infra_block', infra_block)
    owner.store('supra_block', supra_block)

    # déterminer le niveau du nœud ciblé
    if target_index <= 2:
        level = 0  # SUPRA
        if supra_block:
            return
    else:
        level = 1  # INFRA
        if infra_block:
            return

    # quantité d’énergie disponible
    amount = rec_rate * tick
    amount = min(amount, rz_energy)
    if amount <= 0:
        return

    # récupérer tous les nœuds
    all_nodes = _get_all_nodes()

    # séparer par niveau
    supra_nodes = all_nodes[:3]
    infra_nodes = all_nodes[3:]

    # trouver les bons groupes
    if level == 0:
        target_group = supra_nodes
        opposite_group = infra_nodes
        local_index = target_index
    else:
        target_group = infra_nodes
        opposite_group = supra_nodes
        local_index = target_index - 3

    # parts
    total_parts = op('../Rz_Units')[0]
    unit = amount / total_parts

    # --- niveau ciblé ---
    for i, par in enumerate(target_group):
        val_now = float(par.eval())
        if i == local_index:
            par.val = val_now + (unit * 2.0)
        else:
            par.val = val_now + unit

    # --- niveau opposé : nœud le plus faible ---
    weakest = min(opposite_group, key=lambda p: float(p.eval()))
    weakest.val = float(weakest.eval()) + unit

    # mise à jour rhizome
    new_rz = max(0, min(rz_energy - amount, RZ_MAX))
    owner.par.Energy = new_rz

    if new_rz <= 0:
        owner.par.Connecttonode = 0

    return


def whileOff(channel, sampleIndex, val, prev):
    if not NODES or not RZ_DATA:
        return

    owner      = me.parent()
    idle       = int(RZ_DATA[10])
    rec_rate   = float(RZ_DATA[3])
    tick       = float(RZ_DATA[9])
    rz_energy  = float(RZ_DATA[5])

    if idle == 1:
        regen = rec_rate * tick * 0.3
        new_rz = min(rz_energy + regen, RZ_MAX)
        owner.par.Energy = new_rz
    return


def onOffToOn(channel, sampleIndex, val, prev): return
def onOnToOff(channel, sampleIndex, val, prev): return
def onValueChange(channel, sampleIndex, val, prev): return
