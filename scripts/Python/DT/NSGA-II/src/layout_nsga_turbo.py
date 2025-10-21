# src/layout_nsga_turbo.py
import json, math, time
import numpy as np
from pathlib import Path
from PIL import Image
import networkx as nx
from scipy.spatial import cKDTree
from scipy.ndimage import distance_transform_edt
from pymoo.core.sampling import Sampling

from pymoo.core.problem import ElementwiseProblem
from pymoo.algorithms.moo.nsga2 import NSGA2
from pymoo.termination import get_termination
from pymoo.optimize import minimize

# -------------------- CONFIG --------------------
CFG = json.loads(Path("data/config.json").read_text())
MPP = float(CFG["meters_per_pixel"])
N_OBJ = int(CFG["num_objects"])
ENTRANCES = np.array(CFG["entrances"], dtype=float)

# speed/quality knobs (overridable via config.json)
GRAPH_STEP_PX   = int(CFG.get("graph_step_px", 6))     # 3 precise, 6 fast
SITE_STRIDE_PX  = int(CFG.get("site_stride_px", 6))    # candidate sampling stride
VIS_RAYS        = int(CFG.get("vis_rays", 8))          # 24 precise, 8 fast
VIS_RANGE_M     = float(CFG.get("vis_range_m", 12.0))  # max LOS range
POP             = int(CFG.get("pop_size", 48))         # 96 precise, 48 fast
N_GEN           = int(CFG.get("n_generations", 160))    # 160 precise, 60 fast
CHECKPOINT_EVERY= int(CFG.get("checkpoint_every", 10))

MIN_OBJ_TO_WALL_M = float(CFG.get("min_obj_to_wall_m", 3.0))   # NEW: min distance to walls (m)
PLACEABLE_PATH = CFG.get("placeable_png", None)                 # NEW: optional path to placeable mask

# -------------------- LOAD MASKS --------------------
# floor: white=walkable, black=obstacle
mask_np = (np.array(Image.open(CFG["walkable_png"]).convert("L")) > 127).astype(np.uint8)
H, W = mask_np.shape

# optional "placeable" mask: white=allowed to place, black=forbidden (e.g., stairs)
placeable = None
if PLACEABLE_PATH:
    placeable = (np.array(Image.open(PLACEABLE_PATH).convert("L")) > 127).astype(np.uint8)
    assert placeable.shape == mask_np.shape, "placeable.png must have same size as floor.png"

# distance to nearest obstacle (m)
# EDT expects 1 for foreground; our walkable is 1. Distance from walkable pixels to nearest 0 (obstacle).
dist_px = distance_transform_edt(mask_np)
dist_m  = dist_px * MPP

# -------------------- CANDIDATE SITES --------------------
def sample_sites(mask, stride_px=6, min_wall_m=0.0, placeable_mask=None):
    ys = np.arange(0, mask.shape[0], stride_px)
    xs = np.arange(0, mask.shape[1], stride_px)
    xx, yy = np.meshgrid(xs, ys)
    walk_ok = (mask[yy, xx] == 1)
    wall_ok = (dist_m[yy, xx] >= min_wall_m)
    if placeable_mask is not None:
        place_ok = (placeable_mask[yy, xx] == 1)
        keep = walk_ok & wall_ok & place_ok
    else:
        keep = walk_ok & wall_ok
    pts = np.stack([xx[keep]*MPP, yy[keep]*MPP], axis=1)
    return pts

SITES = sample_sites(mask_np, stride_px=SITE_STRIDE_PX,
                     min_wall_m=MIN_OBJ_TO_WALL_M,
                     placeable_mask=placeable)
print(f"[init] SITES kept: {len(SITES)} (>= {MIN_OBJ_TO_WALL_M} m from walls; placeable mask={'on' if placeable is not None else 'off'})")

