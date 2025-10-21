import json
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont

def load_mpp(config_path: Path, layout_path: Path):
    mpp = None
    if layout_path.exists():
        try:
            mpp = json.loads(layout_path.read_text()).get("meters_per_pixel")
        except Exception:
            pass
    if (mpp is None or mpp <= 0) and config_path.exists():
        try:
            mpp = json.loads(config_path.read_text()).get("meters_per_pixel")
        except Exception:
            pass
    if not mpp or mpp <= 0:
        raise ValueError("meters_per_pixel introuvable. Ajoute-le dans outputs/best_layout.json ou data/config.json")
    return float(mpp)

def meters_to_pixels(x_m, y_m, mpp):
    return int(round(x_m / mpp)), int(round(y_m / mpp))

def draw_layout_on_image(
    img_path: Path,
    layout_json: Path,
    out_path: Path,
    config_path: Path = Path("data/config.json"),
    point_radius_px: int = 10,
    stroke_px: int = 2,
    with_labels: bool = True
):
    # charge image
    img = Image.open(img_path).convert("RGBA")
    W, H = img.size

    # charge layout
    data = json.loads(layout_json.read_text())
    layout = data["layout"]
    mpp = load_mpp(config_path, layout_json)

    # calque de dessin (pour alpha propre)
    overlay = Image.new("RGBA", (W, H), (0,0,0,0))
    draw = ImageDraw.Draw(overlay)

    # style
    color_point = (0, 255, 0, 255)     # vert
    color_outline = (0, 0, 0, 255)     # contour noir
    color_label = (255, 255, 255, 255) # texte blanc
    bbox_fill = (0, 0, 0, 180)         # fond des étiquettes

    # font (fallback PIL)
    try:
        font = ImageFont.truetype("arial.ttf", 18)
    except:
        font = ImageFont.load_default()

    # dessine chaque point
    for i, (x_m, y_m) in enumerate(layout, start=1):
        x_px, y_px = meters_to_pixels(x_m, y_m, mpp)

        # skip si en dehors
        if x_px < 0 or y_px < 0 or x_px >= W or y_px >= H:
            continue

        # disque
        r = point_radius_px
        bbox = [x_px - r, y_px - r, x_px + r, y_px + r]
        draw.ellipse(bbox, fill=color_point, outline=color_outline, width=stroke_px)

        # label optionnel
        if with_labels:
            label = f"{i}"
            tw, th = draw.textbbox((0,0), label, font=font)[2:]
            pad = 4
            rect = [x_px + r + 6, y_px - th//2 - pad, x_px + r + 6 + tw + 2*pad, y_px + th//2 + pad]
            draw.rectangle(rect, fill=bbox_fill)
            draw.text((rect[0] + pad, rect[1] + pad), label, fill=color_label, font=font)

    # compose et sauvegarde
    out = Image.alpha_composite(img, overlay).convert("RGB")
    out_path.parent.mkdir(parents=True, exist_ok=True)
    out.save(out_path, quality=95)
    print(f"✅ Image annotée sauvegardée → {out_path}")

if __name__ == "__main__":
    # chemins par défaut (tu peux changer au vol)
    IMG = Path("data/Full-Plan - SB.png")
    BEST = Path("outputs/best_layout.json")     # le layout choisi
    OUT  = Path("outputs/floor_with_best.jpg")  # image sortie

    draw_layout_on_image(
        img_path=IMG,
        layout_json=BEST,
        out_path=OUT,
        config_path=Path("data/config.json"),
        point_radius_px=10,    # taille des points
        stroke_px=2,           # contour
        with_labels=True       # affiche 1..N
    )
