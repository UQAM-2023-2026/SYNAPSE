# src/pick_best_from_nsga_turbo.py
import json
import numpy as np
from pathlib import Path

# Charger le front Pareto produit par layout_nsga_turbo.py
p = Path("outputs/layouts.json")
if not p.exists():
    raise SystemExit("❌ outputs/layouts.json introuvable. Lance d'abord layout_nsga_turbo.py.")

data = json.loads(p.read_text())
solutions = data.get("solutions", [])
if not solutions:
    raise SystemExit("⚠️ Aucun layout trouvé. Vérifie que des solutions faisables ont été générées.")

# Extraire les vecteurs de scores
F = np.array([
    [
        s["scores"]["distance"],
        s["scores"]["neg_visibility"],
        s["scores"]["congestion_proxy"]
    ]
    for s in solutions
], dtype=float)

# Normaliser chaque objectif entre 0 et 1 (min = meilleur)
Fmin, Fmax = F.min(0), F.max(0)
Fz = (F - Fmin) / np.clip(Fmax - Fmin, 1e-12, None)

# Calculer la distance à l'idéal (0,0,0)
d = np.linalg.norm(Fz, axis=1)
best_i = int(np.argmin(d))

# Enregistrer la meilleure solution
best = {
    "layout": solutions[best_i]["layout"],
    "scores": solutions[best_i]["scores"],
    "meters_per_pixel": data.get("meters_per_pixel", None),
    "index_in_pareto": best_i,
    "num_solutions": len(solutions)
}

Path("outputs/best_layout.json").write_text(json.dumps(best, indent=2))
print(f"✅ Meilleur layout sauvegardé -> outputs/best_layout.json (solution {best_i+1}/{len(solutions)})")
print("Scores:", best["scores"])
