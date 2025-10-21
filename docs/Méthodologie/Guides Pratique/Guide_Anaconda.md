# 🧬 Guide d’utilisation — Anaconda (Projet Mycorhize)

## 🔧 Objectif
Ce guide explique comment :
1. Ouvrir et utiliser **Anaconda**.  
2. Déployer l’environnement existant à partir d’un **backup**.  
3. Installer de nouveaux packages via `pip install`.  
4. Lancer le projet sans modifier la configuration globale.

---

## 🪴 1. Lancer Anaconda

### Sous **Windows**
- Ouvre le **menu Démarrer**.
- Recherche **Anaconda Prompt** et ouvre-le.
- (Optionnel) Tu peux aussi utiliser **Anaconda Navigator** pour une interface graphique.

### Sous **macOS / Linux**
- Ouvre ton **Terminal**.
- Tape :
  ```
  anaconda-navigator
  ```
ou directement :
  ```
  conda activate mycorhize
  ```
🧩 2. Déployer le backup de l’environnement
Un fichier environment_backup.yml (ou environment_backup.txt) est fourni dans le dossier du projet.
C’est une sauvegarde complète de l’environnement Anaconda.

📦 Étapes :
Place le fichier de backup dans ton dossier du projet.

Ouvre Anaconda Prompt ou ton Terminal.

Exécute :
```
conda env create -f environment_backup.yml
```
ou si c’est un fichier .txt :
```
conda create --name mycorhize --file environment_backup.txt
```
Active l’environnement :
```
conda activate mycorhize
```
Vérifie que tout fonctionne :
```
python --version
pip list
```
🧠 3. Ajouter de nouveaux packages
L’environnement contient déjà les dépendances principales.
Si tu veux en ajouter d’autres, utilise pip :
```
pip install numpy
pip install opencv-python
pip install pyserial
```
💡 Astuce : utilise toujours pip (et non conda install) pour rester compatible avec le backup.

Pour vérifier :
```
pip show nom_du_package
```
🧼 4. Mettre à jour les dépendances
Si un nouveau requirements.txt ou environment_backup.yml est publié :
```
pip install -r requirements.txt
```
ou
```
conda env update -f environment_backup.yml --prune
```
🧭 5. Lancer le projet
Depuis l’environnement activé :
```
cd chemin/vers/le/projet
python main.py
```
Exemple :
```
cd "C:\Users\sasha\OneDrive - UQAM\Automne 2025\Projet Final\Mycorhize"
python app.py
```
💾 6. Sauvegarder l’environnement (optionnel)
Si tu fais des modifications importantes :
```
conda env export > environment_backup.yml
```
Cela permet de partager la configuration mise à jour avec toute l’équipe.

⚠️ Erreurs fréquentes
Problème	Solution
CommandNotFoundError: conda	Anaconda n’est pas ajouté au PATH. Ouvre via Anaconda Prompt.
PackagesNotFoundError	Le package n’existe pas dans conda. Utilise pip install.
Environment already exists	Supprime l’ancien avec conda env remove -n mycorhize, puis recrée-le.

🪶 Résumé rapide
Action	Commande
Activer l’environnement	conda activate mycorhize
Installer un package	pip install nom_du_package
Lister les packages	pip list
Mettre à jour depuis backup	conda env update -f environment_backup.yml
Lancer le projet	python main.py

📁 Structure typique du projet
css
Copy code
Mycorhize/
│
├── environment_backup.yml
├── requirements.txt
├── app.py
├── data/
├── src/
└── README.md
✳️ Conseil :
Si tu partages le projet avec d’autres personnes, vérifie que le fichier environment_backup.yml est bien à jour après toute modification des dépendances.
