<!-- This file was generated with the assistance of an AI coding tool. -->

# BonsaiViewer50 - Mesure Plus

## Introduction

**Mesure Plus** regroupe l'ensemble des fonctions de mesure avancées du viewer, au-delà du simple clic de distance (BonsaiViewer11) : comptage, longueurs, surfaces, volumes, mesures de locaux et de niveaux, masses, attributs et pentes.


Deux principes fondamentaux :

- **Recalcul systématique depuis la géométrie 3D** : les `IfcQuantity*` (Qto) du fichier IFC ne sont jamais utilisées comme source.
- **Aucune saisie manuelle de points, courbes ou arêtes** : les géométries de mesure (contacts, arêtes, parcours) sont détectées automatiquement par le script à partir des relations IFC (`IfcRelVoidsElement`, `IfcRelConnectsPorts`) et des intersections géométriques. En cas d'objet absent de la maquette, la substitution est gérée par l'utilisateur (hors périmètre de cette spec).

## Specification

### Principes transverses

| Principe | Choix retenu |
|---|---|
| Source des quantités | Recalcul depuis la géométrie 3D (BREP/paramétrique). Les Qto IFC ne sont pas utilisées. |
| Restitution | Chaque fonction doit retourner un nombre. On doit pouvoir visualiser la quantité qui a été mesure, une ligne, une surface qui sera affiché en plus sur le viewer |
| Entrées des fonctions | Sélection/filtre d'objets IFC + paramètres scalaires uniquement (seuils, épaisseurs, coefficients, options oui/non, tables de valeurs). Le filtre est géré par ailleurs (BonsaiViewer06). |

### Utilisation de ces fonctions

Ces fonctions seront utilisables à deux endroits dans le logiciels, dans le panneau de fonction et dans les tables.
Les règles qui nécéssitent de sélectionner à la suite deux éléments ne pourront pas être présente dans les tables.

### Panneau de Mesure
Un nouveau panneau sera construit avec chaque fonction cliquable.

Au clic, la fonction sera appliqué aux éléments sélectionné. 
Si plusieurs éléments sont sélectionné, le resultat sera additionné.

Si rien n'est sélectionné, le curseur va se transformer et dès que l'utilisateur sélectionne un objet, la fonction va s'activer.

Dans ce panneau, on retrouvera également un paramètrage des fonctions, ou l'on peut configurer les paramètres supplémentaires. Ce bouton sera un raccourci qui ouvrira les settings dans une nouvelle partie "Mesure".



### Mesure dans les tables
Dans un second temps, je veux pouvoir intégrer ces fonctions dans une Table. Cela sera alors une colonne, que l'utilisateur pour configurer et qui viendra calculer le résultat de la ligne en question.



### Comptage et longueurs (M1-M5)

- **M1 `compter`** → U / Nb / ens. Entrées : sélection + attribut de regroupement. 

- **M2 `longueur_axe`** → ml. Éléments linéaires (`IfcRailing`, `IfcMember`, plinthes, `IfcKerb`, seuils, profilés, rails). Longueur suivant l'axe ; courbes en développé réel. 

- **M3 `perimetre`** → ml. Sur la base de l'objet, on prend la longueur de base de l'objet.

