import socket
import struct
import time
import numpy as np

# ================== CONFIG LIDAR (PC A) ==================

LIVOX_UDP_PORT = 56000      # Port host configuré dans Livox Viewer
MAX_UDP_SIZE = 1500
_udp_sock = None

def init_udp_socket():
    global _udp_sock
    if _udp_sock is not None:
        return _udp_sock

    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.bind(("0.0.0.0", LIVOX_UDP_PORT))
    s.setblocking(True)
    print(f"[i] UDP bind sur 0.0.0.0:{LIVOX_UDP_PORT} pour Mid360")
    _udp_sock = s
    return s


def get_next_frame_points():
    """
    Lit UN paquet Livox Mid360 et retourne un np.ndarray (N, 3) en mètres.
    Gère data_type 1/2/3.
    """
    sock = init_udp_socket()
    data, addr = sock.recvfrom(MAX_UDP_SIZE)

    if len(data) < 36:
        return np.empty((0, 3), dtype=np.float32)

    header_fmt = "<B H H H H B B B 12s I Q"
    header_size = struct.calcsize(header_fmt)

    try:
        (
            version,
            length,
            time_interval,
            dot_num,
            udp_cnt,
            frame_cnt,
            data_type,
            time_type,
            reserved,
            crc32_hdr,
            timestamp
        ) = struct.unpack_from(header_fmt, data, 0)
    except struct.error:
        return np.empty((0, 3), dtype=np.float32)

    payload = data[header_size:]

    if data_type == 1:
        # Cartesian 32 bits
        point_stride = 14
        expected_size = dot_num * point_stride
        if len(payload) < expected_size:
            return np.empty((0, 3), dtype=np.float32)

        points = []
        offset = 0
        for _ in range(dot_num):
            x, y, z, refl, tag = struct.unpack_from("<iiiBB", payload, offset)
            offset += point_stride
            points.append((x * 0.001, y * 0.001, z * 0.001))  # mm -> m

        return np.array(points, dtype=np.float32)

    elif data_type == 2:
        # Cartesian 16 bits (10mm)
        point_stride = 8
        expected_size = dot_num * point_stride
        if len(payload) < expected_size:
            return np.empty((0, 3), dtype=np.float32)

        points = []
        offset = 0
        for _ in range(dot_num):
            x, y, z, refl, tag = struct.unpack_from("<hhhBB", payload, offset)
            offset += point_stride
            points.append((x * 0.01, y * 0.01, z * 0.01))  # 10mm -> m

        return np.array(points, dtype=np.float32)

    elif data_type == 3:
        # Sphérique -> cartésien
        point_stride = 10
        expected_size = dot_num * point_stride
        if len(payload) < expected_size:
            return np.empty((0, 3), dtype=np.float32)

        points = []
        offset = 0
        for _ in range(dot_num):
            depth_mm, theta_cdeg, phi_cdeg, refl, tag = struct.unpack_from("<IHHBB", payload, offset)
            offset += point_stride

            depth = depth_mm * 0.001  # m
            theta = (theta_cdeg * 0.01) * (np.pi / 180.0)
            phi   = (phi_cdeg   * 0.01) * (np.pi / 180.0)

            x = depth * np.sin(theta) * np.cos(phi)
            y = depth * np.sin(theta) * np.sin(phi)
            z = depth * np.cos(theta)
            points.append((x, y, z))

        return np.array(points, dtype=np.float32)

    else:
        return np.empty((0, 3), dtype=np.float32)


# ================== TRAITEMENT (PC A) ==================

def process_points(points: np.ndarray) -> np.ndarray:
    """
    Ici tu fais ton traitement.
    Exemples possibles :
      - filtrer sol / plafond
      - ne garder qu'une zone
      - downsample
      - clustering / seuils, etc.
    Retourne un np.ndarray (M, 3).
    """
    if points is None or points.shape[0] == 0:
        return points

    pts = points

    # Exemple 1 : enlever le sol (z < 0.0)
    pts = pts[pts[:, 2] > 0.0]

    # Exemple 2 : garder une zone en X/Y/Z (ROI)
    xmin, xmax = -5.0, 5.0
    ymin, ymax = -5.0, 5.0
    zmin, zmax = 0.0, 4.0
    mask = (
        (pts[:, 0] >= xmin) & (pts[:, 0] <= xmax) &
        (pts[:, 1] >= ymin) & (pts[:, 1] <= ymax) &
        (pts[:, 2] >= zmin) & (pts[:, 2] <= zmax)
    )
    pts = pts[mask]

    # Exemple 3 : downsample si trop dense
    MAX_POINTS = 20000
    if pts.shape[0] > MAX_POINTS:
        idx = np.random.choice(pts.shape[0], MAX_POINTS, replace=False)
        pts = pts[idx]

    return pts


# ================== ENVOI VERS PC B (POP) ==================

TD_PC_IP = "10.0.2.245"   # <-- IP du PC B (TouchDesigner)
TD_POINTS_PORT = 6000       # Port du UDP In DAT sur PC B

_points_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

def send_points_to_client(points: np.ndarray):
    """
    Envoie les points (x y z) en texte vers le PC de viz.
    """
    if points is None or points.shape[0] == 0:
        return

    lines = [f"{p[0]:.3f} {p[1]:.3f} {p[2]:.3f}" for p in points]
    payload = ("\n".join(lines)).encode("utf-8")

    try:
        _points_sock.sendto(payload, (TD_PC_IP, TD_POINTS_PORT))
    except Exception as e:
        print(f"[WARN] send_points_to_client error: {e}")


# ================== LOOP PRINCIPALE ==================

REFRESH_INTERVAL = 0.0  # 0 = aussi vite que les paquets arrivent

def main():
    print("[i] LiDAR -> Traitement (PC A) -> UDP -> POPs (PC B)")
    print(f"[i] LiDAR UDP in  : 0.0.0.0:{LIVOX_UDP_PORT}")
    print(f"[i] Client points : {TD_PC_IP}:{TD_POINTS_PORT}")

    try:
        while True:
            raw_points = get_next_frame_points()
            processed = process_points(raw_points)

            if processed is not None and processed.shape[0] > 0:
                print(f"Frame: {processed.shape[0]} points envoyés")
            else:
                print("No points after processing")

            send_points_to_client(processed)

            if REFRESH_INTERVAL > 0.0:
                time.sleep(REFRESH_INTERVAL)

    except KeyboardInterrupt:
        print("\n[i] Arrêt demandé, fermeture.")


if __name__ == "__main__":
    main()