# -------------------- NAV GRAPH (COARSE) --------------------
def build_graph(mask, step_px=6):
    G = nx.Graph()
    H, W = mask.shape
    for y in range(0, H, step_px):
        for x in range(0, W, step_px):
            if mask[y, x] == 1:
                u = (x, y)
                G.add_node(u, pos=(x*MPP, y*MPP))
                for dx, dy in ((step_px,0),(0,step_px),(step_px,step_px),(step_px,-step_px)):
                    nx_, ny_ = x+dx, y+dy
                    if 0 <= nx_ < W and 0 <= ny_ < H and mask[ny_, nx_] == 1:
                        G.add_edge(u, (nx_, ny_), weight=math.hypot(dx, dy)*MPP)
    return G

G = build_graph(mask_np, step_px=GRAPH_STEP_PX)
NODE_LIST = list(G.nodes())
NODE_POS  = np.array([G.nodes[n]["pos"] for n in NODE_LIST], dtype=float)  # [M,2] meters
NODE_TREE = cKDTree(NODE_POS)
print(f"[init] Graph nodes: {len(NODE_LIST)}")

def nearest_node_idx(p_xy):
    _, idx = NODE_TREE.query(p_xy, k=1)
    return int(idx)

# -------------------- PRECOMPUTE DIJKSTRA (ENTRANCES -> ALL) --------------------
ENT_NODE_IDX = [nearest_node_idx(e) for e in ENTRANCES]
ENT_NODE_KEYS = [NODE_LIST[i] for i in ENT_NODE_IDX]

ENT_DISTS = []
for en_key in ENT_NODE_KEYS:
    d = nx.single_source_dijkstra_path_length(G, en_key, weight="weight")
    arr = np.full(len(NODE_LIST), 1e12, dtype=float)
    for i, n in enumerate(NODE_LIST):
        if n in d: arr[i] = d[n]
    ENT_DISTS.append(arr)
ENT_DISTS = np.stack(ENT_DISTS, axis=0)  # [E, M]
print(f"[init] Precomputed Dijkstra from {len(ENT_NODE_IDX)} entrances.")

# -------------------- VISIBILITY (FAST) --------------------
def visibility_score(points_xy, max_range_m=VIS_RANGE_M, rays=VIS_RAYS):
    if len(points_xy) == 0: return 0.0
    acc = 0.0
    step = MPP*2
    angles = np.linspace(0, 2*math.pi, rays, endpoint=False)
    for (px, py) in points_xy:
        v = 0.0
        for ang in angles:
            r = 0.0
            while r <= max_range_m:
                xi = int((px + math.cos(ang)*r)/MPP)
                yi = int((py + math.sin(ang)*r)/MPP)
                if xi < 0 or yi < 0 or xi >= W or yi >= H or mask_np[yi, xi] == 0:
                    break
                r += step
            v += r
        acc += (v / rays)
    return acc / len(points_xy)

# -------------------- CONGESTION PROXY (FAST) --------------------
DEG_CENT = nx.degree_centrality(G)            # dict node->val
DEG_ARRAY = np.array([DEG_CENT[n] for n in NODE_LIST], dtype=float)

def congestion_proxy_from_indices(node_indices):
    if len(node_indices) == 0: return 0.0
    return float(DEG_ARRAY[np.array(node_indices, dtype=int)].mean())

class FeasibleSampling(Sampling):
    def _do(self, problem, n_samples, **kwargs):
        # problem has: problem.n_var = N_OBJ, and we can see global SITES + min_clear
        rng = np.random.default_rng(42)
        sols = []
        tries = 0
        while len(sols) < n_samples and tries < n_samples * 50:
            # greedy pick: start from a random site, then keep adding far-enough sites
            idxs = rng.permutation(len(SITES)).tolist()
            chosen = []
            for i in idxs:
                p = SITES[i]
                if all(np.sum((p - SITES[j])**2) >= problem.min_clear2 for j in chosen):
                    chosen.append(i)
                    if len(chosen) == N_OBJ:
                        break
            if len(chosen) == N_OBJ:
                sols.append(np.array(chosen, dtype=int))
            tries += 1

        # if not enough feasible, fill the rest randomly (NSGA-II gérera via contrainte)
        while len(sols) < n_samples:
            sols.append(np.random.randint(0, len(SITES), size=problem.n_var, dtype=int))

        return np.array(sols, dtype=int)