- **M4 `longueur_arete`** → ml. Sélection d'éléments + paramètre « type d'arête » (rive de vide, nez de dalle/quai/marche, tête d'acrotère, rive de fosse). L'arête est extraite automatiquement par intersection/contact entre faces. Courbe = longueur développée de l'arête extraite.

### Surfaces horizontales et posées (M6-M10)

- **M4 `surface_coplaine`** → m². Sélectionne une surface, la fonction vient trouver tous les triangles coplanaires adjacents et additionne les quantités.

- **M6 `surface_top`** (Top Surface) → m² de la face horizontale **supérieure** d'un élément. Entrées : tout élément surfacique (`IfcSlab`, `IfcCovering`, `IfcRoof`, chape, revêtement de sol…) + seuil de déduction des ouvertures/trémies (paramétrable). La face est identifiée par sa normale verticale vers le haut. 

- **M7 `surface_bottom`** (Bottom Surface) → m² de la face horizontale **inférieure** d'un élément. Entrées : tout élément surfacique (`IfcSlab`, `IfcCovering` plafond, sous-face de dalle, sous-face de poutre…) + seuil de déduction des trémies/réservations (paramétrable). La face est identifiée par sa normale verticale vers le bas.


- **M8 `surface_lateral`** → m². Calcule la surface latérale d'un objet, cad sans le top et sans la surface bottom.
Option: 1 face l'on choissit la plus grande face coplanaire, on obtiens sa surface.
2 faces: on prend les deux plus grandes face de l'objet.
Wall faces: On prends toutes les faces latérale.
Cette fonction sert principalement pour les murs.

- **M9 `surface_vitrage`** → Calcul les surfaces transparente d'un objet. Utilise les matériaux de l'objet pour détecter sa surface transparente et calcul la surface de la plus grande face coplanaire de cette objet.

### Surfaces développées et spéciales (M11-M16)

- **M11 `surface_toiture_developpee`** → m² en rampant (réel, pas projection). Toitures inclinées, membranes, bacs acier.

- **M12 `surface_coffrage`** → m². `IfcBeam` / `IfcColumn` / `IfcWall` / `IfcSlab` / `IfcFooting` + options « inclure abouts » (non par défaut), « inclure face supérieure » (non par défaut). Classification des faces du mesh par leur normale. Poutre : 2 joues + sous-face, abouts exclus. Formule de contrôle : `m² = L × (2h + b)`. Absent : mode « section nominale » (L, b, h en attributs ou paramètres scalaires).

- **M13 `surface_section_projeté`** → m² = périmètre de section × longueur. Entrées : `IfcDuctSegment` rectangulaires.

- **M16 `surface_ouverture`** → m² / U par Ø / ml périmètre. `IfcOpeningElement`, carottages, ouvrants de désenfumage (SUE), grilles (surface libre), rideaux (tablier = largeur × hauteur). Absent : rectangle de passage déduit automatiquement de l'`IfcDoor` liée. Sortie avec flag désenfumage pour contrôle SUE totale (M26).
Le fonctionnement n'est pas très claire, c'est à préciser.

### Volumes (M17-M20)

- **M17 `volume_element`** → m³. BREP fermé, somme des volumes signés des tétraèdres. `IfcWall`, `IfcSlab`, `IfcBeam`, `IfcFooting`, `IfcPile`, `IfcExcavation`.

- **M18 `volume_couche`** → m³ = surface (M6/M7/M8/M21) × épaisseur (attribut ou paramètre). L'épaisseur est rentrée en configuration.

- **M20 `volume_Entre`** → m³ entre 2 surfaces on calcule le volume entre les deux surfaces les plus proches. 
Comme cela demande de sélectionner deux surfaces, cela ne sera pas possible via le Table.

### Locaux, niveaux, masses, attributs, pentes (M21-M27)

- **M21 `local_surface`** → par `IfcSpace` : m² sol net. On 

- **M21 `local_périmètre`** → par `IfcSpace` et IfcWall, IfcDoor et IfcWindows : On va calculer le périmètre de l'espace qui est proche du mur. On va déduire la longueur des portes et si les fenêtres touchent le sol, on va également rétirer la longueur des fenêtres.

- **M21 `local_hauteur`** → par `IfcSpace` : m

- **M21 `local_hauteur_sousplafond`** → par `IfcSpace` et par "Plafond":
On va mesurer la hauteur entre l'espace et l'objet "Plafond".

- **M21 `local_volume_sousplafond`** → par `IfcSpace` et par "Plafond":
On multiplie la surface du local par la hauteur sous plafond.


- **M22 `local_surface_sous_plafond`** → par `IfcSpace` et `IfcElement` On veut calculer la surface qui possède plus de 1.80m de haut.


- **M27 `pente`** → % (ou °). Segment linéaire : (z₂ − z₁) / longueur horizontale × 100, option « sur longueur 3D », min/max/moyenne par groupe. Surface : ajustement d'un plan z = ax + by + c → pente = √(a² + b²) × 100 %, direction = atan2(b, a) ; équivalent via la normale (arccos(|nz|)). Absent : pente entre les 2 ports des équipements connectés (Δz / Δx via `IfcRelConnectsPorts`), ou valeur forfaitaire en paramètre. Contrôles : alertes réglementaires (PMR ≤ 5 %, pente minimale réseaux EP selon DN).


