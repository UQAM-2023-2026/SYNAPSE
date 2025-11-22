# ConnectNode_v3 — RZ1

# Hypothèses:
# - Global_Config : op('../Global_Config') (Constant CHOP)
#   channels: Base_drain_rate, Drain_curve_factor,
#             Node_refuse_ratio, Tick_rate
#
# - Nodes_Energy  : op('../Nodes/Nodes_Energy') (Constant CHOP)
#   channels: supra0, supra1, supra2, infra0, infra1, infra2
#
# - RZ_State      : op('RZ_State') (Constant CHOP)
#   channels: energy, connected, target_index, is_regen, regen_t


# ---------- Utilitaires ----------

def get_cfg():
    return op('../Global_Config')

def get_nodes():
    return op('../Nodes/Nodes_Energy')

def get_rz():
    return op('RZ_State')

def set_chop_channel(chop, chan_name, value):
    """
    Met à jour un channel d'un Constant CHOP en écrivant
    dans le paramètre ValueX / constXvalue correspondant.
    On matche par index: index du channel -> index du param 'value*'.
    """
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


def _level_from_index(idx):
    # 0,1,2 -> supra ; 3,4,5 -> infra
    idx = int(idx)
    if idx <= 2:
        return 'supra'
    else:
        return 'infra'


def _indices_for_level(level):
    if level == 'supra':
        return [0, 1, 2]
    else:
        return [3, 4, 5]


def whileOn(channel, sampleIndex, val, prev):
    """
    Appelé chaque frame tant que le CHOP RZ_State est "ON"
    (cf. paramètres du CHOP Execute).
    On gère nous-mêmes la condition:
      - connected == 1
      - is_regen == 0
    """

    cfg_chop   = get_cfg()
    nodes_chop = get_nodes()
    rz_state   = get_rz()

    if cfg_chop is None or nodes_chop is None or rz_state is None:
        return

    # --- Lecture des paramètres globaux ---
    base_drain   = float(cfg_chop['Base_drain_rate'])
    k            = float(cfg_chop['Drain_curve_factor'])
    refuse_ratio = float(cfg_chop['Node_refuse_ratio'])
    try:
        dt = float(cfg_chop['Tick_rate'])
    except:
        dt = 1.0 / 60.0  # fallback si jamais

    # --- État du rhizome ---
    rz_energy   = float(rz_state['energy'])
    connected   = float(rz_state['connected'])
    is_regen    = float(rz_state['is_regen'])
    target_idx  = int(rz_state['target_index'])

    # Si pas connecté ou en regen -> on ne fait rien
    if connected < 0.5 or is_regen > 0.5:
        return

    # clamp index 0..5
    target_idx = max(0, min(5, target_idx))

    # Si le rhizome est vide -> bascule en mode regen
    if rz_energy <= 0.0:
        set_chop_channel(rz_state, 'energy', 0.0)
        set_chop_channel(rz_state, 'is_regen', 1.0)
        set_chop_channel(rz_state, 'regen_t', 0.0)
        set_chop_channel(rz_state, 'connected', 0.0)
        return

    # --- Énergie des noeuds ---
    node_energies = [float(c) for c in nodes_chop.chans()]
    if target_idx >= len(node_energies):
        return  # sécurité

    target_energy = node_energies[target_idx]

    target_level  = _level_from_index(target_idx)
    same_indices  = _indices_for_level(target_level)
    other_level   = 'infra' if target_level == 'supra' else 'supra'
    other_indices = _indices_for_level(other_level)

    # ================================
    #  REFUS + VITESSE BASÉE SUR SHARE
    # ================================

    # Énergie totale du niveau (supra ou infra)
    level_total = sum(node_energies[i] for i in same_indices)

    share = 0.0
    if level_total > 0.0:
        share = target_energy / level_total

        # Refus : si ce noeud possède déjà > refuse_ratio de l'énergie de son niveau
        if share > refuse_ratio:
            set_chop_channel(rz_state, 'connected', 0.0)
            return

    # emptiness = "manque relatif" dans ce niveau
    # - share petit  => noeud pauvre => emptiness proche de 1 => plus rapide
    # - share grand  => noeud riche  => emptiness proche de 0 => plus lent
    emptiness = 1.0 - share
    emptiness = max(0.0, min(1.0, emptiness))

    # Vitesse de vidange : base_drain modulé par emptiness
    # k contrôle la sensibilité à ce manque
    drain_rate = base_drain * (1.0 + k * emptiness)

    # quantité brute à transférer pour ce frame
    raw_amount = drain_rate * dt

    # On peut TOUT donner jusqu'à 0
    amount = min(raw_amount, rz_energy)

    if amount <= 0.0:
        return

    # --- Split 80% / 20% ---
    amount_level = amount * 0.8
    amount_other = amount * 0.2

    # --- 80% : même niveau ---
    to_connected    = amount_level * 0.6
    to_others_total = amount_level * 0.4

    other_same = [i for i in same_indices if i != target_idx]

    weights = []
    for i in other_same:
        e = node_energies[i]
        w = 1.0 / (e + 1.0)  # plus vide -> plus de poids
        weights.append(w)

    weight_sum = sum(weights) if weights else 0.0

    delta_nodes = [0.0] * len(node_energies)

    # noeud connecté
    delta_nodes[target_idx] += to_connected

    if weight_sum > 0.0 and to_others_total > 0.0:
        for idx_local, i in enumerate(other_same):
            delta_nodes[i] += to_others_total * (weights[idx_local] / weight_sum)
    else:
        # s'il n'y a pas d'autre noeud, tout va au connecté
        delta_nodes[target_idx] += to_others_total

    # --- 20% : niveau opposé ---
    lowest_idx = None
    lowest_val = 1e9
    for i in other_indices:
        if node_energies[i] < lowest_val:
            lowest_idx = i
            lowest_val = node_energies[i]

    redirected_to_level = 0.0

    if lowest_idx is not None:
        other_level_total = sum(node_energies[i] for i in other_indices)
        can_take = True
        if other_level_total > 0.0:
            share_other = node_energies[lowest_idx] / other_level_total
            if share_other > refuse_ratio:
                can_take = False

        if can_take:
            delta_nodes[lowest_idx] += amount_other
        else:
            redirected_to_level = amount_other
    else:
        redirected_to_level = amount_other

    if redirected_to_level > 0.0:
        # simple: tout ce qui ne peut pas aller à l'autre niveau revient au noeud connecté
        delta_nodes[target_idx] += redirected_to_level

    # --- Apply updates ---

    # rhizome : on enlève "amount"
    new_rz_energy = rz_energy - amount
    if new_rz_energy < 0.0:
        new_rz_energy = 0.0
    set_chop_channel(rz_state, 'energy', new_rz_energy)

    # nodes
    node_chans = nodes_chop.chans()
    for i, ch in enumerate(node_chans):
        new_val = float(ch) + delta_nodes[i]
        if new_val < 0.0:
            new_val = 0.0
        set_chop_channel(nodes_chop, ch.name, new_val)

    # Si on vient d'atteindre zéro → trigger regen comme prévu
    if new_rz_energy <= 0.0:
        set_chop_channel(rz_state, 'energy', 0.0)
        set_chop_channel(rz_state, 'is_regen', 1.0)
        set_chop_channel(rz_state, 'regen_t', 0.0)
        set_chop_channel(rz_state, 'connected', 0.0)

    return


def onOffToOn(channel, sampleIndex, val, prev):
    # Quand 'connected' passe 0->1 si tu veux faire un reset custom plus tard
    return
