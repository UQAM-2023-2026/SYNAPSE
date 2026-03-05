#!/bin/bash

# Script de lancement pour l'application Résidences EDM3003

echo "╔══════════════════════════════════════════════════════════╗"
echo "║  Résidences de Création - EDM3003                        ║"
echo "╚══════════════════════════════════════════════════════════╝"
echo ""

# Vérifier que Python est installé
if ! command -v python3 &> /dev/null; then
    echo "❌ Python 3 n'est pas installé."
    echo "   Installez Python 3 depuis https://www.python.org"
    exit 1
fi

echo "✅ Python 3 détecté"
echo ""
echo "🚀 Lancement du serveur..."
echo ""

# Lancer le serveur
python3 serveur.py