# -------------------- NSGA-II PROBLEM --------------------
class LayoutProblem(ElementwiseProblem):
    def __init__(self, sites_xy, min_clear_m):
        super().__init__(n_var=N_OBJ, n_obj=3, n_constr=1, xl=0, xu=len(sites_xy)-1, type_var=int)
        self.sites = sites_xy
        self.min_clear2 = (min_clear_m**2)

    def _evaluate(self, x, out, *args, **kwargs):
        idx = np.array(x, dtype=int)
        sel = self.sites[idx]  # [N,2] meters

        # constraint: pairwise clearance
        feasible = 1.0
        for i in range(len(sel)):
            d2 = np.sum((sel[i+1:] - sel[i])**2, axis=1)
            if d2.size and np.min(d2) < self.min_clear2:
                feasible = 0.0; break

        # map to graph nodes
        placed_node_idx = [nearest_node_idx(p) for p in sel]

        # f1: avg shortest path entrance->nearest object
        dmins = []
        for e in range(ENT_DISTS.shape[0]):
            d_e = ENT_DISTS[e, np.array(placed_node_idx, dtype=int)]
            dmins.append(np.min(d_e))
        f1 = float(np.mean(dmins))

        # f2: maximize visibility -> minimize negative visibility
        f2 = -visibility_score(sel)

        # f3: congestion proxy (lower is better)
        f3 = congestion_proxy_from_indices(placed_node_idx)

        out["F"] = [f1, f2, f3]
        out["G"] = [0.0 if feasible == 1.0 else 1.0]

min_clear = float(CFG.get("min_clearance_m", 0.6))
problem = LayoutProblem(SITES, min_clear)

algo = NSGA2(pop_size=POP, eliminate_duplicates=True, sampling=FeasibleSampling())
termination = get_termination("n_gen", N_GEN)

# checkpoint callback
def on_gen(algorithm):
    gen = getattr(algorithm, "n_gen", None)
    if gen is None or gen == 0 or gen % CHECKPOINT_EVERY != 0:
        return
    pop = algorithm.pop
    Fp, Xp = pop.get("F"), pop.get("X")
    CVp = pop.get("CV")
    feas = (CVp <= 0).flatten() if CVp is not None else np.ones(len(pop), bool)

    sols = []
    for f, x in zip(Fp[feas], Xp[feas]):
        layout_xy = SITES[np.array(x, int)].tolist()
        sols.append({"layout": layout_xy,
                     "scores": {"distance": float(f[0]),
                                "neg_visibility": float(f[1]),
                                "congestion_proxy": float(f[2])}})
    Path("outputs").mkdir(exist_ok=True)
    Path("outputs/layouts_partial.json").write_text(json.dumps({
        "meters_per_pixel": MPP,
        "generation": int(gen),
        "solutions": sols[:50]
    }, indent=2))
    print(f"💾 checkpoint @ gen {gen} (feasible={feas.sum()})", flush=True)

# run
t0 = time.time()
res = minimize(problem, algo, termination, seed=42, verbose=True, callback=on_gen)
t1 = time.time()
print(f"⏱ total time: {t1 - t0:.1f}s for {N_GEN} gens (≈ {(t1-t0)/max(1,N_GEN):.2f}s/gen)")

# collect feasible Pareto
X, F = [], []
for x, f, g in zip(res.X, res.F, res.G):
    if g[0] <= 0:
        X.append(x); F.append(f)
X, F = np.array(X), np.array(F)

order = np.argsort(F[:,0]) if len(F) else []
solutions = []
for i in order[:50]:
    layout_xy = SITES[X[i].astype(int)].tolist()
    solutions.append({
        "layout": layout_xy,
        "scores": {"distance": float(F[i,0]),
                   "neg_visibility": float(F[i,1]),
                   "congestion_proxy": float(F[i,2])}
    })

Path("outputs").mkdir(exist_ok=True)
Path("outputs/layouts.json").write_text(json.dumps({
    "meters_per_pixel": MPP,
    "solutions": solutions
}, indent=2))
print(f"✅ Saved outputs/layouts.json with {len(solutions)} Pareto solutions")
