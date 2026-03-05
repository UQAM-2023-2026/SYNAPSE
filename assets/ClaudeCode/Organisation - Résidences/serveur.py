#!/usr/bin/env python3
"""
Serveur web simple pour l'application de gestion des résidences
Lance sur http://localhost:8000
"""

import http.server
import socketserver
import webbrowser
import os

PORT = 8000

class MyHTTPRequestHandler(http.server.SimpleHTTPRequestHandler):
    def end_headers(self):
        # Allow local storage
        self.send_header('Access-Control-Allow-Origin', '*')
        super().end_headers()

os.chdir(os.path.dirname(os.path.abspath(__file__)))

Handler = MyHTTPRequestHandler

print(f"""
╔══════════════════════════════════════════════════════════╗
║  Serveur de Gestion des Résidences de Création          ║
╚══════════════════════════════════════════════════════════╝

🌐 Le serveur est en cours de démarrage...
📍 Adresse: http://localhost:{PORT}
📍 Fichier: residences.html

Pour y accéder depuis un autre appareil sur le même réseau:
📍 Utilisez l'adresse IP de cet ordinateur au lieu de 'localhost'

Appuyez sur Ctrl+C pour arrêter le serveur.
""")

with socketserver.TCPServer(("", PORT), Handler) as httpd:
    # Ouvrir le navigateur automatiquement
    webbrowser.open(f'http://localhost:{PORT}/index.html')

    print(f"✅ Serveur démarré! Ouverture du navigateur...\n")

    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        print("\n\n👋 Serveur arrêté. À bientôt!")
        httpd.shutdown()
