# src/layout_nsga.py
import json, math
import numpy as np
from pathlib import Path
from PIL import Image
import networkx as nx

from pymoo.core.problem import ElementwiseProblem
from pymoo.algorithms.moo.nsga2 import NSGA2
from pymoo.termination import get_termination
from pymoo.optimize import minimize

# ---------- I/O ----------
CFG = json.loads(Path("data/config.json").read_text())
MPP = float(CFG["meters_per_pixel"])
N_OBJ = int(CFG["num_objects"])
ENTRANCES = np.array(CFG["entrances"], dtype=float)

# ---------- Load walkable mask (white=1, black=0) ----------
mask_np = (np.array(Image.open(CFG["walkable_png"]).convert("L")) > 127).astype(np.uint8)
H, W = mask_np.shape

# ---------- Candidate sites (grid sampling on walkable) ----------
def sample_sites(mask, stride_px=6):
    pts = []
    for y in range(0, mask.shape[0], stride_px):
        row = mask[y]
        for x in range(0, mask.shape[1], stride_px):
            if row[x] == 1:
                pts.append((x*MPP, y*MPP))
    return np.array(pts, dtype=float)

SITES = sample_sites(mask_np, stride_px=6)  # [N,2] in meters

# ---------- Build lightweight nav graph ----------
def build_graph(mask, step_px=6):
    G = nx.Graph()
    H, W = mask.shape
    for y in range(0, H, step_px):
        for x in range(0, W, step_px):
            if mask[y, x] == 1:
                u = (x, y)
                G.add_node(u, pos=(x*MPP, y*MPP))
                for dx, dy in [(step_px,0),(0,step_px),(step_px,step_px),(step_px,-step_px)]:
                    nx_, ny_ = x+dx, y+dy
                    if 0 <= nx_ < W and 0 <= ny_ < H and mask[ny_, nx_] == 1:
                        G.add_edge(u, (nx_, ny_), weight=math.hypot(dx, dy)*MPP)
    return G

G = build_graph(mask_np, step_px=3)

def nearest_node(G, p_xy):
    # p_xy in meters; nodes have 'pos' attr in meters
    best = None; bd = 1e12
    for n, d in G.nodes(data=True):
        x, y = d["pos"]
        s = (x - p_xy[0])**2 + (y - p_xy[1])**2
        if s < bd:
            bd = s; best = n
    return best

# ---------- Coarse visibility (ray marching on mask) ----------
def visibility_score(points_xy, max_range_m=12.0, rays=8):
    if len(points_xy) == 0: return 0.0
    acc = 0.0
    for (px, py) in points_xy:
        v = 0.0
        for ang in np.linspace(0, 2*math.pi, rays, endpoint=False):
            r = 0.0
            step = MPP*2
            while r <= max_range_m:
                xi = int((px + math.cos(ang)*r)/MPP)
                yi = int((py + math.sin(ang)*r)/MPP)
                if xi < 0 or yi < 0 or xi >= W or yi >= H or mask_np[yi, xi] == 0:
                    break
                r += step
            v += r
        acc += (v / rays)
    return acc / len(points_xy)

# ---------- Congestion proxy (centrality around placed) ----------
def congestion_proxy(G, placed_nodes):
    if len(G) == 0 or len(placed_nodes) == 0: return 0.0
    # sample betweenness for speed
    k = min(400, len(G))
    bw = nx.betweenness_centrality(G, k=k)
    return np.mean([bw.get(n, 0.0) for n in placed_nodes])

# ---------- NSGA-II problem ----------
class LayoutProblem(ElementwiseProblem):
    def __init__(self, sites_xy, entrances_xy, min_clear_m):
        super().__init__(n_var=N_OBJ, n_obj=3, n_constr=1, xl=0, xu=len(sites_xy)-1, type_var=int)
        self.sites = sites_xy
        self.entrances = entrances_xy
        self.min_clear2 = (min_clear_m**2)

    def _evaluate(self, x, out, *args, **kwargs):
        idx = list(map(int, x))
        sel = self.sites[idx]  # [N,2] meters

        # Constraint: mutual clearance
        feasible = 1.0
        for i in range(len(sel)):
            for j in range(i+1, len(sel)):
                if ((sel[i]-sel[j])**2).sum() < self.min_clear2:
                    feasible = 0.0
                    break
            if feasible == 0.0: break

        # f1: avg shortest path (entrances -> nearest object)
        placed_nodes = [nearest_node(G, p) for p in sel]
        ent_nodes = [nearest_node(G, e) for e in self.entrances]
        dsum = 0.0; cnt = 0
        for en in ent_nodes:
            dists = nx.single_source_dijkstra_path_length(G, en, weight="weight")
            best = min((dists.get(n, 1e12) for n in placed_nodes))
            dsum += best; cnt += 1
        f1 = dsum / max(1, cnt)  # minimize

        # f2: maximize visibility -> minimize negative visibility
        f2 = -visibility_score(sel, max_range_m=12.0, rays=24)

        # f3: congestion proxy (minimize)
        f3 = congestion_proxy(G, placed_nodes)

        # constraint convention: <= 0 is feasible
        out["F"] = [f1, f2, f3]
        out["G"] = [0.0 if feasible == 1.0 else 1.0]

min_clear = float(CFG.get("min_clearance_m", 0.6))
problem = LayoutProblem(SITES, ENTRANCES, min_clear)

algo = NSGA2(pop_size=96, eliminate_duplicates=True)
termination = get_termination("n_gen", 160)  # augmente pour meilleure qualité

res = minimize(problem, algo, termination, seed=42, verbose=True)

# keep feasible only
X = []
F = []
for x, f, g in zip(res.X, res.F, res.G):
    if g[0] <= 0:
        X.append(x)
        F.append(f)
X = np.array(X); F = np.array(F)

# Save top Pareto set (up to 50 solutions) sorted by f1
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
