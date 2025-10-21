import json, math, random
import numpy as np
from PIL import Image
import networkx as nx
from pathlib import Path
from sklearn.neighbors import NearestNeighbors

CFG = json.loads(Path("data/config.json").read_text())
mpp = CFG["meters_per_pixel"]
N = CFG["num_objects"]

mask = (np.array(Image.open(CFG["walkable_png"]).convert('L'))>127).astype(np.uint8)
H, W = mask.shape

# --- Points candidats marchables ---
sites = [(x, y) for y in range(0, H, 6) for x in range(0, W, 6) if mask[y, x] == 1]
sites_xy = np.array([(x * mpp, y * mpp) for x, y in sites], float)

# --- Graphe de navigation ---
G = nx.Graph()
for y in range(0, H, 3):
    for x in range(0, W, 3):
        if mask[y, x] == 1:
            u = (x, y)
            G.add_node(u, pos=(x * mpp, y * mpp))
            for dx, dy in [(3, 0), (0, 3), (3, 3), (3, -3)]:
                nx_, ny_ = x + dx, y + dy
                if 0 <= nx_ < W and 0 <= ny_ < H and mask[ny_, nx_] == 1:
                    G.add_edge(u, (nx_, ny_), weight=math.hypot(dx, dy) * mpp)

def nearest_node(p):
    best = None
    bd = 1e9
    for n, d in G.nodes(data=True):
        x, y = d['pos']
        s = (x - p[0]) ** 2 + (y - p[1]) ** 2
        if s < bd:
            bd = s
            best = n
    return best

# --- Évaluation d’un layout ---
def eval_layout(idx):
    P = sites_xy[idx]
    ent_nodes = [nearest_node(tuple(e)) for e in CFG["entrances"]]
    obj_nodes = [nearest_node(tuple(p)) for p in P]

    # f1: distance moyenne
    dsum = 0; c = 0
    for en in ent_nodes:
        dists = nx.single_source_dijkstra_path_length(G, en, weight='weight')
        dsum += min(dists.get(n, 1e9) for n in obj_nodes)
        c += 1
    f1 = dsum / max(1, c)

    # f2: visibilité simplifiée (distance libre moyenne)
    def vis_point(p):
        rays = 16; acc = 0
        for a in np.linspace(0, 2 * math.pi, rays, endpoint=False):
            r = 0; step = mpp * 2
            while True:
                x = int((p[0] + math.cos(a) * r) / mpp)
                y = int((p[1] + math.sin(a) * r) / mpp)
                if x < 0 or y < 0 or x >= W or y >= H or mask[y, x] == 0: break
                r += step
                if r > 10: break
            acc += r
        return acc / rays

    vis = np.mean([vis_point(p) for p in P])
    f2 = -vis  # à minimiser

    # f3: congestion proxy (densité locale)
    if len(P) > 1:
        nn = NearestNeighbors(n_neighbors=min(6, len(P))).fit(P)
        dists, _ = nn.kneighbors(P)
        f3 = 1.0 / np.clip(dists[:, 1:].mean(), 1e-6, None)
    else:
        f3 = 0.0

    return f1, f2, f3

# --- Génération aléatoire + sélection du meilleur ---
def sample_layout():
    min_clear = CFG.get("min_clearance_m", 0.6)
    chosen = []
    tries = 0
    while len(chosen) < N and tries < 5000:
        i = random.randrange(len(sites_xy))
        if all(np.linalg.norm(sites_xy[i] - sites_xy[j]) >= min_clear for j in chosen):
            chosen.append(i)
        tries += 1
    return np.array(chosen)

cand = []
for _ in range(200):  # 200 patterns testés
    idx = sample_layout()
    if len(idx) == N:
        f1, f2, f3 = eval_layout(idx)
        cand.append((idx, [f1, f2, f3]))

# --- Normalisation + score global ---
F = np.array([c[1] for c in cand], float)
Fmin, Fmax = F.min(0), F.max(0)
Fz = (F - Fmin) / np.clip(Fmax - Fmin, 1e-9, None)
w = np.array([0.6, 0.2, 0.2])  # poids: distance > visibilité > congestion
score = (Fz * w).sum(1)
best = cand[int(np.argmin(score))]

out = {
  "layout": sites_xy[best[0]].tolist(),
  "scores": {"distance": float(best[1][0]),
             "neg_visibility": float(best[1][1]),
             "congestion_proxy": float(best[1][2])},
  "meters_per_pixel": mpp
}
Path("outputs/best_layout_baseline.json").write_text(json.dumps(out, indent=2))
print("✅ Saved outputs/best_layout_baseline.json")
