# RhizomeNetwork_v7 — recharge collective basée sur E_min → E_max en T(N)
#
# Règle :
#   N = nombre de rhizomes actifs dans un groupe (même rz_link > 0)
#
#   Temps voulu E_min -> E_max :
#       N == 2 → 10 sec
#       N == 3 → 8 sec
#       N >= 4 → 6 sec
#
#   dt est dérivé du framerate réel de TD (me.time.rate)
#   deltaE/frame = ((cap - E_min) / T(N)) * dt
#
#   Donc un rhizome qui part de E_min arrivera au cap en T secondes,
#   indépendamment de ton Tick_rate custom.

def get_cfg():
    return op('Global_Config')

def set_chop_channel(chop, chan_name, value):
    """Écrit dans un Constant CHOP via son paramètre constXvalue."""
    chans = chop.chans()
    pars  = chop.pars('*value*')
    idx = None
    for i, ch in enumerate(chans):
        if ch.name == chan_name:
            idx = i
            break
    if idx is None or idx >= len(pars):
        return
    pars[idx].val = value


def _group_time_for_count(n):
    if n <= 1:
        return None
    elif n == 2:
        return 10.0
    elif n == 3:
        return 8.0
    else:
        return 6.0  # 4 et plus


def whileOn(channel, sampleIndex, val, prev):
    cfg = get_cfg()
    if cfg is None:
        return

    try:
        cap     = float(cfg['RZ_capacity'])
        min_pct = float(cfg['RZ_min_percent'])
    except:
        return

    # dt = temps réel par frame (en secondes)
    try:
        fps = me.time.rate
        if fps <= 0:
            fps = 60.0
        dt = 1.0 / fps
    except:
        dt = 1.0 / 60.0

    E_min = cap * min_pct

    parent_comp = me.parent()
    rz_comps = [c for c in parent_comp.children if c.name.startswith('RZ')]

    # GROUPS : link_id -> rhizomes
    groups = {}

    for rz_comp in rz_comps:
        rz_state = rz_comp.op('RZ_State')
        if rz_state is None:
            continue

        try:
            energy    = float(rz_state['energy'])
            connected = float(rz_state['connected'])
            rz_link_v = float(rz_state['rz_link'])
        except:
            continue

        link_id = int(round(rz_link_v))
        if link_id <= 0:
            continue

        # Un rhizome participe à SON réseau si :
        # - pas connecté à un nœud
        # - pas déjà full
        # - peu importe is_regen
        if not (connected < 0.5 and energy < cap):
            continue

        groups.setdefault(link_id, []).append({
            'state':  rz_state,
            'energy': energy,
        })

    # TRAITEMENT DE CHAQUE GROUPE
    for link_id, pool in groups.items():
        n = len(pool)
        T = _group_time_for_count(n)
        if T is None:
            continue

        # vitesse pour E_min → cap en T secondes
        target_range = cap - E_min
        if target_range <= 0:
            continue

        rate = target_range / T  # unités/sec
        deltaE = rate * dt

        if deltaE <= 0:
            continue

        for p in pool:
            energy = p['energy']
            new_energy = energy + deltaE
            if new_energy > cap:
                new_energy = cap
            set_chop_channel(p['state'], 'energy', new_energy)


    return


def onOffToOn(channel, sampleIndex, val, prev):
    return
