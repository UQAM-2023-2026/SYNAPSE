#!/usr/bin/env python3
"""
Serveur Flask pour l'application Résidences
Fournit une persistance des données via fichier JSON
"""

from flask import Flask, send_from_directory, request, jsonify
from flask_cors import CORS
import json
import os

app = Flask(__name__)
CORS(app)

# Chemin vers le fichier de données
DATA_FILE = os.path.join(os.path.dirname(__file__), 'data.json')

# Données par défaut
DEFAULT_DATA = {
    "people": ["Sasha", "Dom", "Rachel", "Florence", "Tom", "Aldric", "Émilie", "Maria",
               "Maude", "Noum", "Med", "Thierry", "No", "JC", "Justin", "Raton",
               "Anthonny", "Sarah", "Ugo", "Clément", "Max", "Mery", "Laura", "Emmanuel",
               "Alexandre", "Katianna", "Louis-David", "Léa"],
    "availabilities": {},
    "residencies": [],
    "projects": {},
    "teamMembers": {}
}

def load_data():
    """Charge les données depuis le fichier JSON"""
    if os.path.exists(DATA_FILE):
        try:
            with open(DATA_FILE, 'r', encoding='utf-8') as f:
                return json.load(f)
        except:
            return DEFAULT_DATA.copy()
    return DEFAULT_DATA.copy()

def save_data(data):
    """Sauvegarde les données dans le fichier JSON"""
    with open(DATA_FILE, 'w', encoding='utf-8') as f:
        json.dump(data, f, ensure_ascii=False, indent=2)

# Routes API
@app.route('/api/load', methods=['GET'])
def api_load():
    """Charge les données"""
    data = load_data()
    return jsonify(data)

@app.route('/api/save', methods=['POST'])
def api_save():
    """Sauvegarde les données"""
    data = request.get_json()
    save_data(data)
    return jsonify({"success": True})

# Servir les fichiers statiques
@app.route('/')
def index():
    return send_from_directory('.', 'residences.html')

@app.route('/<path:filename>')
def serve_file(filename):
    return send_from_directory('.', filename)

if __name__ == '__main__':
    print("🚀 Serveur Résidences démarré sur http://localhost:8080")
    print("📁 Les données sont sauvegardées dans data.json")
    app.run(host='0.0.0.0', port=8080, debug=False)
